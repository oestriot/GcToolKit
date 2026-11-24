#include <vitasdk.h>
#include <GcToolKit.h>
#include <string.h>
#include <stdio.h>
#include "mbr.h"
#include "io.h"
#include "kernel.h"
#include "backup.h"
#include "device.h"
#include "fs.h"
#include "vci.h"
#include "psv.h"
#include "err.h"

/*
* Vita exfatfs driver reads total 0x20000 (0x100 sectors) sectors at a time
* internally according to Princess of Sleeping; 
* also it should be a multiple of the sector size, (0x200) 
* also aligned it to a memory page boundary for faster read times.
*/
static uint8_t DEVICE_DUMP_BUFFER[SECTOR_SIZE * 0x100]__attribute__((aligned(0x40))); 

/*
*	Supported format header writer functions ..
*/

static int setup_state(BackupState* state, const char* block_device, const char* output_path, BackupFormat format, GcCmd56Keys* keys, ProgressCallback* callback) {
	int res = -1;
	PRINT_FUNC();
	
	memset(state, 0x00, sizeof(BackupState));
	
	state->block_device = block_device;
	state->output_path = output_path;
	
	res = get_device_size(block_device, &state->device_size);
	if(res < 0) return res;

	res = get_trimmed_size(block_device, &state->trim_size);
	if(res < 0) return res;

	res = get_header_size(format, &state->header_size);
	if(res < 0) return res;;

	res = get_effective_size(block_device, format, &state->effective_size);
	if(res < 0) return res;
	
	FsChangeSize(state->effective_size);
	
	state->format = format;

	state->keys = keys;	
		
	state->callback = callback;

	state->have_sha = FMT_IS_PSV(format);
	if(state->have_sha) sha256_init(&state->sha_ctx);
	
	
	PRINT_STR("state->block_device = %s\n", state->block_device);
	PRINT_STR("state->output_path = %s\n", state->output_path);

	PRINT_STR("state->device_size = 0x%llX\n", state->device_size);
	PRINT_STR("state->trim_size = 0x%llX\n", state->trim_size);
	PRINT_STR("state->header_size = 0x%llX\n", state->header_size);
	PRINT_STR("state->effective_size = 0x%llX\n", state->effective_size);

	PRINT_STR("state->format = 0x%02X\n", state->format);
	PRINT_STR("state->have_sha = 0x%02X\n", state->have_sha);
	
	PRINT_STR("state->keys = %p\n", state->keys);
	PRINT_STR("state->callback = %p\n", state->callback);
	
	return res;
}

static inline int create_psv_header(BackupState* state, DeviceAccessCallback* wr_func) {
	PRINT_FUNC();
	PsvHeader psv;
	memset(&psv, 0x00, sizeof(PsvHeader));
	memcpy(psv.magic, PSV_MAGIC, sizeof(psv.magic));
	
	psv.version = PSV_VER;
	psv.flags = PSV_FLAGS;
	
	kGetNpdrmKeys(&psv.keys);
	memset(psv.all_sectors_sha256, 0xFF, sizeof(psv.all_sectors_sha256));
	
	psv.image_size = state->device_size;
	psv.image_offset = sizeof(PsvHeader) / 0x200;
	
	int wr = wr_func(state->wr_fd, &psv, sizeof(PsvHeader));
	PRINT_STR("wr_func = 0x%x\n", wr);
	
	if(wr == 0) return SIZE_IS_ZERO;
	if(wr < 0) return wr;
	if(wr != sizeof(PsvHeader)) return SIZE_NOT_MATCH;
	return 0;
}

static inline int create_vci_header(BackupState* state, DeviceAccessCallback* wr_func) {
	PRINT_FUNC();
	VciHeader vci;
	memset(&vci, 0x00, sizeof(VciHeader));
	
	memcpy(vci.magic, VCI_MAGIC, sizeof(vci.magic));
	
	vci.major_version = VCI_MAJOR_VER;
	vci.minor_version = VCI_MINOR_VER;
	
	vci.device_size = state->device_size;
	memcpy(&vci.keys, state->keys, sizeof(GcCmd56Keys));
	
	int wr = wr_func(state->wr_fd, &vci, sizeof(VciHeader));
	PRINT_STR("wr_func = 0x%x\n", wr);
	
	if(wr == 0) return SIZE_IS_ZERO;
	if(wr < 0) return wr;
	if(wr != sizeof(VciHeader)) return SIZE_NOT_MATCH;
	return 0;
}

static inline int finalize_psv_header(BackupState* state) {
	int res = -1;
	PRINT_STR("Finalizing PSV Header\n");
	const uint64_t offset = offsetof(PsvHeader, all_sectors_sha256);
	
	if(FMT_IS_PSV(state->format)) {
		uint8_t sha256_out[0x20];
		sha256_final(&state->sha_ctx, sha256_out);
		
		PRINT_STR("sha256_out: ");
		PRINT_BUFFER(sha256_out);

		if(state->wr_fd >= 0) {
			res = FsFwrite(state->wr_fd, offset, sha256_out, sizeof(sha256_out));
			if(res > 0) return res;
		}	
	}
	

	return res;
}

static inline int create_header(BackupState* state, DeviceAccessCallback* wr_func) {
	PRINT_FUNC();
	int res = 0;
	switch(state->format) {
		case BACKUP_FORMAT_VCI:
		case BACKUP_FORMAT_VCI_TRIM:
			res = create_vci_header(state, wr_func);
			break;
		case BACKUP_FORMAT_PSV:
		case BACKUP_FORMAT_PSV_TRIM:
			res = create_psv_header(state, wr_func);
			break;
		case BACKUP_FORMAT_RAW:
		default:
			break;
	}
	return res;
}

static inline int device_access_loop(BackupState* state, DeviceAccessCallback* rd_func, DeviceAccessCallback wr_func) {
	uint64_t to_read = (state->effective_size - state->header_size);
	
	do {
		int rd = rd_func(state->rd_fd, DEVICE_DUMP_BUFFER, sizeof(DEVICE_DUMP_BUFFER));
		if(rd == 0) return SIZE_IS_ZERO;
		if(rd < 0) return rd;

		// sha256 hash the data if needed (i.e in psvgamesd .psv format)
		if(state->have_sha) sha256_update(&state->sha_ctx, DEVICE_DUMP_BUFFER, rd);

		int wr = wr_func(state->wr_fd, DEVICE_DUMP_BUFFER, rd);
		if(wr == 0) return SIZE_IS_ZERO;
		if(wr < 0) return wr; 
		
		state->total += wr; 
		if(state->callback != NULL) state->callback(state->block_device, state->output_path, state->total, to_read);
	} while(state->total < to_read);
	
	return 0;
}

static inline int read_null(SceUID fd, void* data, size_t size) {
	memset(data, 0x00, size);
	return size;
}

static inline int read_with_zeros(SceUID fd, void* data, size_t size) {
	int rd = FsRead(fd, data, size);
	if(rd < 0) return rd;
	
	// if there is remaining space, memset it to 0
	if(rd < size) {
		memset(data+rd, 0x00, size-rd);
	}
	
	return size;
}


int dump_device(const char* block_device, const char* output_path, BackupFormat format, GcCmd56Keys* keys, ProgressCallback* callback) {
	int res = -1;
	
	BackupState state;
	res = setup_state(&state,
			block_device,
			output_path,
			format,
			keys,
			callback);
	if(res < 0) goto error;
	
	
	PRINT_STR("Begining dump of %s to %s\n", state.block_device, state.output_path);
	make_directories_excluding_last(state.output_path);
	
	// open device
	state.rd_fd = kOpenDevice(state.block_device, SCE_O_RDONLY | SCE_O_RDLOCK);
	if(state.wr_fd < 0) ERROR(state.rd_fd);
	
	// open image file
	state.wr_fd = FsOpen(state.output_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC);
	if(state.wr_fd < 0) ERROR(state.wr_fd);
	
	// write format header
	res = create_header(&state, FsWrite);
	if(res < 0) goto error;
	
	// enter read/write loop
	res = device_access_loop(&state, kReadDevice, FsWrite);
	if(res < 0) goto error;
	
	// add hash into .psv file;
	if(FMT_IS_PSV(state.format)) res = finalize_psv_header(&state); 
	
error:
	if(state.wr_fd >= 0) FsClose(state.wr_fd);
	if(state.rd_fd >= 0) kCloseDevice(state.rd_fd);
	if(res < 0) sceIoRemove(state.output_path);
	
	return res;
}

int restore_device(const char* block_device, char* input_data, ProgressCallback callback) {
	int res = -1;
	
	// WARN:
	// only allow writing/restoring of gamecart(gcd) device and not any others;
	// this is a sanity check to avoid completely bricking the device
	// in theroy- the menu does not allow you to select any other device to begin with
	// but you can never be too sure..
	DEVICE_WHITELIST_CHECK(block_device);

	// start restore ..
	PRINT_STR("Begining restore of %s to %s\n", input_data, block_device);

	BackupState state;	
	res = setup_state(&state,
		block_device,
			input_data,
			BACKUP_FORMAT_RAW,
			NULL,
			callback);
	if(res < 0) goto error;

	// check image file size
	if(state.effective_size < get_file_size(input_data)) ERROR(SIZE_NO_SPACE);

	// open image file
	state.rd_fd = FsOpen(input_data, SCE_O_RDONLY);
	if(state.rd_fd < 0) ERROR(state.rd_fd);
	
	// open device
	state.wr_fd = kOpenDevice(block_device, SCE_O_WRONLY | SCE_O_WRLOCK);
	if(state.wr_fd < 0) ERROR(state.rd_fd);
	
	// enter read/write loop
	res = device_access_loop(&state, read_with_zeros, kWriteDevice);

error:
	if(state.rd_fd >= 0) FsClose(state.rd_fd);
	if(state.wr_fd >= 0) kCloseDevice(state.wr_fd);
	
	return res;
}

int wipe_device(const char* block_device, ProgressCallback callback) {
	int res = -1;
	
	// WARN:
	// only allow wiping of gamecart(gcd) device and not any others;
	// this is a sanity check to avoid completely bricking the device
	// in theroy- the menu does not allow you to select any other device to begin with
	// but you can never be too sure..
	
	DEVICE_WHITELIST_CHECK(block_device);
	PRINT_STR("Begining wipe of %s\n", block_device);

	BackupState state;
	res = setup_state(&state,
			block_device,
			"the void",
			BACKUP_FORMAT_RAW,
			NULL,
			callback);
	if(res < 0) goto error;
	
		
	// open device
	state.wr_fd = kOpenDevice(block_device, SCE_O_WRONLY | SCE_O_WRLOCK);	
	if(state.wr_fd < 0) ERROR(state.wr_fd);
	
	// enter read/write loop
	res = device_access_loop(&state, read_null, kWriteDevice);
	
error:
	if(state.wr_fd >= 0) kCloseDevice(state.wr_fd);

	return res;
}

// translate enums to extension/types

const char* wht_to_blkdev(BackupWhat what) {
	switch(what) {
		case WHT_WHOLE_GAMECART:
			return BLOCK_DEVICE_GC;
		case WHT_MEDIA_ID:
			return BLOCK_DEVICE_MEDIAID;
		case WHT_GAMECART_REWRITABLE:
			return BLOCK_DEVICE_GRW0;
		case WHT_GAMECART_READONLY:
			return BLOCK_DEVICE_GRO0;
		case WHT_CMD56_KEYS:
		case WHT_NPDRM_KEYS:
		case WHT_REPORT:
		default:
			return NULL;
	}
}

const char* wht_to_extension(BackupWhat what) {
	switch(what) {
		case WHT_MEDIA_ID:
			return ".mediaid";
		case WHT_CMD56_KEYS:
			return ".keys.bin";
		case WHT_NPDRM_KEYS:
			return ".npdrm.bin";
		case WHT_GAMECART_READONLY:
		case WHT_RAW_PARTITION:
		case WHT_GAMECART_REWRITABLE:
			return ".img";
		case WHT_REPORT:
			return ".report.txt";
		default:
			return ".bin";
	}
}

const char* fmt_to_extension(BackupFormat fmt) {
	switch(fmt) {
		case BACKUP_FORMAT_VCI:
			return ".vci";
		case BACKUP_FORMAT_VCI_TRIM:
			return ".trim.vci";
		case BACKUP_FORMAT_PSV:
			return ".psv";
		case BACKUP_FORMAT_PSV_TRIM:
			return "trim.psv";
		case BACKUP_FORMAT_RAW:
		default:
			return ".img";
	}
}