#ifndef GC_TOOL_KIT_H
#define GC_TOOL_KIT_H 1

#define MAX_PATH (0x512)
#define BUFFER_SIZE (0x1028)

#include "GcKernKit.h"
#include "Error.h"
#include "Packet.h"

#define DEVICE_WHITELIST_CHECK(dev) do { if(memcmp(dev, "sdstor0:gcd", 11) != 0) return DEVICE_WHITELIST_FAILED; } while(0)

#endif // GC_TOOL_KIT_H