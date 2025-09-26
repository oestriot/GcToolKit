#include <stdint.h>

typedef struct GcSfoHeader {
	char magic[4];
	uint32_t version;
	uint32_t key_offset;
	uint32_t value_offset;
	uint32_t count;
} GcSfoHeader;

typedef struct GcSfoKey {
	uint16_t name_offset;
	uint8_t alignment;
	uint8_t type;
	uint32_t value_size;
	uint32_t total_size;
	uint32_t data_offset;
} GcSfoKey;

enum GcSfoTypes {
	PSF_TYPE_BIN = 0,
	PSF_TYPE_STR = 2,
	PSF_TYPE_VAL = 4
};

int read_sfo_key(const char* sfo_file, const char* sfo_key, char* out_key, size_t out_key_size);