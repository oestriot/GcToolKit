#ifndef VCI_H 
#define VCI_H 1

#include <stdint.h>
#include <assert.h>

#include "auth.h"
#include "gc_ident.h"

#define VCI_HEADER_SIZE (0x200)

#define VCI_MAGIC "VCI\0"
#define VCI_MAJOR_VER 1
#define VCI_MINOR_VER 1

typedef struct VciHeader {
	// essential fields :
	
	char magic[0x4]; // 0x4
	uint16_t major_version; // 0x6
	uint16_t minor_version; // 0x8
	uint64_t device_size; // 0x10
	GcCmd56Keys keys; // 0x50
	
	
	/*
	* technically; these are not needed to replicate gamecarts in hardware
	*	it is mostly just curiousity; for research purposes.
	*	GcCmd56Keys is personalized per-cart anyway, so it's not changing much
	*
	* VCI version 1.0, doesn't even have these fields at all
	*
	* as far as i know; vita doesn't care about card_id other than it being an EMMC device
	* and always seemingly uses keyid 0x1, but i've included them anyway
	*
	* -> maybe it could be checked in a newer update or on the psp3 ..
	*
	*/
	
	uint16_t key_id; // 0x52
	EmmcCardId card_id; // 0x62
	
	uint8_t padding[0x19E]; // 0x200
} __attribute__((packed)) VciHeader;
static_assert(sizeof(VciHeader) == VCI_HEADER_SIZE);

#endif