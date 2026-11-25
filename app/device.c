#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vitasdk.h>
#include <GcToolKit.h>

#include "kernel.h"
#include "log.h"
#include "auth.h"
#include "device.h"
#include "fs.h"

#include "mbr.h"
#include "vci.h"
#include "psv.h"

#include "err.h"
#include "net.h"

#include "sha256.h"


const char* BLOCK_DEVICES[][2] = { 
	{"GCD entire", "sdstor0:gcd-lp-ign-entire"}, 
	{"GCD active mediaid", "sdstor0:gcd-lp-act-mediaid"}, 
	{"GCD inactive mediaid", "sdstor0:gcd-lp-ina-mediaid"}, // ???
	{"GCD readonly:", "sdstor0:gcd-lp-ign-gamero"}, 
	{"GCD readwrite:", "sdstor0:gcd-lp-ign-gamerw"}, 

	{"MCD entire", "sdstor0:mcd-lp-act-entire"}, 
	{"MCD active mediaid", "sdstor0:mcd-lp-act-mediaid"}, 
	{"MCD inactive mediaid", "sdstor0:mcd-lp-ina-mediaid"},  // ???
	{"MCD readonly", "sdstor0:mcd-lp-ign-userext"}, 


	{"External(?) NAND", "sdstor0:ext-lp-act-entire"}, 
	{"Internal NAND", "sdstor0:int-lp-act-entire"}, 


	{"Active os0:", "sdstor0:int-lp-act-os"}, 
	{"InActive os0:", "sdstor0:int-lp-ina-os"}, 


	{"Active SLB2", "int-lp-act-sloader"}, 
	{"InActive SLB2", "int-lp-ina-sloader"}, 


	{"vs0:", "sdstor0:int-lp-ign-vsh"}, 
	{"vd0:", "sdstor0:int-lp-ign-vshdata"}, 
	{"tm0:", "sdstor0:int-lp-ign-vtrm"}, 
	{"ur0:", "sdstor0:int-lp-ign-user"}, 
	{"imc0:", "sdstor0:int-lp-ign-userext"}, 
	{"ud0:", "sdstor0:int-lp-ign-updater"}, 
	{"sa0:", "sdstor0:int-lp-ign-sysdata"}, 
	{"pd0:", "sdstor0:int-lp-ign-pidata"}, 


	{"USB storage entire", "sdstor0:uma-lp-act-entire"}, 
	{"USD active entire", "sdstor0:usd-lp-act-entire"},  // // ???
	{"USD inactive entire", "sdstor0:usd-lp-ina-entire"}, // ???
	
	{NULL, NULL}
};



uint8_t device_exist(const char* block_device) {
	if(!is_module_started(KMODULE_NAME)) return KERNEL_MODULE_FAILED_START;
	
	SceUID dfd = kOpenDevice(block_device, SCE_O_RDONLY | SCE_O_RDLOCK);
	if(dfd < 0) return 0;
	
	kCloseDevice(dfd);
	return 1;
}

int get_trimmed_size(const char* block_device, uint64_t* size) {
	PRINT_FUNC();
	if(!is_module_started(KMODULE_NAME)) return KERNEL_MODULE_FAILED_START;
	if(size == NULL) return POINTER_WAS_NULL;
	
	uint64_t device_size = 0;

	int dfd = kOpenDevice(block_device, SCE_O_RDONLY | SCE_O_RDLOCK);
	if(dfd < 0) return dfd;

	SceMbr mbr;
	size_t sz = kReadDevice(dfd, &mbr, sizeof(SceMbr)); 

	// validate MBR
	if(sz == sizeof(SceMbr) && memcmp(mbr.magic, SCE_MBR_MAGIC, sizeof(mbr.magic)) == 0 && mbr.version == 0x03) {
		
		PRINT_STR("mbr.magic = %.32s\n", mbr.magic);
		PRINT_STR("mbr.version = 0x%02X\n", mbr.version);
		PRINT_STR("mbr.device_size = 0x%02X\n", mbr.device_size);
		
		PRINT_STR("mbr.unk1 = ");
		PRINT_BUFFER(mbr.unk1);
		
		// find end partition:
		for(int i = 0; i < (sizeof(mbr.partitions) / sizeof(ScePartiton)); i++) {
			
			PRINT_STR("mbr.partitions[%u].off = 0x%02X\n", i, mbr.partitions[i].off);
			PRINT_STR("mbr.partitions[%u].sz = 0x%02X\n", i, mbr.partitions[i].sz);
			PRINT_STR("mbr.partitions[%u].code = 0x%02X\n", i, mbr.partitions[i].code);
			PRINT_STR("mbr.partitions[%u].type = 0x%02X\n", i, mbr.partitions[i].type);
			PRINT_STR("mbr.partitions[%u].active = 0x%02X\n", i, mbr.partitions[i].active);
			PRINT_STR("mbr.partitions[%u].flags = 0x%02X\n", i, mbr.partitions[i].flags);
			PRINT_STR("mbr.partitions[%u].unk = 0x%02X\n", i, mbr.partitions[i].unk);
			
			if(mbr.partitions[i].code == ScePartitionCode_EMPTY) continue;
			
			uint64_t abs_offset = (mbr.partitions[i].off * SECTOR_SIZE);
			uint64_t abs_size = (mbr.partitions[i].sz * SECTOR_SIZE);
			uint64_t total_sz = abs_offset + abs_size;
			
			if((total_sz) > device_size) device_size = total_sz;
		}
		
		PRINT_STR("mbr.unk2 = ");
		PRINT_BUFFER(mbr.unk2);

		PRINT_STR("mbr.unk3 = ");
		PRINT_BUFFER(mbr.unk3);

		PRINT_STR("mbr.sig = 0x%02X\n", mbr.sig);
	}
	
	kCloseDevice(dfd);

	if(device_size <= 0) *size = device_size;
	else return get_device_size(block_device, size);
	
	return 0;
}

int get_device_size(const char* block_device, uint64_t* size) {
	int res = 0;
	if(!is_module_started(KMODULE_NAME)) return KERNEL_MODULE_FAILED_START;
	if(size == NULL) return POINTER_WAS_NULL;
	
	PRINT_STR("Calling kernel function ...\n");
	res = kGetDeviceSize(block_device, (int64_t*)size);
	PRINT_STR("device_size: %llx ...\n", *size);
	
	return res;
}

int get_header_size(BackupFormat format, uint64_t* size) {
	PRINT_FUNC();
	if(size == NULL) return POINTER_WAS_NULL;
	
	switch(format) {
		case BACKUP_FORMAT_PSV_TRIM:
		case BACKUP_FORMAT_PSV:
			*size = sizeof(PsvHeader);
			return 0;
		case BACKUP_FORMAT_VCI_TRIM:
		case BACKUP_FORMAT_VCI:
			*size = sizeof(VciHeader);
			return 0;
		case BACKUP_FORMAT_RAW:
		default:
			*size = 0;
			return 0;	
	}
}

int get_effective_size(const char* block_device, BackupFormat format, uint64_t* size) {
	PRINT_STR("block_device: %s format: %x\n", block_device, format);
	
	uint64_t header_size = 0;
	int res = get_header_size(format, &header_size);
	if(res < 0) return res;
	
	if(size == NULL) return POINTER_WAS_NULL;
	
	switch(format) {
		case BACKUP_FORMAT_VCI:
		case BACKUP_FORMAT_PSV:
			PRINT_STR("BACKUP_FORMAT_PSV / BACKUP_FORMAT_VCI\n");
			res = get_device_size(block_device, size);
			*size += header_size;
			break;
		case BACKUP_FORMAT_PSV_TRIM:
		case BACKUP_FORMAT_VCI_TRIM:
			PRINT_STR("BACKUP_FORMAT_PSV_TRIM / BACKUP_FORMAT_VCI_TRIM\n");
			res = get_trimmed_size(block_device, size);
			*size += header_size;
			break;
		case BACKUP_FORMAT_RAW:
		default:
			PRINT_STR("BACKUP_FORMAT_RAW\n");
			res = get_device_size(block_device, size);
			break;
	}
	PRINT_STR("size: %llx\n", *size);
	return res;
}

