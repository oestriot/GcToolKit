#ifndef __AES_CMAC_H__
#define __AES_CMAC_H__

#include "aes.h"

void AES_CMAC_buffer(AES_ctx* ctx, void *input, size_t length, uint8_t *mac_value);
void AES_CMAC_buffer_key(const uint8_t* key, void* input, size_t length, uint8_t* output);

#endif
