#ifndef LOG_H
#define LOG_H 1

#include <vitasdkkern.h>

#ifdef ENABLE_LOGGING
#define PRINT_STR(...) do { ksceDebugPrintf("[GcKernKit] %s: ", __FUNCTION__); ksceDebugPrintf(__VA_ARGS__); } while(0);
#define PRINT_BUFFER(buffer) for(int i = 0; i < sizeof(buffer); i++) { \
									ksceDebugPrintf("%02X ", (unsigned char)(buffer[i]));	\
							 } \
							 ksceDebugPrintf("\n")
#else
#define PRINT_STR(...) /**/
#define PRINT_BUFFER(buffer) /**/
#endif

#endif // LOG_H