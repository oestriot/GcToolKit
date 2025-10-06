#include "gc_ident.h"
#include "auth.h"
#include "log.h"

#include <GcToolKit.h>
#include <string.h>

const char* keyid_to_keygroup(uint16_t key_id) {
	const char* keyType = "Unknown";
	switch(key_id) {
		case 0x1:
			keyType = "Retail";
			break;
		case 0x8001:
			keyType = "Prototype Group 1";
			break;
		case 0x8002:
			keyType = "Prototype Group 2";
			break;
		case 0x8003:
			keyType = "Prototype Group 3";
			break;
	}
	return keyType;
}

const char* mmc_vendor_id_to_manufacturer(uint8_t vendor_id) {
	const char* vendor = "Unknown";
	switch(vendor_id) {
		case 0x00:
			vendor = "Sandisk";
			break;
		case 0x02:
			vendor = "Kingston or SanDisk";
			break;
		case 0x03:
		case 0x11:
			vendor = "Toshiba";
			break;
		case 0x13:
			vendor = "Micron";
			break;
		case 0x15:
			vendor = "Samsung or SanDisk or LG";
			break;
		case 0x37:
			vendor = "KingMax";
			break;
		case 0x44:
			vendor = "ATP";
			break;
		case 0x45:
			vendor = "SanDisk Corporation";
			break;
		case 0x2c:
		case 0x70:
			vendor = "Kingston";
			break;
		case 0x90:
			vendor = "Hynix";
			break;
		case 0xfe:
			vendor = "Micron";
			break;
		
	}
	
	return vendor;
}
void mmc_datetime_from_byte(uint8_t rev, uint8_t mdt, uint16_t* year, uint16_t* month) {
	uint8_t y = mdt & 0x0F;
	uint8_t m = mdt >> 4;
	
	PRINT_STR("y: %x\n", y);
	PRINT_STR("m: %x\n", m);
	
	*year = 1997 + y;
	
	if(rev > 4 && *year < 2010) {
		*year = 2013 + y;
	}
	
	*month = m;
}


const char* mmc_device_type_to_string(uint8_t device_type) {
	switch(device_type) {
		case 0b00:
			return "Removable";
		case 0b01:
			return "Embedded";
		case 0b10:
			return "POP";
		case 0b11:
			return "Reserved";
	}
	return "Invalid";
}

void get_gc_info(GcInfo* info) {
	memset(info, 0, sizeof(GcInfo));
	
	kGetCardId(1, &info->card_id);
	kGetCardCsd(1, info->card_descriptor);
	kGetCardExtCsd(1, info->extra_card_descriptor);
	
	PRINT_STR("Cid: ");
	PRINT_BUFFER(&info->card_id);

	PRINT_STR("Csd: ");
	PRINT_BUFFER(info->card_descriptor);

	PRINT_STR("ExtCsd: ");
	PRINT_BUFFER(info->extra_card_descriptor);
	
	PRINT_STR("Crc7: %x\n", info->card_id.crc7);
	PRINT_STR("ManufactureDate = 0x%x\n", info->card_id.manufacture_date);
	PRINT_STR("Serial: %x\n", info->card_id.serial_number);
	PRINT_STR("Revision: %x\n", info->card_id.revision);
	PRINT_STR("ProductName: %.6s\n", info->card_id.product_name);
	PRINT_STR("OemId: %x\n", info->card_id.oem_id);
	PRINT_STR("DeviceType: %x\n", info->card_id.device_type);
	PRINT_STR("Vendor: 0x%X\n", info->card_id.vendor);
	
	
	info->extra_card_descriptor_revision = info->extra_card_descriptor[0xC0];
	PRINT_STR("ExtCsdRev: 0x%X\n", info->extra_card_descriptor_revision);

	mmc_datetime_from_byte(info->extra_card_descriptor_revision, info->card_id.manufacture_date, &info->year, &info->month);
	
	if(kHasCmd20Captured()) {
		info->key_id = kGetLastCmd20KeyId();
		extract_gc_keys(&info->key_set);
	}
}