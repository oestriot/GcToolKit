#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <vitasdkkern.h>
#include "GcKernKit.h"


SceUID k_open_device(const char* device, int permission) {
	// check if device is being opened for writing ...
	if(permission & (SCE_O_WRONLY | SCE_O_WRONLY | SCE_O_APPEND | SCE_O_TRUNC | SCE_O_CREAT) != 0) DEVICE_WHITELIST_CHECK(device);
	
	int prev = ksceKernelSetPermission(0x80);
	int fd = ksceIoOpen(device, permission, 0777);
	ksceKernelSetPermission(prev);
	return fd;
}

int k_read_device(SceUID device_handle, void* data, int size) {
	int prev = ksceKernelSetPermission(0x80);
	int res = ksceIoRead(device_handle, data, size);
	ksceKernelSetPermission(prev);
	return res;
}

int k_write_device(SceUID device_handle, void* data, int size) {
	int prev = ksceKernelSetPermission(0x80);
	int res = ksceIoWrite(device_handle, data, size);
	ksceKernelSetPermission(prev);
	return res;
}

int k_close_device(SceUID device_handle){
	int prev = ksceKernelSetPermission(0x80);
	ksceIoClose(device_handle);
	ksceKernelSetPermission(prev);
	return 0;
}

uint64_t k_get_device_size(SceUID device_handle) {
	int prev = ksceKernelSetPermission(0x80);	
	uint64_t device_size = ksceIoLseek(device_handle, 0, SCE_SEEK_END);
	ksceIoLseek(device_handle, 0, SCE_SEEK_SET);
	ksceKernelSetPermission(prev);
	
	return device_size;
}

// io syscalls

SceUID kOpenDevice(const char* device, int permission) {
	static char k_device[1028];
	
	ksceKernelStrncpyUserToKernel(k_device, (const void*)device, sizeof(k_device));
	
	return k_open_device((const char*)k_device, permission);
}


int kWriteDevice(SceUID device_handle, void* data, size_t size) {
	void* k_data = NULL;
	size_t k_size = 0;
	uint32_t k_offset = 0;
	
	int uid = ksceKernelUserMap("GcKernKit_WRITE", 3, data, size, &k_data, &k_size, &k_offset);
	if(uid < 0)
		return uid;
	int res = k_write_device(device_handle, (k_data + k_offset) , size);
	ksceKernelMemBlockRelease(uid);
	
	return res;
}


int kReadDevice(SceUID device_handle, void* data, size_t size) {
	void* k_data = NULL;
	size_t k_size = 0;
	uint32_t k_offset = 0;
	
	int uid = ksceKernelUserMap("GcKernKit_READ", 3, data, size, &k_data, &k_size, &k_offset);
	if(uid < 0)
		return uid;
	int res = k_read_device(device_handle, (k_data + k_offset) , size);
	ksceKernelMemBlockRelease(uid);
	
	return res;
}

int kCloseDevice(SceUID device_handle) {
	return k_close_device(device_handle);
}

void kGetDeviceSize(int device_handle, uint64_t* device_size) {
	uint64_t k_device_size = k_get_device_size(device_handle);
	ksceKernelMemcpyKernelToUser(device_size, (const void*)&k_device_size, sizeof(uint64_t));
}

