#ifndef GC_TOOL_KIT_H
#define GC_TOOL_KIT_H 1

#define MAX_PATH (0x512)
#define BUFFER_SIZE (0x1028)

#include "GcKernKit.h"
#include "Error.h"
#include "Packet.h"

// WARN:
// only allow wiping of gamecart(gcd) device and not any others;
// this is a sanity check to avoid completely bricking the device
// in theroy- the menu does not allow you to select any other device to begin with
// but you can never be too sure ..
#define DEVICE_WHITELIST_CHECK(dev) do { if(memcmp(dev, "sdstor0:gcd", 11) != 0) return DEVICE_WHITELIST_FAILED; } while(0)

#endif // GC_TOOL_KIT_H