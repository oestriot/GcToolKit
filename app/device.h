#ifndef DEVICE_H
#define DEVICE_H 1

#include <vitasdk.h>
#include <GcToolKit.h>
#include "auth.h"
#include "sha256.h"
#include "backup.h"

extern const char* BLOCK_DEVICES[][2];

#define BLOCK_DEVICE_GC "sdstor0:gcd-lp-ign-entire"
#define BLOCK_DEVICE_MEDIAID "sdstor0:gcd-lp-act-mediaid"
#define BLOCK_DEVICE_GRW0 "sdstor0:gcd-lp-ign-gamerw"
#define BLOCK_DEVICE_GRO0 "sdstor0:gcd-lp-ign-gamero"

uint8_t device_exist(const char* block_device);

int get_device_size(const char* block_device, uint64_t* size);
int get_trimmed_size(const char* block_device, uint64_t* size);

int get_header_size(BackupFormat format, uint64_t* size);
int get_effective_size(const char* block_device, BackupFormat format, uint64_t* size);

#endif //DEVICE_H