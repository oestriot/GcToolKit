#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "err.h"
#include "log.h"
#include "kernel.h"
#include "io.h"

// the kernel module "GcKernKit" will be attempted to start from the following locations:
// (attempted in the order their listed.)
static const char* LOAD_LOCATIONS[] = { 
	// memory card
	"ux0:/patch",
	"ux0:/app",
	
	// game cartridge
	"grw0:/patch",
	"gro0:/app",

	// devkit
	"host0:/patch",
	"host0:/app",
	"sd0:/patch"
	"sd0:/app",
	
	// homebrew
	"ur0:/patch",
	"ur0:/app",
	"xmc0:/patch",
	"xmc0:/app",
	"imc0:/patch",			
	"imc0:/app",
	"uma0:/patch",
	"uma0:/app",
	
	// system
	"pd0:/app",
	"vs0:/app",
	
	NULL
};

static const char* module_blacklist[] = {
	"yamtKernel",
	"yamtHelper",
	"yamtUser", 
	
	"StorageMgrKernel", // VitaStorageMgr.skprx
	"VitaShellKernel", // gamesd.skprx (seriously?)
	NULL
};
		
int is_module_started(const char* module_name) {
	char buffer[0x8];
	memset(buffer, 0x00, sizeof(buffer));
	
	SceUID uid = _vshKernelSearchModuleByName(module_name, buffer);
	
	return (uid > 0);
}

int try_load(const char* install_path) {
	char kplugin_path[MAX_PATH];
	char titleid[12];
	
	memset(titleid, 0x00, sizeof(titleid));
	memset(kplugin_path, 0x00, sizeof(kplugin_path));
	
	sceAppMgrAppParamGetString(0, 12, titleid , 256);

	snprintf(kplugin_path, sizeof(kplugin_path)-1, "%s/%s/%s.skprx", install_path, titleid, KMODULE_NAME);
	PRINT_STR("trying to load kernel module: %s(%s)\n", __FUNCTION__, kplugin_path);
	SceUID uid = taiLoadStartKernelModule(kplugin_path, 0, NULL, 0);
	
	return (uid > 0);
}

const char* check_loaded_blacklisted_module() {
	
	PRINT_STR("Checking for blacklisted modules\n");
	for(int i = 0; module_blacklist[i] != NULL; i++) {
		PRINT_STR("Checking: %s\n", module_blacklist[i]);
		
		if(is_module_started(module_blacklist[i])) {
			PRINT_STR("Found!\n");
			return module_blacklist[i];
		}
	}
	
	return NULL;
}

int load_kernel_modules() {
	PRINT_STR("Checking kernel module ...\n");
	if(!is_module_started(KMODULE_NAME)) {
		PRINT_STR("Attempting to load kernel module ...\n")
		
		// try load GcKernKit from all load locations.
		for(int i = 0; LOAD_LOCATIONS[i] != NULL; i++) {
			if(try_load(LOAD_LOCATIONS[i])) return 1;
		}
		
	}
	else {
		return 1;
	}
	
	return 0;
}

void restart_application() {
	PRINT_STR("Restarting application ...\n");
	char* argv[2] = {"-restarted", NULL};
	sceAppMgrLoadExec(EBOOT_PATH, argv, NULL);	
}

int check_module_version(const char* module) {
	PRINT_STR("Checking version of module: %s\n", module);
	
	int version = 0x000;
	
	SceUID fd = sceIoOpen(module, SCE_O_RDONLY, 0777);
	if(fd > 0) {
		sceIoLseek(fd, 0x94, SCE_SEEK_SET);
		sceIoRead(fd, &version, sizeof(version));
		sceIoClose(fd);
		
		PRINT_STR("%s version: %x\n", module, version);
		return version;
	}
	PRINT_STR("Failed to read fw version; falling back on default\n");
	return MIN_FW_VERSION;
}

int get_real_fw_version() {
	return check_module_version("os0:/kd/bootimage.skprx");
}