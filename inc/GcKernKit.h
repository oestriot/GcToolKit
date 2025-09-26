/*
*	GcKernKit export headers
*/

#ifndef GC_KERN_KIT_H
#define GC_KERN_KIT_H 1
#include <stdint.h>
#include <psp2common/kernel/iofilemgr.h>

int kResetGc();
int kClearCartSecret();
int kGetCartSecret(uint8_t* keys);

int kHasCmd20Captured();
int kGetLastCmd20KeyId();
int kGetLastCmd20Input(void* cmd20_input);
int kResetCmd20Input();

SceUID kOpenDevice(const char* device, SceMode permission);
int kReadDevice(SceUID device_handle, void* data, size_t size);
int kWriteDevice(SceUID device_handle, void* data, size_t size);
int kCloseDevice(SceUID device_handle);
void kGetDeviceSize(SceUID device_handle, uint64_t* device_size);

int kFormatDevice(const char* device);

int kGetCardId(int deviceIndex, void* cardId);
int kGetCardCsd(int deviceIndex, void* cardCsd);
int kGetCardExtCsd(int deviceIndex, void* cardExtCsd);

typedef struct SceSblSmCommGcData {
    int always1;
    int command;
    uint8_t data[2048];
    int key_id;
    int size;
    int always0;
} SceSblSmCommGcData;

typedef struct CommsData { 
    uint8_t packet6[32];
    uint8_t packet9[48];
    uint8_t packet17[32];
    uint8_t packet18[67];
    uint8_t packet19[16];
    uint8_t packet20[83];
} CommsData;


#endif // GC_KERN_KIT_H