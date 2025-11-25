// This is a hack to try get around issues with YAMT, and other SD2Vita plugins;

#include <vitasdkkern.h>
#include <taihen.h>

#include "module.h"
#include "sd2vita.h"
#include "log.h"
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;
static undone_sd2vita_patch = 0;

int fix_up_mountpoint(int id, const char* new_dev, const char* new_blkdev) {
	SceIoMountPoint* part = sceIoFindMountPoint(id);
	
	PRINT_STR("fixup_mountpoint(0x%x, %s)\n", id, new_blkdev);
	if(part != NULL) {
		// fix part->dev
		if(part->dev != NULL) {
			if(part->dev->blkdev == NULL || strcmp(part->dev->blkdev, new_blkdev) != 0) {
				PRINT_STR("Setting part->dev->blkdev %s\n", new_blkdev);
				DACR_OFF(part->dev->blkdev = new_blkdev);
			}
			if(part->dev->dev == NULL || strcmp(part->dev->dev, new_dev) != 0) {
				PRINT_STR("Setting part->dev->dev %s\n", new_dev);
				DACR_OFF(part->dev->dev = new_dev);
			}
		}
		
		// fix part->dev2
		if(part->dev2 != NULL) {
			if(part->dev2->blkdev == NULL || strcmp(part->dev2->blkdev, new_blkdev) != 0) {
				PRINT_STR("Setting part->dev2->blkdev %s\n", new_blkdev);
				DACR_OFF(part->dev2->blkdev = new_blkdev);
			}
			if(part->dev2->dev == NULL || strcmp(part->dev2->dev, new_dev) != 0) {
				PRINT_STR("Setting part->dev->dev %s\n", new_dev);
				DACR_OFF(part->dev2->dev = new_dev);
			}
		}
	}
	
	
	return 0;
}


int kUndoneSd2VitaPatches() {
	return undone_sd2vita_patch;
}

void kUndoSd2Vita() {
	if(undone_sd2vita_patch) return;
	undone_sd2vita_patch = 1;
	
	// get functions from sceIoFileMgr.
	SceUID module_id = ksceKernelSearchModuleByName("SceIofilemgr");
	int iofilemgr_version = check_module_version("os0:/kd/iofilemgr.skprx");
	
	
	if(module_id > 0){
		if(iofilemgr_version >= 0x363) {
			module_get_offset(KERNEL_PID, module_id, 0, 0x182f4 | 1, (uintptr_t*)&sceIoFindMountPoint);
		}
		else {
			module_get_offset(KERNEL_PID, module_id, 0, 0x138c0 | 1, (uintptr_t*)&sceIoFindMountPoint);
		}
	}
	
	// revert YAMT sd patches
	module_id = ksceKernelSearchModuleByName("SceSysmem");
	int sysmem_verison = check_module_version("os0:/kd/iofilemgr.skprx");
	
	if(module_id > 0) {
		if(sysmem_verison >= 0x360) {
			uint8_t original_data[2] = { 0x00, 0x20 };
			SceUID inject = taiInjectDataForKernel(KERNEL_PID, module_id, 0, 0x21610, original_data, sizeof(original_data));
			PRINT_STR("inject: %x\n", inject);
		}
	}
	
	// fix YAMT redirection
	PRINT_STR("Fixing mountpoints\n");
	fix_up_mountpoint(0x900, "gro0:", "sdstor0:gcd-lp-ign-gamero");
	fix_up_mountpoint(0xA00, "grw0:", "sdstor0:gcd-lp-ign-gamerw");
	fix_up_mountpoint(0xD00, "imc0:", "sdstor0:int-lp-ign-userext");
	fix_up_mountpoint(0xE00, "xmc0:", "sdstor0:xmc-lp-ign-userext");
	fix_up_mountpoint(0xF00, "uma0:", "sdstor0:uma-lp-act-entire");
	
}
