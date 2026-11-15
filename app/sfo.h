#ifndef SFO_H
#define SFO_H 1

#include <stdint.h>

typedef struct SfoHeader {
	char magic[4];
	uint32_t version;
	uint32_t key_offset;
	uint32_t value_offset;
	uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoKey {
	uint16_t name_offset;
	uint8_t alignment;
	uint8_t type;
	uint32_t value_size;
	uint32_t total_size;
	uint32_t data_offset;
} __attribute__((packed)) SfoKey;

enum SfoTypes {
	PSF_TYPE_BIN = 0,
	PSF_TYPE_STR = 2,
	PSF_TYPE_VAL = 4
};

typedef struct SfoHandle {
	uint8_t* buffer;
	int64_t size;
} SfoHandle;

int open_sp_sfo(const char* sfo_file, SfoHandle* sfo);
int open_sfo(const char* sfo_file, SfoHandle* sfo);
void close_sfo(SfoHandle* sfo);
int read_sfo_key(SfoHandle* sfo, const char* sfo_key, char* out_key, size_t out_key_size);

#endif // SFO_H