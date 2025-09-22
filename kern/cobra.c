#include <taihen.h>
#include <stdint.h>
#include <stdlib.h>
#include <vitasdkkern.h>
#include "log.h"

// bypass blackfin mitigation
static int sceKernelGetSystemTimeWideHook = -1;
static tai_hook_ref_t sceKernelGetSystemTimeWideHookRef;

uint64_t return_0() {
	PRINT_STR("res 0\n");
	return 0;
}


void cobra_patch() {
	// undo cobra blackfin patch
	sceKernelGetSystemTimeWideHook = taiHookFunctionImportForKernel(KERNEL_PID,
		&sceKernelGetSystemTimeWideHookRef, 
		"SceSblGcAuthMgr",
		0xE2C40624, // SceThreadmgrForDriver
		0xF4EE4FA9, // sceKernelGetSystemTimeWide
		return_0);
	
	
	PRINT_STR("sceKernelGetSystemTimeWideHook 0x%04X\n", sceKernelGetSystemTimeWideHook);
	PRINT_STR("sceKernelGetSystemTimeWideHookRef 0x%04X\n", sceKernelGetSystemTimeWideHookRef);
	
}
void cobra_unpatch() {
	if (sceKernelGetSystemTimeWideHook >= 0) taiHookReleaseForKernel(sceKernelGetSystemTimeWideHook, sceKernelGetSystemTimeWideHookRef);
}