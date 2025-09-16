#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vitasdk.h>
#include <GcKernKit.h>

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

#define DO_CHECKED(var, func, ...) \
	int var = func(__VA_ARGS__); \
	PRINT_STR(#var " = %x\n", var); \
	if(var < 0) ERROR(var)

#define DEVICE_ACCESS_LOOP(rd_func, wr_func) \
	do { \
		int rd = rd_func(rd_fd, (void*)DEVICE_DUMP_BUFFER, sizeof(DEVICE_DUMP_BUFFER)); \
		if(rd == 0) ERROR(-2); \
		if(rd < 0) ERROR(rd); \
		\
		int wr = wr_func(wr_fd, (void*)DEVICE_DUMP_BUFFER, rd); \
		if(wr == 0) ERROR(-3); \
		if(wr < 0) ERROR(wr); \
		\
		total += wr; \
		if(progress_callback != NULL) progress_callback(block_device, path, total, device_size); \
	} while(total < device_size)

#define CREATE_VCI_HEADER(wr_func) \
	if(keys != NULL) { \
		VciHeader vci; \
		memset(&vci, 0x00, sizeof(VciHeader)); \
		\
		memcpy(vci.magic, VCI_MAGIC, sizeof(vci.magic)); \
		vci.version = VCI_VER; \
		vci.devicesize = device_size; \
		memcpy(&vci.keys, keys, sizeof(GcCmd56Keys)); \
		\
		int wr = wr_func(wr_fd, &vci, sizeof(VciHeader)); \
		PRINT_STR("wr = %x\n", wr); \
		\
		if(wr == 0) ERROR(-9680); \
		if(wr != sizeof(VciHeader)) ERROR(-9681); \
		if(wr < 0) ERROR(wr); \
	}

#define CREATE_PSV_HEADER(wr_func) \
	if(keys != NULL) { \
		PsvHeader psv; \
		memset(&psv, 0x00, sizeof(PsvHeader)); \
		memcpy(psv.magic, PSV_MAGIC, sizeof(psv.magic)); \
		\
		psv.verison = PSV_VER; \
		psv.flags = PSV_FLAGS; \
		\
		derive_cart_secret(keys, psv.cart_secret); \
		derive_packet20_hash(keys, psv.packet20_sha1); \
		memset(psv.all_sectors_sha256, 0xFF, sizeof(psv.all_sectors_sha256)); \
		\
		psv.image_size = device_size; \
		psv.image_offset = sizeof(PsvHeader) / 0x200; \
		\
		int wr = wr_func(wr_fd, &vci, sizeof(PsvHeader)); \
		PRINT_STR("wr = %x\n", wr); \
		\
		if(wr == 0) ERROR(-9682); \
		if(wr != sizeof(VciHeader)) ERROR(-9683); \
		if(wr < 0) ERROR(wr); \
	}

#define CREATE_DEV_SZ(placeholder_txt, dev_fd) \
	uint64_t device_size = 0; \
	kGetDeviceSize(dev_fd, &device_size); \
	PRINT_STR("device_size = %llx\n", device_size); \
	if(device_size == 0) ERROR(-9684); \
	if(progress_callback != NULL) progress_callback((const char*)placeholder_txt, (char*)placeholder_txt, total, device_size) \
	
#define SAFE_CHK(dev) if(memcmp(dev, "sdstor0:gcd", 11) != 0 && memcmp(dev, "sdstor0:uma", 11) != 0) ERROR(-128)

// exfatfs does each 0x20000 reading internally - Princess of Sleeping 
static uint8_t DEVICE_DUMP_BUFFER[0x20000]__attribute__((aligned(0x40))); 

uint8_t device_exist(const char* block_device) {
	int dfd = kOpenDevice(block_device, SCE_O_RDONLY);
	
	if(dfd < 0)
		return 0;
	
	kCloseDevice(dfd);
	return 1;
}

uint64_t device_size(const char* block_device) {

	uint64_t device_size = 0;

	int dfd = kOpenDevice(block_device, SCE_O_RDONLY);
	if(dfd < 0)
		return 0;
	
	kGetDeviceSize(dfd, &device_size);
	kCloseDevice(dfd);
	
	return device_size;
}

// device dump/restore

int read_null(SceUID fd, char* data, int size) {
	memset(data, 0x00, size);
	return size;
}

int read_data_from_image(SceUID fd, char* data, int size) {
	int rd = sceIoRead(fd, data, size);
	if(rd < 0) return rd;
	
	// if there is remaining space, memset it to 0
	if(rd < size) {
		memset(data+rd, 0x00, size-rd);
	}
	
	return size;
}

int dump_device_network(char* ip_address, unsigned short port, const char* block_device, char* path, GcCmd56Keys* keys, void (*progress_callback)(const char*, char*, uint64_t, uint64_t)) {
	int ret = 0;	
	uint64_t total = 0;
	
	PRINT_STR("Begining NETWORK dump of %s to %s:%u\n", block_device, ip_address, port);
	
	// open device
	DO_CHECKED(rd_fd, kOpenDevice, block_device, SCE_O_RDONLY);

	// get device size
	CREATE_DEV_SZ(block_device, rd_fd);
	
	// open socket
	DO_CHECKED(wr_fd, begin_file_send, ip_address, port, path, (keys != NULL) ? device_size + sizeof(VciHeader) : device_size);

	// write vci header
	CREATE_VCI_HEADER(file_send_data);

	// enter read/write loop
	DEVICE_ACCESS_LOOP(kReadDevice, file_send_data);
	
error:
	if(wr_fd >= 0)
		end_file_send(wr_fd);
	if(rd_fd >= 0)
		kCloseDevice(rd_fd);
	
	return ret;
}


int dump_device(const char* block_device, char* path, GcCmd56Keys* keys, void (*progress_callback)(const char*, char*, uint64_t, uint64_t)) {
	int ret = 0;
	uint64_t total = 0;

	PRINT_STR("Begining dump of %s to %s\n", block_device, path);
	
	// open device
	DO_CHECKED(rd_fd, kOpenDevice, block_device, SCE_O_RDONLY);
	
	// open image file
	DO_CHECKED(wr_fd, sceIoOpen, path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	
	// get device size
	CREATE_DEV_SZ(block_device, rd_fd);

	// write vci header
	CREATE_VCI_HEADER(sceIoWrite);

	// enter read/write loop
	DEVICE_ACCESS_LOOP(kReadDevice, sceIoWrite);
	
error:
	if(wr_fd >= 0)
		sceIoClose(wr_fd);
	if(rd_fd >= 0)
		kCloseDevice(rd_fd);
	
	return ret;
}

int restore_device(const char* block_device, char* path, void (*progress_callback)(const char*, char*, uint64_t, uint64_t)) {
	int ret = 0;
	uint64_t total = 0;
	
	SAFE_CHK(block_device);
	PRINT_STR("Begining restore of %s to %s\n", path, block_device);
	
	// get image file size
	uint64_t img_file_sz = get_file_size(path);
	PRINT_STR("img_file_sz = %llx\n", img_file_sz);
	
	// open image file
	DO_CHECKED(rd_fd, sceIoOpen, path, SCE_O_RDONLY, 0777);

	// open device
	DO_CHECKED(wr_fd, kOpenDevice, block_device, SCE_O_WRONLY);
	
	// get device size
	CREATE_DEV_SZ(block_device, wr_fd);
	if(img_file_sz > device_size) ERROR(-2);
	
	// enter read/write loop
	DEVICE_ACCESS_LOOP(read_data_from_image, kWriteDevice);
	
error:
	if(rd_fd >= 0)
		sceIoClose(rd_fd);
	if(wr_fd >= 0)
		kCloseDevice(wr_fd);
	
	return ret;
}

int wipe_device(const char* block_device, void (*progress_callback)(const char*, char*, uint64_t, uint64_t)) {
	int ret = 0;
	uint64_t total = 0;
	
	int rd_fd = 0;
	char* path = NULL;
	
	SAFE_CHK(block_device);
	PRINT_STR("Begining wipe of %s\n", block_device);
	
	// open device
	DO_CHECKED(wr_fd, kOpenDevice, block_device, SCE_O_WRONLY);
	
	// get device size
	CREATE_DEV_SZ(block_device, wr_fd);
	
	// enter read/write loop
	DEVICE_ACCESS_LOOP(read_null, kWriteDevice);
	
error:
	if(wr_fd >= 0)
		kCloseDevice(wr_fd);
	
	return ret;
}