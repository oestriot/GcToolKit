#include <vitasdk.h>
#include <stdio.h>
#include <stdint.h>
#include "device.h"
#include "err.h"
#include "kernel.h"

static uint8_t disable_power = 0;
static uint8_t flag_lock_gc = 0;

/* power lock thread */

int power_tick_thread(size_t args, void* argp) {
	disable_power = 1;
	while(disable_power){
		// constantly tick power so that console doesn't fall asleep
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
		sceKernelDeleteThread(1000 * 1000 * 1); // 1s		
	}
	return 0;
}

void unlock_power() {
	disable_power = 0;
}

int lock_power() {
	int res = 0;
	SceUID thread_id = sceKernelCreateThread("PowerTickThread", power_tick_thread, 0x10000100, 0x1000, 0, 0, NULL);
	if(thread_id < 0) ERROR(thread_id);

	res = sceKernelStartThread(thread_id, 0, NULL);
	
error:
	if(thread_id < 0)
		sceKernelDeleteThread(thread_id);
	return res;
}


/* gc lock thread */

int gc_lock_thread(size_t args, void* argp) {
	
	flag_lock_gc = 1;
	while(flag_lock_gc){
		// check if the gc is still inserted
		if(!device_exist(BLOCK_DEVICE_GC)) {
			// if it isn't; then restart the application ???
			// to be honest im not sure this is much better than just spamming the cancel button.
			sceAppMgrLoadExec(EBOOT_PATH, NULL, NULL);
			flag_lock_gc = 0;
		}
		sceKernelDeleteThread(1000 * 300); // 300ms
	}
	return 0;
}


void unlock_gc() {
	flag_lock_gc = 0;
}

int lock_gc() {
#ifdef ENABLE_GC_LOCK
	int res = 0;
	SceUID thread_id = sceKernelCreateThread("GcLockThread", gc_lock_thread, 0x10000100, 0x1000, 0, 0, NULL);
	if(thread_id < 0) ERROR(thread_id);

	res = sceKernelStartThread(thread_id, 0, NULL);
	
error:
	if(thread_id < 0)
		sceKernelDeleteThread(thread_id);
	return res;
#else
	return 0;
#endif
}

/* Default locks */

void lock_shell() {
	sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU |
					 SCE_SHELL_UTIL_LOCK_TYPE_POWEROFF_MENU |
					 SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION |
					 SCE_SHELL_UTIL_LOCK_TYPE_MUSIC_PLAYER |
					 SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
}

void unlock_shell() {
	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU |
					   SCE_SHELL_UTIL_LOCK_TYPE_POWEROFF_MENU |
					   SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION |
					   SCE_SHELL_UTIL_LOCK_TYPE_MUSIC_PLAYER |
					   SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
	
}

// disable memory card remove/insert prompt everywhere;
void init_shell() { 
	sceShellUtilInitEvents(0);
	sceShellUtilLock(
		SCE_SHELL_UTIL_LOCK_TYPE_MC_INSERTED |
		SCE_SHELL_UTIL_LOCK_TYPE_MC_REMOVED
	);
}

void term_shell() {
	sceShellUtilUnlock(
		SCE_SHELL_UTIL_LOCK_TYPE_MC_INSERTED |
		SCE_SHELL_UTIL_LOCK_TYPE_MC_REMOVED
	);
}