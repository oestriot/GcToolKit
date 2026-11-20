#include "compiler_defs.h"


#if no_include(<string>)

// define our own memory functions if dont have libc.
void* memcpy(void* buf, void const* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t*)buf)[i] = ((uint8_t*)src)[i];
    }
    return buf;
}
void* memset(void* buf, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t*)buf)[i] = ((uint8_t)c);
    }
    return buf;
}
int memcmp(void const* a, void const* b, size_t len) {
    size_t count = len;
    uint8_t* s1 = (uint8_t*)a;
    uint8_t* s2 = (uint8_t*)b;
    while (count-- > 0)
    {
        if (*s1++ != *s2++) {
            return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

#endif

uint8_t is_endian_big() {
    uint16_t i = 1;
    return ((uint8_t*)&i)[0] == 0;
}
uint8_t is_endian_little() {
    return !is_endian_big();
}

uint16_t endian_big16(uint16_t val) {
    if (is_endian_little()) return endian_swap16(val);
    else return val;
}
uint16_t endian_little16(uint16_t val) {
    if (is_endian_big()) return endian_swap16(val);
    else return val;
}

uint32_t endian_little32(uint32_t val) {
    if (is_endian_big()) return endian_swap32(val);
    else return val;

}
uint32_t endian_big32(uint32_t val) {
    if (is_endian_little()) return endian_swap32(val);
    else return val;
}