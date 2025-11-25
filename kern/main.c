#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include <vitasdkkern.h>
#include <GcToolKit.h>
#include "format.h"
#include "auth_emu.h"
#include "otg.h"
#include "gc.h"
#include "sd2vita.h"


void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args)
{
	otg_patch(); // enable otg
	get_format_functions(); // get format stuff
	get_interupt_location(); // get location of interupt in sdif
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	otg_unpatch(); // stop using otg.
	return SCE_KERNEL_STOP_SUCCESS;
}
