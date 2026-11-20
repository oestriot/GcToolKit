/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#ifndef COMPILER_DEFS_H
#define COMPILER_DEFS_H 1

// test for libc, and headers
#define has_libc (defined(__GLIBC__))
#define has_include(inc) (__has_include(inc) || !has_libc)
#define no_include(inc) (!__has_include(inc) || !has_libc)

#ifdef __GNUC__
#define PACK( declaration ) declaration __attribute__((__packed__))
#elif _MSC_VER
#define PACK( declaration ) __pragma(pack(push, 1) ) declaration __pragma(pack(pop))
#endif

#ifdef _MSC_VER
#define endian_swap16 _byteswap_ushort
#define endian_swap32 _byteswap_ulong
#elif __GNUC__
#define endian_swap16 __builtin_bswap16
#define endian_swap32 __builtin_bswap32
#endif

#if has_include(<stdint.h>) // if we dont have libc, specify our own versions of stdint variables.
    #include <stdint.h>
#else
    typedef signed char int8_t;
    typedef unsigned char uint8_t;
    typedef signed short int int16_t;
    typedef unsigned short int uint16_t;
    typedef signed int int32_t;
    typedef unsigned int uint32_t;
    typedef signed long long int int64_t;
    typedef unsigned long long int uint64_t;
#endif

#if has_include(<stddef.h>) 
#include <stddef.h>
#else
#if defined(_WIN64) || defined(__x86_64__)
    typedef unsigned __int64 size_t;
    typedef unsigned __int64 uintptr_t;
#elif defined(__GNUC__)
    typedef __SIZE_TYPE__ size_t;
    typedef size_t uintptr_t;
#else
    typedef unsigned int     size_t;
    typedef unsigned int     uintptr_t;
#endif
#endif

#if defined(_MSC_VER) // msvc has memset/memcpy, etc as compiler builtins ..
#define memset __impl_memset
#define memcpy __impl_memcpy
#define memcmp __impl_memcmp
void* memcpy(void* buf, void const* src, size_t n);
void* memset(void* buf, int c, size_t n);
int memcmp(void const* a, void const* b, size_t len);
#elif no_include(<string>)
    void* memcpy(void* buf, void const* src, size_t n);
    void* memset(void* buf, int c, size_t n);
    int memcmp(void const* a, void const* b, size_t len);
#else // we have libc, so just use the default ones.
#include <string.h>
#endif


#ifdef _MSC_VER
#define inline __forceinline
#endif

#ifndef offsetof
#define offsetof(st, m) ((size_t)&(((st*)0)->m))
#endif

#ifndef NULL 
#define NULL ((void*)0)
#endif

typedef enum boolean {
    TRUE = 1,
    FALSE = 0
} boolean;

uint16_t endian_big16(uint16_t val);
uint16_t endian_little16(uint16_t val);

uint32_t endian_little32(uint32_t val);
uint32_t endian_big32(uint32_t val);

#endif
