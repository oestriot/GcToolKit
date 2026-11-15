#include <vitasdkkern.h>
#include <stdint.h>

#include "log.h"
#include "power.h"


int kPowerSetCallbackState(uint32_t enable) {
	PRINT_STR("kscePowerSetCallbackState(%i);\n", enable);
	return kscePowerSetCallbackState(enable);	
}