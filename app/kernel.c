#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "err.h"
#include "log.h"
#include "kernel.h"

// the kernel module "GcKernKit" will be attempted to start from the following locations:
// (attempted in the order their listed.)
static const char* load_locations[] = { 
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
		
int kernel_started() {
	char buffer[0x8];
	memset(buffer, 0x00, sizeof(buffer));
	
	SceUID uid = _vshKernelSearchModuleByName(KMODULE_NAME, buffer);
	PRINT_STR("_vshKernelSearchModuleByName = %x\n", uid);
	
	return (uid > 0);
}

int try_load(const char* install_path) {
	char kplugin_path[0x1028];
	char titleid[12];
	
	memset(titleid, 0x00, sizeof(titleid));
	memset(kplugin_path, 0x00, sizeof(kplugin_path));
	
	sceAppMgrAppParamGetString(0, 12, titleid , 256);

	snprintf(kplugin_path, sizeof(kplugin_path)-1, "%s/%s/%s.skprx", install_path, titleid, KMODULE_NAME);
	SceUID uid = taiLoadStartKernelModule(kplugin_path, 0, NULL, 0);
	PRINT_STR("%s(%s) = %x\n", __FUNCTION__, kplugin_path, uid);
	
	return (uid > 0);
}

void load_kernel_modules() {
	if(!kernel_started()) {
		
		// try load GcKernKit from all load locations.
		for(int i = 0; load_locations[i] != NULL; i++) {
			if(try_load(load_locations[i])) break;
		}
		
		// restart this application		
		char* argv[2] = {"-restarted", NULL};
		sceAppMgrLoadExec(EBOOT_PATH, argv, NULL);
	}
	
}
