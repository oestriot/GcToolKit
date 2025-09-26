#ifndef VCI_H 
#define VCI_H
#include <stdint.h>

#define VCI_MAGIC "VCI\0"
#define VCI_VER (0x1)

typedef struct GcCmd56Keys{
	uint8_t packet20_key[0x20];
	uint8_t packet18_key[0x20];
} __attribute__((packed)) GcCmd56Keys;

typedef struct VciHeader {
	char magic[0x4];
	int version;
	uint64_t devicesize;
	GcCmd56Keys keys;
	uint8_t padding[0x1B0];
} __attribute__((packed)) VciHeader;


#endif