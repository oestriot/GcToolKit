#ifndef FORMAT_H
#define FORMAT_H 1

typedef struct SceFatFormatParam { // size is 0x30-bytes
	SceUInt64 data_0x00;
	const char *path;
	void *pWorkingBuffer;
	SceSize workingBufferSize;
	SceSize bytePerCluster;
	SceSize bytePerSector;
	SceUInt32 data_0x1C;       // Unknown. Cleared by internal.
	SceUInt32 fat_time;
	SceUInt32 data_0x24;       // Unknown. Must be zero.
	SceUInt32 processing_state;
	SceUInt32 sce_fs_type;
} SceFatFormatParam;

#define SCE_FAT_FORMAT_TYPE_FAT12 (1)
#define SCE_FAT_FORMAT_TYPE_FAT16 (2)
#define SCE_FAT_FORMAT_TYPE_FAT32 (3)
#define SCE_FAT_FORMAT_TYPE_EXFAT (4)


int kFormatDevice(const char* device);
void get_format_functions();

#endif