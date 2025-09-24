#ifndef _IO_H
#define _IO_H 1

SceUID kOpenDevice(const char* device, SceMode permission);
int kReadDevice(SceUID device_handle, void* data, size_t size);
int kWriteDevice(SceUID device_handle, void* data, size_t size);
int kCloseDevice(SceUID device_handle);
void kGetDeviceSize(SceUID device_handle, uint64_t* device_size);

#endif // _IO_H