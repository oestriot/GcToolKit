#include <sys/time.h>
#include <string.h>
#include <vitasdk.h>

#include "device.h"
#include "io.h"

static unsigned int buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};
	
unsigned int SCE_CTRL_CONFIRM = SCE_CTRL_CROSS;
unsigned int SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;

// check what button is set as confirm (swap O/X for japanese)
void init_ctrl() {
	
	int keySwap = 0;
	int res = sceRegMgrGetKeyInt("/CONFIG/SYSTEM", "button_assign", &keySwap);
	if(res < 0) return;
	
	if(keySwap == 0) {
		SCE_CTRL_CONFIRM = SCE_CTRL_CIRCLE;
		SCE_CTRL_CANCEL = SCE_CTRL_CROSS;		
	}
	else {
		SCE_CTRL_CONFIRM = SCE_CTRL_CROSS;
		SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;		
	}
}

int get_key() {	
	unsigned int prev = 0;
	SceCtrlData pad;

	while (1) {
		memset(&pad, 0, sizeof(pad));
		
		// this is a stupid hack, i hate this ...
		// if theres no gamecart inserted, just press cancel everywhere.
		if(!device_exist(BLOCK_DEVICE_GC)) return SCE_CTRL_CANCEL; 
		
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned int new_buttons = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (int i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i) {
			if (new_buttons & buttons[i]) {
				return buttons[i];	
			}				
		}

		sceKernelDelayThread(1000); // 1ms
	}
}