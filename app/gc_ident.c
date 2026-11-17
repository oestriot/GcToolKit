#include "gameinfo.h"
#include "gc_ident.h"
#include "auth.h"
#include "log.h"
#include "net.h"

#include <GcToolKit.h>
#include <string.h>

#define WTXT(...) if(rptr < (rtxt + rtxt_len)) { snprintf(rptr, (rtxt_len - (rptr - rtxt)-1), __VA_ARGS__); rptr += strlen(rptr); };
#define WBUF(buffer) do { \
						 for(int i = 0; i < sizeof(buffer); i++) { \
								WTXT("%02X ", *((uint8_t*)buffer + i)); \
						 } \
						 WTXT("\n"); \
					 } while(0);
#define SBOOL(val) val ? "Yes" : "No", val

const char* keyid_to_keygroup(uint16_t key_id) {
	const char* key_type = "Unknown";
	switch(key_id) {
		case 0x1:
			key_type = "Retail";
			break;
		case 0x8001:
			key_type = "Prototype Group 1";
			break;
		case 0x8002:
			key_type = "Prototype Group 2";
			break;
		case 0x8003:
			key_type = "Prototype Group 3";
			break;
	}
	return key_type;
}

// origin: WiiUIdent which i think(?) got it from LSBLK..
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



void get_gc_info(GcInfo* info) {
	memset(info, 0, sizeof(GcInfo));
	
	kGetCardId(1, &info->card_id);
	kGetCardCsd(1, &info->card_descriptor);
	kGetCardExtCsd(1, info->extra_card_descriptor);
	kGetNpdrmKeys(&info->npdrm_keys);
	
	info->extra_card_descriptor_revision = info->extra_card_descriptor[0xC0];
	
	mmc_datetime_from_byte(info->extra_card_descriptor_revision, info->card_id.manufacture_date, &info->year, &info->month);
	
	if(kIsAuthenticated()) {
		info->key_id = kGetKeyId();
		kGetPerCartKeys(&info->key_set);
	}
}


size_t create_report(char* rtxt, size_t rtxt_len) {
	char hex[0x1000] = { 0 };
	char title[50];
	char title_id[50];
	
	memset(rtxt, 0x00, rtxt_len);
	SceDateTime datetime;
	char* rptr = rtxt; // set ptr

	// read game information ...
	GcInfo info = { 0 };
	get_gc_info(&info);
	read_gameinfo(title_id, title, sizeof(title));
	
	// get current time ..
	sceRtcGetCurrentClockLocalTime(&datetime);
	
	WTXT("GcToolKit v%.02f\n", GCTK_VERSION);
	WTXT("%s [%s] - Report created %02hu/%02hu/%04hu %02hu:%02hu:%02hu.%iµs\n", title, title_id,
																			datetime.day, datetime.month, datetime.year, 
																			datetime.hour, datetime.minute, datetime.second, datetime.microsecond);
	WTXT("\n");

	WTXT("== eMMC Registers ==\n");
	
	TO_HEX(&info.card_id, sizeof(info.card_id), hex);
	WTXT("Card ID: %s\n", hex);

	TO_HEX(&info.card_descriptor, sizeof(info.card_descriptor), hex);
	WTXT("Card Descriptor: %s\n", hex);

	TO_HEX(&info.extra_card_descriptor, sizeof(info.extra_card_descriptor), hex);
	WTXT("Extended Card Descriptor: %s\n", hex);
	
	WTXT("== General Command (CMD56) ==\n");
	
	TO_HEX(info.key_set.packet18_key, sizeof(info.key_set.packet18_key), hex);
	WTXT("Key18: %s\n", hex);
	
	TO_HEX(info.key_set.packet20_key, sizeof(info.key_set.packet20_key), hex);
	WTXT("Key20: %s\n", hex);
	
	WTXT("Key ID: %s (0x%02X)\n", keyid_to_keygroup(info.key_id), info.key_id);

	WTXT("== eMMC Card ID ==\n");
	WTXT("Crc7: 0x%02X\n", info.card_id.crc7);
	WTXT("Manufactured Date: %u/%u (0x%02X)\n", info.month, info.year, info.card_id.manufacture_date);
	WTXT("Serial Number: 0x%02X\n", info.card_id.serial_number);
	WTXT("Revision: 0x%02X\n", info.card_id.revision);
	WTXT("Extended Revision: 0x%02X\n", info.extra_card_descriptor_revision);
	WTXT("Product Name: %.6s\n", info.card_id.product_name);
	WTXT("Oem ID: 0x%02X\n", info.card_id.oem_id);
	WTXT("Device Type: %s (0x%02X)\n", mmc_device_type_to_string(info.card_id.device_type), info.card_id.device_type);
	WTXT("Vendor: %s (0x%02X)\n", mmc_vendor_id_to_manufacturer(info.card_id.vendor), info.card_id.vendor);

	WTXT("== eMMC Card Descriptor ==\n");
	WTXT("Crc7: 0x%02X\n", info.card_descriptor.crc7);
	WTXT("Error Correction Code: 0x%02X\n", info.card_descriptor.ecc_code);
	WTXT("File Format: 0x%02X\n", info.card_descriptor.file_format);
	WTXT("Temporary Write Protection: %s (0x%02X)\n", SBOOL(info.card_descriptor.tmp_write_prot));
	WTXT("Permanant Write Protection: %s (0x%02X)\n", SBOOL(info.card_descriptor.perm_write_prot));
	WTXT("Is Copy: %s (0x%02X)\n", SBOOL(info.card_descriptor.copy));
	WTXT("File Format Group: 0x%02X\n", info.card_descriptor.file_format_group);
	WTXT("Content Protection Application: 0x%02X\n", info.card_descriptor.content_protection_application);
	WTXT("Partial Write Block Allowed: %s (0x%02X)\n", SBOOL(info.card_descriptor.partial_write_block_allowed));
	WTXT("Max Write Data Block Length: 0x%02X\n", info.card_descriptor.max_write_data_blk_length);
	WTXT("Write Speed Factor: 0x%02X\n", info.card_descriptor.write_speed_factor);
	WTXT("Manufacturer Default Ecc: 0x%02X\n", info.card_descriptor.manufacturer_default_ecc);
	WTXT("Write Protection Group Enable: 0x%02X\n", info.card_descriptor.write_prot_group_enable);
	WTXT("Write Protection Group Size: 0x%02X\n", info.card_descriptor.write_prot_group_size);
	WTXT("Erase Group Size Multipler: 0x%02X\n", info.card_descriptor.erase_group_size_mult);
	WTXT("Erase Group Size: 0x%02X\n", info.card_descriptor.erase_group_size);
	WTXT("Device Size Multiplier: 0x%02X\n", info.card_descriptor.device_size_mult);
	WTXT("Vdd Write Current Max: 0x%02X\n", info.card_descriptor.vdd_w_cur_max);
	WTXT("Vdd Write Current Min: 0x%02X\n", info.card_descriptor.vdd_w_cur_min);
	WTXT("Vdd Read Current Max: 0x%02X\n", info.card_descriptor.vdd_r_cur_max);
	WTXT("Vdd Read Current Min: 0x%02X\n", info.card_descriptor.vdd_r_cur_min);
	WTXT("Device Size: 0x%02X\n", info.card_descriptor.device_size);
	WTXT("Dsr Implemented: %s (0x%02X)\n", SBOOL(info.card_descriptor.dsr_implemented));
	WTXT("Read Block Misalignment: 0x%02X\n", info.card_descriptor.read_blk_misalignment);
	WTXT("Write Block Misalignment: 0x%02X\n", info.card_descriptor.write_blk_misalignment);
	WTXT("Partial Block For Read Allowed: %s (0x%02X)\n", SBOOL(info.card_descriptor.partial_block_for_read_allowed));
	WTXT("Max Read Data Block Length: 0x%02X\n", info.card_descriptor.max_read_data_block_length);
	WTXT("Device Command Classes: 0x%02X\n", info.card_descriptor.device_command_classes);
	WTXT("Max Bus Clock Frequency: 0x%02X\n", info.card_descriptor.max_bus_clk_frequency);
	WTXT("Data Read Access Time In 2 Clock Cycles: 0x%02X\n", info.card_descriptor.data_read_access_time_in_2_clk_cycles);
	WTXT("Data Read Access Time 1: 0x%02X\n", info.card_descriptor.data_read_access_time_1);
	WTXT("System Specification Version: 0x%02X\n", info.card_descriptor.system_specification_version);
	WTXT("Csd Structure: 0x%02X\n", info.card_descriptor.csd_structure);

	
	return rptr - rtxt;
}

int dump_report(const char* output_file) {
	PRINT_FUNC();
	char report[0x1000];
	
	make_directories_excluding_last(output_file);
	size_t report_sz = create_report(report, sizeof(report));
	
	int wr = write_file(output_file, report, report_sz);
	
	if(wr == 0) return SIZE_IS_ZERO;
	if(wr < 0) return wr;
	if(wr != report_sz) return SIZE_NOT_MATCH;

	return 0;
}

int dump_report_network(const char* ip_address, short port, const char* output_file) {
	PRINT_FUNC();
	char report[0x1000];
	int netwr = -1;
	
	size_t report_sz = create_report(report, sizeof(report));
	
	SceUID fd = begin_connection(ip_address, port);
	PRINT_STR("fd = %x\n", fd);
	if(fd < 0) return fd;
	
	int res = begin_file_send(fd, output_file, report_sz);
	if(res < 0) goto error;
	
	netwr = file_send_data(fd, report, report_sz);
	PRINT_STR("netwr = %x (sizeof = %x)\n", netwr, report_sz);

error:
	if(fd >= 0) end_connection(fd);
	if(netwr == 0) return SIZE_IS_ZERO;
	if(netwr != report_sz) return SIZE_NOT_MATCH;
	
	return res;	
}
