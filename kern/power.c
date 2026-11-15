#include <vitasdkkern.h>
#include "power.h"


int kPowerSetCallbackState(int enable) {
	return kscePowerSetCallbackState(enable);	
}