#include "log.h"
#include <vitasdkkern.h>
#include <taihen.h>
#include <stdint.h>

static SceUID ksceSysconGetMicroUsbInfoHook = -1;
static tai_hook_ref_t ksceSysconGetMicroUsbInfoRef;

static SceUID ksceSysrootIsSafeModeHook = -1;
static tai_hook_ref_t ksceSysrootIsSafeModeRef;


static int return_1() {
	return 1;
}

// shamelessly stolen from dots_tb
static SceUID ksceSysconGetMicroUsbInfo_patched(uint32_t *pwr_val) {
	SceUID res, state;
	ENTER_SYSCALL(state);
	
	res = TAI_CONTINUE(SceUID, ksceSysconGetMicroUsbInfoRef, pwr_val);
	PRINT_STR("sceSysconSetOtgPowerLevel original %x\n", *pwr_val);
	if(*pwr_val == 0x700) {
		*pwr_val = 0x200;
		PRINT_STR("changing power value\n");		
	}
	
	EXIT_SYSCALL(state);
	return res;
}

// force load usb mass storage plugin on all devices
int load_umass() {
	if(ksceKernelSearchModuleByName("SceUsbMass") < 0) {
		tai_hook_ref_t tmpHookRef;

		// temporarily patch isSafeMode and isDolce
		SceUID tmpHook = taiHookFunctionExportForKernel(KERNEL_PID, 
			&tmpHookRef, 
			"SceSysmem", 
			0x2ED7F97A, // SceSysrootForKernel
			0x834439A7, // ksceSysrootIsSafemode 
			return_1); 

		PRINT_STR("tmpHook 0x%04X\n", tmpHook);
		PRINT_STR("tmpHookRef 0x%04X\n", tmpHookRef);

		tai_hook_ref_t tmpHook2Ref;

		SceUID tmpHook2 = taiHookFunctionExportForKernel(KERNEL_PID, 
			&tmpHook2Ref, 
			"SceSysmem", 
			0xFD00C69A, // SceSblAIMgrForDriver
			0x71608CA3, // ksceSblAimgrIsDolce 
			return_1);
			
		PRINT_STR("tmpHook2 0x%04X\n", tmpHook2);
		PRINT_STR("tmpHook2Ref 0x%04X\n", tmpHook2Ref);
		
		// TODO: load from the bootimage
		SceUID umass_modid = ksceKernelLoadStartModule("ux0:VitaShell/module/umass.skprx", 0, NULL, 0, NULL, NULL);
		PRINT_STR("Load umass.skprx 0x%04X\n", umass_modid);
				
		// release hooks
		if(tmpHook > 0) taiHookReleaseForKernel(tmpHook, tmpHookRef);
		if(tmpHook2 > 0) taiHookReleaseForKernel(tmpHook2, tmpHook2Ref);
		if(umass_modid < 0) return umass_modid;
	}
	else{
		PRINT_STR("umass.skprx already running\n");
	}

	
	return 0;
}

int otg_patch() {
		
	// enable mounting usb drives
	ksceSysrootIsSafeModeHook = taiHookFunctionImportForKernel(KERNEL_PID, 
		&ksceSysrootIsSafeModeRef, "SceUsbServ",
		0x2ED7F97A, // SceSysrootForKernel
		0x834439A7, // ksceSysrootIsSafemode
		return_1);
																
	PRINT_STR("ksceSysrootIsSafeModeHook 0x%04X\n", ksceSysrootIsSafeModeHook);
	PRINT_STR("ksceSysrootIsSafeModeRef 0x%04X\n", ksceSysrootIsSafeModeRef);
	
	// improve compatibility with OTG connectors
	ksceSysconGetMicroUsbInfoHook = taiHookFunctionImportForKernel(KERNEL_PID,
		&ksceSysconGetMicroUsbInfoRef,
		"SceUsbServ",
		0x60A35F64, // SceSysconForDriver 
		0xD6F6D472, // ksceSysconGetMicroUsbInfo
		ksceSysconGetMicroUsbInfo_patched);

	PRINT_STR("ksceSysconGetMicroUsbInfoHook 0x%04X\n", ksceSysconGetMicroUsbInfoHook);
	PRINT_STR("ksceSysconGetMicroUsbInfoRef 0x%04X\n", ksceSysconGetMicroUsbInfoRef);

	// allow loading usb storage on regular vita
	load_umass();

	return 0;
}

int otg_unpatch() {
	if(ksceSysconGetMicroUsbInfoHook >= 0) 
		taiHookReleaseForKernel(ksceSysconGetMicroUsbInfoHook, ksceSysconGetMicroUsbInfoRef);
	
	if(ksceSysrootIsSafeModeHook >= 0) 
		taiHookReleaseForKernel(ksceSysrootIsSafeModeHook, ksceSysrootIsSafeModeRef);
	
	return 0;
}