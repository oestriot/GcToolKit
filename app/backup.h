#ifndef BACKUP_H
#define BACKUP_H
#include "sha256.h"

#define FMT_IS_PSV(fmt) (fmt == BACKUP_FORMAT_PSV || fmt == BACKUP_FORMAT_PSV_TRIM)
#define FMT_IS_VCI(fmt) (fmt == BACKUP_FORMAT_VCI || fmt == BACKUP_FORMAT_VCI_TRIM)
#define FMT_IS_RAW(fmt) (fmt == BACKUP_FORMAT_RAW)
#define FMT_IS_TRIM(fmt) (fmt == BACKUP_FORMAT_PSV_TRIM || fmt == BACKUP_FORMAT_VCI_TRIM)

#define WHT_IS_EXFAT(wht) (wht == WHT_GAMECART_READONLY || wht == WHT_GAMECART_REWRITABLE)
#define WHT_IS_RAW(wht) (wht == WHT_MEDIA_ID)


typedef enum BackupFormat {
	BACKUP_FORMAT_VCI,
	BACKUP_FORMAT_VCI_TRIM,
	BACKUP_FORMAT_PSV,
	BACKUP_FORMAT_PSV_TRIM,
	BACKUP_FORMAT_RAW
} BackupFormat;

typedef enum BackupWhat {
	WHT_WHOLE_GAMECART,
	WHT_CMD56_KEYS,
	WHT_NPDRM_KEYS,
	WHT_MEDIA_ID,
	WHT_GAMECART_REWRITABLE,
	WHT_GAMECART_READONLY,
	WHT_REPORT,
	WHT_RAW_PARTITION
} BackupWhat;

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
	
} BackupState;



int dump_device(const char* block_device, const char* output_path, BackupFormat format, GcCmd56Keys* keys, ProgressCallback* callback);
int wipe_device(const char* block_device, ProgressCallback* callback);
int restore_device(const char* block_device, char* input_path, ProgressCallback* callback);


const char* fmt_to_extension(BackupFormat fmt);
const char* wht_to_extension(BackupWhat what);
const char* wht_to_blkdev(BackupWhat what);

#endif