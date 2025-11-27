/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#ifndef F00D_EMU_H
#define F00D_EMU_H 1
#include "compiler_defs.h"

typedef uint16_t cmd56_sm_keyid;
enum CMD56_SM_KEY_IDS {
	PROTOTYPE_KEY_ID1 = 0x8001,

	// on CEX 1.04+ 8002 and 8003 are hardcoded blocked in gcauthmgr.skprx 
	PROTOTYPE_KEY_ID2 = 0x8002,
	PROTOTYPE_KEY_ID3 = 0x8003,

	RETAIL_KEY_ID = 0x1
};

#define decrypt_cbc_zero_iv(ctx, data, len) AES_CBC_decrypt_buffer(ctx, (uint8_t*)data, len, (uint8_t*)NULL);
#define encrypt_cbc_zero_iv(ctx, data, len) AES_CBC_encrypt_buffer(ctx, (uint8_t*)data, len, (uint8_t*)NULL);

int derive_session_key(uint8_t* session_key_out, uint8_t* cart_random, int key_id);
void do_cmd56_cmac_hash(AES_ctx* ctx, void* data, uint32_t header, uint8_t* output, size_t size);

// random number generators
void rand_seed(const void* seed, size_t size);
void rand_bytes(void* buf, size_t size);
uint32_t rand_uint32(int limit);

#define or_w_80(buf, size) for(int i = 0; i < size; i+=0x10) { \
								((uint8_t*)buf)[i] |= 0x80; \
						   }

#define rand_bytes_or_w_80(buf, size) do { \
										rand_bytes(buf, size); \
										or_w_80(buf, size); \
									  } while(0);

#endif