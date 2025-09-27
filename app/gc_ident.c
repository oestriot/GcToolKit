#include "gc_ident.h"
#include "auth.h"
#include "log.h"

#include <GcToolKit.h>
#include <string.h>

const char* keyid_to_keygroup(uint16_t keyId) {
	const char* keyType = "Unknown";
	switch(keyId) {
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

const char* mmc_vendor_id_to_manufacturer(uint8_t vendorId) {
	const char* vendor = "Unknown";
	switch(vendorId) {
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
	
	// sony doesnt set the rev bit to 0x4 in EXT_CSD ??
	// clearly these carts are not created in 1994.
	
	if(rev > 4 && *year < 2010) {
		*year = 2013 + y;
	}
	
	*month = m;
}

void get_gc_info(GcInfo* info) {
	memset(info, 0, sizeof(GcInfo));
	
	kGetCardId(1, info->Cid);
	kGetCardCsd(1, info->Csd);
	kGetCardExtCsd(1, info->ExtCsd);
	
	PRINT_STR("Cid: ");
	PRINT_BUFFER(info->Cid);

	PRINT_STR("Csd: ");
	PRINT_BUFFER(info->Csd);

	PRINT_STR("ExtCsd: ");
	PRINT_BUFFER(info->ExtCsd);
	
	info->Vendor = info->Cid[0xF];
	PRINT_STR("Vendor: 0x%X\n", info->Vendor);
	
	info->ExtCsdRev = info->ExtCsd[0xC0];
	PRINT_STR("ExtCsdRev: 0x%X\n", info->ExtCsdRev);
	
	memcpy(&info->DeviceSerial, info->Cid + 0x2, 0x4);
	PRINT_STR("DeviceSerial: %x\n", info->DeviceSerial);

	info->DeviceRev = info->Cid[0x6];
	PRINT_STR("DeviceRev: %x\n", info->DeviceRev);

	memcpy(info->DeviceName, info->Cid + 0x7, 0x6);
	PRINT_STR("DeviceName: %s\n", info->DeviceName);
	
	
	uint8_t mdt = info->Cid[1];
	PRINT_STR("mdt = 0x%x\n", mdt);
	
	mmc_datetime_from_byte(info->ExtCsdRev, mdt, &info->Year, &info->Month);
	
	if(kHasCmd20Captured()) {
		info->KeyId = kGetLastCmd20KeyId();
		extract_gc_keys(&info->KeySet);
	}
}