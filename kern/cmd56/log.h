/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/
#ifndef LOG_H
#define LOG_H 1

#if (!defined(LOGGING_ENABLED) && defined(_DEBUG))
	#define LOGGING_ENABLED 1
#endif

#if defined(LOGGING_ENABLED)
	#if defined(__vita__)
		#if defined(VITA_IS_USERLAND)
			#include <vitasdk.h>
			#define PRINT_STR(...) sceClibPrintf(__VA_ARGS__)
		#else
			#include <vitasdkkern.h>
			#define PRINT_STR(...) ksceKernelPrintf(__VA_ARGS__)
		#endif
	#else 
		#include <stdio.h>
		#define PRINT_STR(...) printf(__VA_ARGS__)
	#endif
	
	// define PRINT_BUFFER & PRINT_BUFFER_LEN
	#define PRINT_BUFFER(buffer) PRINT_BUFFER_LEN(buffer, sizeof(buffer))
	#define PRINT_BUFFER_LEN(buffer, size) for(int i = 0; i < size; i++) { PRINT_STR("%02X ", ((unsigned char*)buffer)[i]); }; PRINT_STR("\n");

#else
	#define PRINT_STR(...) /**/
	#define PRINT_BUFFER(buffer) /**/
	#define PRINT_BUFFER_LEN(buffer, size) /**/
#endif

#endif