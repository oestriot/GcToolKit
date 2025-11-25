#ifndef LOG_H
#define LOG_H 1

#include <vitasdkkern.h>
#include <stdint.h>

#ifdef ENABLE_LOGGING
#define PRINT_STR(...) do { ksceDebugPrintf("[GcKernKit] %s: ", __FUNCTION__); ksceDebugPrintf(__VA_ARGS__); } while(0);
#define PRINT_BUFFER(buffer) PRINT_BUFFER_LEN(buffer, sizeof(buffer))
#define PRINT_BUFFER_LEN(buffer, len) for(int i = 0; i < len; i++) { \
									ksceDebugPrintf("%02X ", ((unsigned char*)buffer)[i]);	\
							 } \
							 ksceDebugPrintf("\n")
#else
#define PRINT_STR(...) /**/
#define PRINT_BUFFER(buffer) /**/
#define PRINT_BUFFER_LEN(buffer, len) /**/
#endif

#endif // LOG_H