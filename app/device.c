#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vitasdk.h>
#include <GcKernKit.h>

#include "kernel.h"
#include "log.h"
#include "crypto.h"
#include "device.h"

#include "mbr.h"
#include "vci.h"
#include "psv.h"

#include "err.h"
#include "net.h"

#include "sha1.h"
#include "sha256.h"


// exfatfs does each 0x20000 reading internally - Princess of Sleeping 
static uint8_t DEVICE_DUMP_BUFFER[0x20000]__attribute__((aligned(0x40))); 

/*
*	Supported format header writer functions
*/

static inline int create_psv_header(BackupState* state, DeviceAccessCallback* wr_func) {
	PsvHeader psv;
	memset(&psv, 0x00, sizeof(PsvHeader));
	memcpy(psv.magic, PSV_MAGIC, sizeof(psv.magic));
	
	psv.version = PSV_VER;
	psv.flags = PSV_FLAGS;
	
	derive_cart_secret(state->keys, psv.cart_secret);
	derive_packet20_hash(state->keys, psv.packet20_sha1);
	
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
	VciHeader vci;
	memset(&vci, 0x00, sizeof(VciHeader));
	
	memcpy(vci.magic, VCI_MAGIC, sizeof(vci.magic));
	vci.version = VCI_VER;
	vci.devicesize = state->device_size;
	memcpy(&vci.keys, state->keys, sizeof(GcCmd56Keys));
	
	int wr = wr_func(state->wr_fd, &vci, sizeof(VciHeader));
	PRINT_STR("wr_func = 0x%x\n", wr);
	
	if(wr == 0) return SIZE_IS_ZERO;
	if(wr < 0) return wr;
	if(wr != sizeof(VciHeader)) return SIZE_NOT_MATCH;
	return 0;
}


static inline int finalize_psv_header(BackupState* state) {
	PRINT_STR("Finalizing PSV Header\n");
	const uint64_t offset = offsetof(PsvHeader, all_sectors_sha256);
	
	if(state->format == BACKUP_FORMAT_PSV || state->format == BACKUP_FORMAT_PSV_TRIM) {
		uint8_t sha256_out[0x20];
		sha256_final(&state->sha_ctx, sha256_out);
		PRINT_STR("sha256_out: ");
		PRINT_BUFFER(sha256_out);

		if(state->wr_fd >= 0) {
			if(state->net_info == NULL) { // phys
				uint64_t seek = sceIoLseek(state->wr_fd, offset, SCE_SEEK_SET);
				PRINT_STR("seek %llx, offset %llx\n", offset, seek);
	
				if(seek == offset) {
					int wr = sceIoWrite(state->wr_fd, sha256_out, sizeof(sha256_out));
					
					PRINT_STR("wr %x\n", wr);
					
					if(wr == 0) return SIZE_IS_ZERO;
					if(wr < 0) return wr;
					if(wr != sizeof(sha256_out)) return SIZE_NOT_MATCH;
					return 0;
				}
				
				return SIZE_NOT_MATCH;
			}
			else { // net				
				int res = send_packet_patch(state->wr_fd, state->output_path, offset, sha256_out, sizeof(sha256_out));
				PRINT_STR("res = %x\n", res);
			}
		}	
	}
	

	return 0;
}

static inline int create_header(BackupState* state, DeviceAccessCallback* wr_func) {
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


static inline uint64_t get_effective_size(BackupState* state) {
	switch(state->format) {
		case BACKUP_FORMAT_PSV:
			return state->device_size + sizeof(PsvHeader);
		case BACKUP_FORMAT_PSV_TRIM:
			return state->trim_size + sizeof(PsvHeader);
		case BACKUP_FORMAT_VCI:
			return state->device_size + sizeof(VciHeader);
		case BACKUP_FORMAT_VCI_TRIM:
			return state->trim_size + sizeof(VciHeader);
		default:
			return state->device_size;
	}
}

static inline int device_access_loop(BackupState* state, DeviceAccessCallback* rd_func, DeviceAccessCallback wr_func) {
	do {
		int rd = rd_func(state->rd_fd, DEVICE_DUMP_BUFFER, sizeof(DEVICE_DUMP_BUFFER));
		if(rd == 0) return SIZE_IS_ZERO;
		if(rd < 0) return rd;

		// hash this data if a hash is needed (i.e in psvgamesd .psv format)
		if(state->format == BACKUP_FORMAT_VCI) sha256_update(&state->sha_ctx, DEVICE_DUMP_BUFFER, rd);

		int wr = wr_func(state->wr_fd, DEVICE_DUMP_BUFFER, rd);
		if(wr == 0) return SIZE_IS_ZERO;
		if(wr < 0) return wr; 


		state->total += wr; 
		if(state->callback != NULL) state->callback(state->block_device, state->output_path, state->total, state->device_size);
	} while(state->total < state->device_size);
}

// device access functions
static inline int read_null(SceUID fd, void* data, size_t size) {
	memset(data, 0x00, size);
	return size;
}

static inline int read_data_from_image(SceUID fd, void* data, size_t size) {
	int rd = sceIoRead(fd, data, size);
	if(rd < 0) return rd;
	
	// if there is remaining space, memset it to 0
	if(rd < size) {
		memset(data+rd, 0x00, size-rd);
	}
	
	return size;
}

static inline int write_data(SceUID fd, void* data, size_t size) {
	return sceIoWrite(fd, data, size);
}


static inline int dump_device_network(BackupState* state) {
	int res = -1;
	uint64_t effective_size = get_effective_size(state);
	PRINT_STR("Begining net dump of %s to %s:%hu\n", state->block_device, state->net_info->ip_address, state->net_info->port);
	
	// open device
	state->rd_fd = kOpenDevice(state->block_device, SCE_O_RDONLY | SCE_O_RDLOCK);
	if(state->rd_fd < 0) ERROR(state->rd_fd);
	
	// open socket
	state->wr_fd = begin_file_send(state->net_info->ip_address, state->net_info->port, state->output_path, effective_size);
	if(state->wr_fd < 0) ERROR(state->wr_fd);
	
	// write vci header
	res = create_header(state, file_send_data);
	if(res < 0) goto error;
	
	// enter read/write loop
	res = device_access_loop(state, kReadDevice, file_send_data);
	if(res < 0) goto error;
	
	res = finalize_psv_header(state);
	if(res < 0) goto error;
	
error:
	if(state->wr_fd >= 0) end_file_send(state->wr_fd);
	if(state->rd_fd >= 0) kCloseDevice(state->rd_fd);
	
	return res;
}

static inline int dump_device_phys(BackupState* state) {
	int res = -1;
	
	PRINT_STR("Begining physical dump of %s to %s\n", state->block_device, state->output_path);
	
	// open device
	state->rd_fd = kOpenDevice(state->block_device, SCE_O_RDONLY | SCE_O_RDLOCK);
	if(state->wr_fd < 0) ERROR(state->rd_fd);
	
	// open image file
	state->wr_fd = sceIoOpen(state->output_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if(state->wr_fd < 0) ERROR(state->wr_fd);
	
	// write format header
	res = create_header(state, write_data);
	if(res < 0) goto error;
	
	// enter read/write loop
	res = device_access_loop(state, kReadDevice, write_data);
	if(res < 0) goto error;
	
	res = finalize_psv_header(state);
	if(res < 0) goto error;
	
error:
	if(state->wr_fd >= 0)
		sceIoClose(state->wr_fd);
	if(state->rd_fd >= 0)
		kCloseDevice(state->rd_fd);
	
	return res;		
}


int dump_device(const char* block_device, const char* output_path, BackupFormat format, GcCmd56Keys* keys, NetworkInfo* net_info, ProgressCallback* callback) {
	int res = -1;
	
	if(!kernel_started()) return KERNEL_MODULE_FAILED_START;
	
	BackupState state;
	memset(&state, 0x00, sizeof(BackupState));
	
	state.block_device = block_device;
	state.output_path = output_path;
	state.device_size = get_device_size(block_device);	
	state.trim_size = get_trimmed_size(block_device);
	state.format = format;
	state.keys = keys;
	state.callback = callback;
	state.net_info = net_info;

	PRINT_STR("state.block_device = %s\n", state.block_device);
	PRINT_STR("state.output_path = %s\n", state.output_path);
	PRINT_STR("state.device_size = %llx\n", state.device_size);
	PRINT_STR("state.trim_size = %llx\n", state.trim_size);
	PRINT_STR("state.format = %x\n", state.format);
	PRINT_STR("state.keys = %p\n", state.keys);
	PRINT_STR("state.callback = %p\n", state.callback);
	PRINT_STR("state.net_info = %p\n", state.net_info);

	sha256_init(&state.sha_ctx);	
	
	if(state.net_info == NULL) {
		res = dump_device_phys(&state);
	}
	else {
		res = dump_device_network(&state);
	}
	

	return res;
}

int restore_device(const char* block_device, char* output_path, ProgressCallback callback) {
	int res = -1;
	
	if(!kernel_started()) return KERNEL_MODULE_FAILED_START;
	DEVICE_WHITELIST_CHECK(block_device);

	// start restore ..
	PRINT_STR("Begining restore of %s to %s\n", output_path, block_device);

	BackupState state;
	memset(&state, 0x00, sizeof(BackupState));
	
	state.block_device = block_device;
	state.output_path = output_path;
	state.device_size = get_device_size(block_device);	
	state.trim_size = get_trimmed_size(block_device);
	state.format = BACKUP_FORMAT_RAW;
	state.callback = callback;

	// check image file size
	if(get_file_size(output_path) > state.device_size) ERROR(SIZE_NO_SPACE);
	
	// open image file
	state.rd_fd = sceIoOpen(output_path, SCE_O_RDONLY, 0777);
	if(state.rd_fd < 0) ERROR(state.rd_fd);
	
	// open device
	state.wr_fd = kOpenDevice(block_device, SCE_O_WRONLY);
	if(state.wr_fd < 0) ERROR(state.rd_fd);
	
	// enter read/write loop
	res = device_access_loop(&state, read_data_from_image, kWriteDevice);
	
error:
	if(state.rd_fd >= 0) sceIoClose(state.rd_fd);
	if(state.wr_fd >= 0) kCloseDevice(state.wr_fd);
	
	return res;
}

int wipe_device(const char* block_device, ProgressCallback callback) {
	int res = -1;
	
	if(!kernel_started()) return KERNEL_MODULE_FAILED_START;
	DEVICE_WHITELIST_CHECK(block_device);
	PRINT_STR("Begining wipe of %s\n", block_device);

	BackupState state;
	memset(&state, 0x00, sizeof(BackupState));
	
	state.block_device = block_device;
	state.output_path = "the void";
	state.device_size = get_device_size(block_device);
	state.trim_size = get_trimmed_size(block_device);
	state.format = BACKUP_FORMAT_RAW;
	state.callback = callback;
	
	
	// open device
	state.wr_fd = kOpenDevice(block_device, SCE_O_WRONLY);	
	if(state.wr_fd < 0) ERROR(state.wr_fd);
	
	// enter read/write loop
	res = device_access_loop(&state, read_null, kWriteDevice);
	
error:
	if(state.wr_fd >= 0) kCloseDevice(state.wr_fd);
	
	return res;
}

uint8_t device_exist(const char* block_device) {
	if(!kernel_started()) return KERNEL_MODULE_FAILED_START;
	
	SceUID dfd = kOpenDevice(block_device, SCE_O_RDONLY);
	if(dfd < 0) return 0;
	
	kCloseDevice(dfd);
	return 1;
}

uint64_t get_trimmed_size(const char* block_device) {
	// TODO: parse mbr
	
	return get_device_size(block_device);
}

uint64_t get_device_size(const char* block_device) {
	if(!kernel_started()) return KERNEL_MODULE_FAILED_START;

	uint64_t device_size = 0;

	int dfd = kOpenDevice(block_device, SCE_O_RDONLY);
	if(dfd < 0) return 0;
	
	kGetDeviceSize(dfd, &device_size);
	kCloseDevice(dfd);
	
	return device_size;
}