#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <vitasdkkern.h>
#include <GcToolKit.h>
#include "io.h"
#include "log.h"

SceUID k_open_device(const char* device, int permission) {
	// check if device is being opened for writing ...
	if(permission & (SCE_O_WRONLY | SCE_O_WRONLY | SCE_O_APPEND | SCE_O_TRUNC | SCE_O_CREAT) != 0) DEVICE_WHITELIST_CHECK(device);
	
	int prev = ksceKernelSetPermission(0x80);
	SceUID fd = ksceIoOpen(device, permission, 0777);
	ksceKernelSetPermission(prev);

	if(fd < 0) PRINT_STR("failed : %x\n", fd);
	return fd;
}

int k_read_device(SceUID device_handle, void* data, int size) {
	int prev = ksceKernelSetPermission(0x80);
	int res = ksceIoRead(device_handle, data, size);
	ksceKernelSetPermission(prev);

	if(res < 0) PRINT_STR("failed : %x\n", res);
	return res;
}

int k_write_device(SceUID device_handle, void* data, int size) {
	int prev = ksceKernelSetPermission(0x80);
	int res = ksceIoWrite(device_handle, data, size);
	ksceKernelSetPermission(prev);

	if(res < 0) PRINT_STR("failed : %x\n", res);
	return res;
}

int k_close_device(SceUID device_handle){
	int prev = ksceKernelSetPermission(0x80);
	ksceIoClose(device_handle);
	ksceKernelSetPermission(prev);
	return 0;
}

int k_get_device_size(char* device, int64_t* size) {
	PRINT_FUNC();
	if(size == NULL) return -1;
	
	int prev = ksceKernelSetPermission(0x80);	
	SceUID fd = ksceIoOpen(device, SCE_O_RDONLY | SCE_O_RDLOCK, 0777);
	if(fd < 0) { PRINT_STR("failed: %x\n", fd);  return fd; }
	
	*size = ksceIoLseek(fd, 0, SCE_SEEK_END);
	ksceIoClose(fd);
	ksceKernelSetPermission(prev);

	PRINT_STR("%s: %llx\n", device, *size);

	return 0;
}

// io syscalls

SceUID kOpenDevice(const char* device, int permission) {
	static char k_device[MAX_PATH];
	
	ksceKernelStrncpyUserToKernel(k_device, (const void*)device, sizeof(k_device));
	
	return k_open_device((const char*)k_device, permission);
}


int kWriteDevice(SceUID device_handle, void* data, size_t size) {
	void* k_data = NULL;
	size_t k_size = 0;
	uint32_t k_offset = 0;
	
	int uid = ksceKernelUserMap("GcKernKit_WRITE", 3, data, size, &k_data, &k_size, &k_offset);
	if(uid < 0) return uid;
	
	int res = k_write_device(device_handle, (k_data + k_offset) , size);
	ksceKernelMemBlockRelease(uid);
	
	return res;
}


int kReadDevice(SceUID device_handle, void* data, size_t size) {
	void* k_data = NULL;
	size_t k_size = 0;
	uint32_t k_offset = 0;
	
	int uid = ksceKernelUserMap("GcKernKit_READ", 3, data, size, &k_data, &k_size, &k_offset);
	if(uid < 0) return uid;
	
	int res = k_read_device(device_handle, (k_data + k_offset) , size);
	ksceKernelMemBlockRelease(uid);
	
	return res;
}

int kCloseDevice(SceUID device_handle) {
	return k_close_device(device_handle);
}

int kGetDeviceSize(const char* device, int64_t* device_size) {
	PRINT_FUNC();
	
	static char k_device[MAX_PATH];
	static int64_t k_size = 0;
	
	ksceKernelStrncpyUserToKernel(k_device, (const void*)device, sizeof(k_device));
	
	int res = k_get_device_size(k_device, &k_size);
	if(res < 0) return res;
	
	ksceKernelMemcpyKernelToUser(device_size, (const void*)&k_size, sizeof(int64_t));
}

