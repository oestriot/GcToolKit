#include <GcKernKit.h>
#include <vitasdk.h>
#include "log.h"
#include "lock.h"
#include "kernel.h"
#include "io.h"

static uint8_t SD2VITA_WAS_USED = 0;

int check_using_sd2vita() {
	if(check_loaded_blacklisted_module() == NULL) return 0;
	
	if(kIsSdInserted()) {
		SD2VITA_WAS_USED = 1;
		return 1;
	}
	
	return 0;
}

void wait_for_sd2vita_eject() {
	while(check_using_sd2vita()) {
		sceKernelDelayThread(1000 * 30); // 10ms
	}
}


void undo_sd2vita_patches() {
	if(check_loaded_blacklisted_module() != NULL) { // check for YAMT, StorageMGR, etc.

		// undo sd2vita patches 
		PRINT_STR("undoing sd2vita patches\n");
		kUndoSd2Vita();
		
		if(SD2VITA_WAS_USED) { // lock home if sd2vita was used. (requires reboot to fix configuration.)
			PRINT_STR("locking home button...\n");
			lock_home();
			PRINT_STR("unmounting ux0...\n");
			umount_ux0();
		}
	}
}