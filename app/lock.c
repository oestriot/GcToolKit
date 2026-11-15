#include <vitasdk.h>
#include <stdio.h>
#include <stdint.h>
#include "device.h"
#include "err.h"
#include "kernel.h"

static uint8_t DISABLE_POWER = 0;
static uint8_t DISABLE_HOME = 0;

/* power lock thread */

int power_tick_thread(size_t args, void* argp) {
	DISABLE_POWER = 1;
	while(DISABLE_POWER){
		// constantly tick power so that console doesn't fall asleep
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
		sceKernelDeleteThread(1000 * 1000 * 1); // 1s		
	}
	return 0;
}

void unlock_power() {
	kPowerSetCallbackState(0xFFFFFFFF);
	DISABLE_POWER = 0;
}

int lock_power() {
	int res = 0;
	kPowerSetCallbackState(0);

	SceUID thread_id = sceKernelCreateThread("PowerTickThread", power_tick_thread, 0x10000100, 0x1000, 0, 0, NULL);
	if(thread_id < 0) ERROR(thread_id);

	res = sceKernelStartThread(thread_id, 0, NULL);
error:
	if(thread_id < 0)
		sceKernelDeleteThread(thread_id);
	return res;
}



/* Default locks */
void lock_home() {
	DISABLE_HOME = 1;
	sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
}

void unlock_home() {
	DISABLE_HOME = 0;
	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
}

void lock_shell() {
	sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU |
					 SCE_SHELL_UTIL_LOCK_TYPE_POWEROFF_MENU |
					 SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION |
					 SCE_SHELL_UTIL_LOCK_TYPE_MUSIC_PLAYER |
					 SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
	lock_power();
}

void unlock_shell() {
	SceShellUtilLockType locks = SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU |
							     SCE_SHELL_UTIL_LOCK_TYPE_POWEROFF_MENU |
							     SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION |
							     SCE_SHELL_UTIL_LOCK_TYPE_MUSIC_PLAYER;
								 
	if(!DISABLE_HOME) locks |= SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN;
	
	sceShellUtilUnlock(locks);
	unlock_power();
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