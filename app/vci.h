#ifndef VCI_H 
#define VCI_H 1

#include <stdint.h>
#include <assert.h>

#include "auth.h"
#include "gc_ident.h"

#define VCI_HEADER_SIZE (0x200)

#define VCI_MAGIC "VCI\0"
#define VCI_MAJOR_VER 1
#define VCI_MINOR_VER 0

typedef struct VciHeader {
	// essential fields :
	
	char magic[0x4]; // 0x4
	uint16_t major_version; // 0x6
	uint16_t minor_version; // 0x8
	uint64_t device_size; // 0x10
	GcCmd56Keys keys; // 0x50
	
	uint8_t padding[0x1B0]; // 0x200
} __attribute__((packed)) VciHeader;
static_assert(sizeof(VciHeader) == VCI_HEADER_SIZE);

#endif