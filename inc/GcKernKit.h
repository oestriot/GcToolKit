/*
*	GcKernKit export headers
*/

#ifndef GC_KERN_KIT_H
#define GC_KERN_KIT_H 1

#include <stdint.h>
#include <psp2common/kernel/iofilemgr.h>
#include <assert.h>

/*
* GcCmd56Keys are:
*	Keys exchanged as part of the CMD56 authentication process.
*	sha256(GcCmd56Keys) equals cart_secret in (e.g psvgamesd)
*	despite not needing to be; these keys differ per-cart, not per-game.
*	
*	and every gamecart has its own license RIF to go along with this
*	for this reason, every vita gamecart is personalized.
*
*/

typedef struct GcCmd56Keys{
	uint8_t packet20_key[0x20];
	uint8_t packet18_key[0x20];
} __attribute__((packed)) GcCmd56Keys;
static_assert(sizeof(GcCmd56Keys) == 0x40);


typedef struct GcNpdrmKeys {
	uint8_t cart_secret[0x20];
	uint8_t packet20_sha1[0x14];
} __attribute__((packed)) GcNpdrmKeys;
static_assert(sizeof(GcNpdrmKeys) == 0x34);


// auth_emu.c
int kIsAuthenticated();
uint16_t kGetKeyId();
void kGetPerCartKeys(GcCmd56Keys* output);
void kGetNpdrmKeys(GcNpdrmKeys* output);
int kCheckCartHash(const uint8_t* hash);
int kEnableGcEmuMgr();
int kDisableGcEmuMgr();

// io.c
SceUID kOpenDevice(const char* device, SceMode permission);
int kReadDevice(SceUID device_handle, void* data, size_t size);
int kWriteDevice(SceUID device_handle, void* data, size_t size);
int kCloseDevice(SceUID device_handle);
int kGetDeviceSize(const char* device, int64_t* device_size);
int kWriteShared(SceUID device_handle, void* data, size_t sz);
int kReadShared(SceUID device_handle, void* data, size_t sz);

// format.c
int kFormatDevice(const char* device);

// gc.c
int kGetCardId(int deviceIndex, void* cardId);
int kGetCardCsd(int deviceIndex, void* cardCsd);
int kGetCardExtCsd(int deviceIndex, void* cardExtCsd);
int kResetGc();
int kIsSdInserted();
int kIsMmcInserted();

// sd2vita.c
void kUndoSd2Vita();

// power.c
int kPowerSetCallbackState(uint32_t enable);

// memory.c
void* kStartUserMap(void* data, size_t size);
void kEndUserMap(void* data);

#endif // GC_KERN_KIT_H