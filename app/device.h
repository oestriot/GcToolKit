#ifndef DEVICE_H
#define DEVICE_H 1

#include <vitasdk.h>
#include <GcToolKit.h>
#include "auth.h"
#include "sha256.h"

#define BLOCK_DEVICE_GC "sdstor0:gcd-lp-ign-entire"
#define BLOCK_DEVICE_MEDIAID "sdstor0:gcd-lp-act-mediaid"
#define BLOCK_DEVICE_GRW0 "sdstor0:gcd-lp-ign-gamerw"
#define BLOCK_DEVICE_GRO0 "sdstor0:gcd-lp-ign-gamero"

#define FMT_IS_PSV(fmt) (fmt == BACKUP_FORMAT_PSV || fmt == BACKUP_FORMAT_PSV_TRIM)
#define FMT_IS_VCI(fmt) (fmt == BACKUP_FORMAT_VCI || fmt == BACKUP_FORMAT_VCI_TRIM)
#define FMT_IS_RAW(fmt) (fmt == BACKUP_FORMAT_RAW)
#define FMT_IS_TRIM(fmt) (fmt == BACKUP_FORMAT_PSV_TRIM || fmt == BACKUP_FORMAT_VCI_TRIM)

typedef enum BackupFormat {
	BACKUP_FORMAT_VCI,
	BACKUP_FORMAT_VCI_TRIM,
	BACKUP_FORMAT_PSV,
	BACKUP_FORMAT_PSV_TRIM,
	BACKUP_FORMAT_RAW
} BackupFormat;

typedef struct NetworkInfo {
	char ip_address[0x20];
	short port;
} NetworkInfo;

typedef void (ProgressCallback)(const char* block_device, const char* output_path, uint64_t total, uint64_t progress); 
typedef int (DeviceAccessCallback)(SceUID fd, void* data, size_t length); 

typedef struct BackupState {
	const char* block_device;
	const char* output_path;
	uint64_t total;
	uint64_t device_size;
	uint64_t trim_size;
	
	uint64_t header_size;
	uint64_t effective_size;
	
	uint8_t have_sha;
	SHA256_CTX sha_ctx;
	
	SceUID rd_fd;
	SceUID wr_fd;
	
	BackupFormat format;
	GcCmd56Keys* keys;
	ProgressCallback* callback;
	NetworkInfo* net_info;
	
} BackupState;


int dump_device(const char* block_device, const char* output_path, BackupFormat format, GcCmd56Keys* keys, NetworkInfo* net_info, ProgressCallback* callback);
int wipe_device(const char* block_device, ProgressCallback* callback);
int restore_device(const char* block_device, char* input_path, ProgressCallback* callback);

uint8_t device_exist(const char* block_device);
uint64_t get_device_size(const char* block_device);
uint64_t get_trimmed_size(const char* block_device);

uint64_t get_header_size(BackupFormat format);
uint64_t get_effective_size(const char* block_device, BackupFormat format);

#endif //DEVICE_H