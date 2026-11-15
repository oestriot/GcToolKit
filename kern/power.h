#ifndef POWER_H
#define POWER_H
#include <vitasdkkern.h>
#include <stdint.h>

// todo: remove when vitasdk gets updated.
int kscePowerSetCallbackState(SceUInt32 enable);

int kPowerSetCallbackState(uint32_t enable);


#endif