#ifndef GC_IDENT_H 
#define GC_IDENT_H 1

#include <stdint.h>
#include <stdio.h>
#include "auth.h"

#define CARD_ID_SIZE 0x10

typedef struct EmmcCardId {
	uint8_t unused : 1;
	uint8_t crc7 : 7;
	uint8_t manufacture_date : 8;
	uint32_t serial_number : 32;
	uint8_t revision : 8;
	char product_name[6];
	uint8_t oem_id : 8;
	uint8_t device_type : 2;
	uint8_t reserved : 6;
	uint8_t vendor : 8;
} __attribute__((packed)) EmmcCardId;
static_assert(sizeof(EmmcCardId) == CARD_ID_SIZE);

typedef struct GcInfo {
	EmmcCardId card_id;
	uint8_t card_descriptor[0x10];
	uint8_t extra_card_descriptor[0x200];
	uint8_t extra_card_descriptor_revision;

	uint16_t month;
	uint16_t year;
	
	uint16_t key_id;
	GcCmd56Keys key_set;
} GcInfo;

void mmc_datetime_from_byte(uint8_t rev, uint8_t mdt, uint16_t* year, uint16_t* month);
const char* mmc_vendor_id_to_manufacturer(uint8_t vendor_id);
const char* keyid_to_keygroup(uint16_t key_id);
const char* mmc_device_type_to_string(uint8_t device_type);

void get_gc_info(GcInfo* info);

#endif // GC_IDENT_H