#include "log.h"
#include <vitasdkkern.h>
#include <taihen.h>
#include <stdint.h>

SceUID sceSysconSetOtgPowerLevelHook = -1;
static tai_hook_ref_t sceSysconSetOtgPowerLevelHookRef;



static int return_1() {
	return 1;
}

// shamelessly stolen from dots_tb
static SceUID sceSysconSetOtgPowerLevel_patched(uint32_t *pwr_val) {
	SceUID res, state;
	ENTER_SYSCALL(state);
	
	res = TAI_CONTINUE(SceUID, sceSysconSetOtgPowerLevelHookRef, pwr_val);
	PRINT_STR("sceSysconSetOtgPowerLevel original %x\n", *pwr_val);
	if(*pwr_val == 0x700)
		PRINT_STR("sceSysconSetOtgPowerLevel new %x\n\n", *pwr_val = 0x200);
	
	EXIT_SYSCALL(state);
	return res;
	
}

// force load usb mass storage plugin on all devices
int load_umass() {
	if(ksceKernelSearchModuleByName("SceUsbMass") < 0) {
		tai_hook_ref_t ksceSysrootIsSafeModeHookRef;
		tai_hook_ref_t ksceSblAimgrIsDolceHookRef;

		// temporarily patch isSafeMode and isDolce
		SceUID ksceSysrootIsSafeModeHook = taiHookFunctionExportForKernel(KERNEL_PID, 
			&ksceSysrootIsSafeModeHookRef, 
			"SceSysmem", 
			0x2ED7F97A, // SceSysrootForKernel
			0x834439A7, // ksceSysrootIsSafemode 
			return_1); 

		PRINT_STR("ksceSysrootIsSafeModeHook 0x%04X\n", ksceSysrootIsSafeModeHook);
		PRINT_STR("ksceSysrootIsSafeModeHookRef 0x%04X\n", ksceSysrootIsSafeModeHookRef);


		SceUID ksceSblAimgrIsDolceHook = taiHookFunctionExportForKernel(KERNEL_PID, 
			&ksceSblAimgrIsDolceHookRef, 
			"SceSysmem", 
			0xFD00C69A, // SceSblAIMgrForDriver
			0x71608CA3, // ksceSblAimgrIsDolce 
			return_1);
			
		PRINT_STR("ksceSblAimgrIsDolceHook 0x%04X\n", ksceSblAimgrIsDolceHook);
		PRINT_STR("ksceSblAimgrIsDolceHookRef 0x%04X\n", ksceSblAimgrIsDolceHookRef);
		
		// load from the bootimage
		SceUID umass_modid = ksceKernelLoadStartModule("ux0:VitaShell/module/umass.skprx", 0, NULL, 0, NULL, NULL);
		PRINT_STR("Load umass.skprx 0x%04X\n", umass_modid);
				
		// release hooks
		if(ksceSysrootIsSafeModeHook > 0) taiHookReleaseForKernel(ksceSysrootIsSafeModeHook, ksceSysrootIsSafeModeHookRef);
		if(ksceSblAimgrIsDolceHook > 0) taiHookReleaseForKernel(ksceSblAimgrIsDolceHook, ksceSblAimgrIsDolceHookRef);
		if(umass_modid < 0) return umass_modid;
	}
	else{
		PRINT_STR("umass.skprx already running\n");
	}
	
	return 0;
}

int otg_patch() {
	// improve compatibility with OTG connectors
	sceSysconSetOtgPowerLevelHook = taiHookFunctionImportForKernel(KERNEL_PID,
		&sceSysconSetOtgPowerLevelHookRef,
		"SceUsbServ",
		TAI_ANY_LIBRARY,
		0xD6F6D472, // sceSysconSetOtgPowerLevel
		sceSysconSetOtgPowerLevel_patched);

	PRINT_STR("sceSysconSetOtgPowerLevelHook 0x%04X\n", sceSysconSetOtgPowerLevelHook);
	PRINT_STR("sceSysconSetOtgPowerLevelHookRef 0x%04X\n", sceSysconSetOtgPowerLevelHookRef);

	// allow loading usb storage on regular vita
	load_umass();

	return 0;
}

int otg_unpatch() {
	if(sceSysconSetOtgPowerLevelHook >= 0) taiHookReleaseForKernel(sceSysconSetOtgPowerLevelHook, sceSysconSetOtgPowerLevelHookRef);
	return 0;
}