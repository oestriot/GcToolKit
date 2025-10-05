#ifndef IO_H
#define IO_H 1

#define MAX_PATH (0x512)

SceUID kOpenDevice(const char* device, SceMode permission);
int kReadDevice(SceUID device_handle, void* data, size_t size);
int kWriteDevice(SceUID device_handle, void* data, size_t size);
int kCloseDevice(SceUID device_handle);
void kGetDeviceSize(SceUID device_handle, uint64_t* device_size);

#endif // IO_H