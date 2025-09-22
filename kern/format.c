/*
 * psp2format
 * Copyright (C) 2021, Princess of Sleeping
 */

#include <vitasdkkern.h>
#include <taihen.h>
#include "mod.h"
#include "format.h"
#include "GcKernKit.h"

#define ERROR(x) return x
#define SAFE_CHK(dev) if(memcmp(dev, "sdstor0:gcd", 11) != 0 && memcmp(dev, "sdstor0:uma", 11) != 0) ERROR(BRICK_PREVENTED_DEVICE_WHITELIST)
	
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

typedef struct SceFatFormatParam { // size is 0x30-bytes
	SceUInt64 data_0x00;
	const char *path;
	void *pWorkingBuffer;
	SceSize workingBufferSize;
	SceSize bytePerCluster;
	SceSize bytePerSector;
	SceUInt32 data_0x1C;       // Unknown. Cleared by internal.
	SceUInt32 fat_time;
	SceUInt32 data_0x24;       // Unknown. Must be zero.
	SceUInt32 processing_state;
	SceUInt32 sce_fs_type;
} SceFatFormatParam;

#define SCE_FAT_FORMAT_TYPE_FAT12 (1)
#define SCE_FAT_FORMAT_TYPE_FAT16 (2)
#define SCE_FAT_FORMAT_TYPE_FAT32 (3)
#define SCE_FAT_FORMAT_TYPE_EXFAT (4)

int ksceRtcGetCurrentClockLocalTime(SceDateTime *time);

SceUInt32 (* sceAppMgrMakeFatTime)(const SceDateTime *time);
int (* sceAppMgrExecFsFatFormat)(SceFatFormatParam *pParam);

int FatfsExecFormatInternal(const char *s, void *pWorkingBuffer, SceSize workingBufferSize, SceSize bytePerCluster, SceUInt32 sce_fs_type){

	int res, old;
	SceDateTime time;
	SceFatFormatParam ff_param;

	res = ksceKernelSetPermission(0x80);
	if(res < 0){
		return res;
	}

	old = res;

	memset(&time, 0, sizeof(time));
	res = ksceRtcGetCurrentClockLocalTime(&time);
	if(res < 0){
		goto error;
	}

	memset(pWorkingBuffer, 0, workingBufferSize);
	memset(&ff_param, 0, sizeof(ff_param));
	ff_param.data_0x00         = 0LL;
	ff_param.path              = s;
	ff_param.pWorkingBuffer    = pWorkingBuffer;
	ff_param.workingBufferSize = workingBufferSize;
	ff_param.bytePerCluster    = bytePerCluster;
	ff_param.bytePerSector     = 0;
	ff_param.data_0x1C         = 0;
	ff_param.fat_time          = sceAppMgrMakeFatTime(&time);
	ff_param.data_0x24         = 0;
	ff_param.processing_state  = 0;
	ff_param.sce_fs_type       = sce_fs_type;

	if(bytePerCluster == 0){
		if(sce_fs_type == SCE_FAT_FORMAT_TYPE_EXFAT){
			ff_param.bytePerCluster = 0x20000;
		}else if(sce_fs_type == SCE_FAT_FORMAT_TYPE_FAT32){
			ff_param.bytePerCluster = 0x8000;
		}else if(sce_fs_type == SCE_FAT_FORMAT_TYPE_FAT16 || sce_fs_type == SCE_FAT_FORMAT_TYPE_FAT12){
			ff_param.bytePerCluster = 0x1000;
		}
	}

	res = sceAppMgrExecFsFatFormat(&ff_param);
	if(res >= 0)
		res = ksceIoSync(s, 0);

error:
	ksceKernelSetPermission(old);

	return res;
}

int FatfsExecFormat(const char *s, SceSize bytePerCluster, SceUInt32 sce_fs_type) {

	int res;
	SceUID memid;
	void *pWorkingBase;

	res = ksceKernelAllocMemBlock("SceFatfsFormatWorkingMemBlock", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x40000, NULL);
	if(res < 0){
		return res;
	}

	memid = res;

	res = ksceKernelGetMemBlockBase(memid, &pWorkingBase);
	if(res >= 0)
		res = FatfsExecFormatInternal(s, pWorkingBase, 0x40000, bytePerCluster, sce_fs_type);

	ksceKernelFreeMemBlock(memid);

	return res;
}


int kFormatDevice(const char* device) {
	int state = 0;
	ENTER_SYSCALL(state);
	static char k_device[1028];
	ksceKernelStrncpyUserToKernel(k_device, (const void*)device, sizeof(k_device));
	
	SAFE_CHK(k_device);
	
	int res = FatfsExecFormat(k_device, 0x8000, SCE_FAT_FORMAT_TYPE_EXFAT);
	EXIT_SYSCALL(state);
	return res;
}



void get_module_functions() {
	SceUID module_id;

	module_id = ksceKernelSearchModuleByName("SceAppMgr");
	int appmgr_version = check_module_version("os0:/kd/bootimage.skprx");
	
	if(module_id > 0){
		if(appmgr_version >= 0x363) {
			module_get_offset(KERNEL_PID, module_id, 0, 0x39B40 | 1, (uintptr_t*)&sceAppMgrMakeFatTime);
			module_get_offset(KERNEL_PID, module_id, 0, 0x3E224 | 1, (uintptr_t*)&sceAppMgrExecFsFatFormat);
		}
		else {
			module_get_offset(KERNEL_PID, module_id, 0, 0x39B54 | 1, (uintptr_t*)&sceAppMgrMakeFatTime);
			module_get_offset(KERNEL_PID, module_id, 0, 0x3E238 | 1, (uintptr_t*)&sceAppMgrExecFsFatFormat);			
		}
	}	
}
