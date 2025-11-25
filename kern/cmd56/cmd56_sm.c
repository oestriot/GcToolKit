/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#include "log.h"
#include "crypto/aes.h"
#include "crypto/aes_cmac.h"
#include "cmd56_sm.h"
#include "compiler_defs.h"

/*
* 
*   BIGMAC_KEY_0x345      = 74C39CA4EF4F122915C71EDA46C88B55BBAD1F4033D755CEA0563CC341F92E66
* 
*	GCAUTHMGR_0x8001_SEED = 6f2285ed463a6e57c5f3550ddcc81feb
*	GCAUTHMGR_0x8002_SEED = da9608b528825d6d13a7af1446b8ec08
*	GCAUTHMGR_0x8003_SEED = 368b2eb5437a821862a6c95596d8c135
*	GCAUTHMGR_0x1_SEED    = 7f1fd065dd2f40b3e26579a6390b616d
*
*	derived by aes-256-ecb decrypt GCAUTHMGR_KEYID_SEED value with BIGMAC_KEY_0x345
*   this is not incuded here because its result is always the same, 
*	calculating it every time would be slower-
*	and would also require an additional implementation of aes-256, which would make filesize larger.
*/ 

static const uint8_t BIGMAC_KEY_0x348[0x20]		= { 0xC0, 0x26, 0x28, 0x14, 0x13, 0xFA, 0x46, 0x2C, 0xCD, 0xEE, 0xD4, 0xBD, 0x6D, 0x08, 0xC3, 0x7C, 0xA6, 0xC9, 0x32, 0x2A, 0xBD, 0x4C, 0x40, 0xAD, 0xE7, 0x2A, 0x0F, 0x54, 0x4F, 0x40, 0x13, 0xAD };

static const uint8_t GCAUTHMGR_0x8001_KEY[0x10] = { 0xCB, 0x80, 0x8D, 0x14, 0x02, 0x62, 0x53, 0x17, 0x25, 0x24, 0xD8, 0xA1, 0xf5, 0x1D, 0x35, 0xC7 };
static const uint8_t GCAUTHMGR_0x8002_KEY[0x10] = { 0x56, 0xB6, 0x7b, 0xE1, 0x00, 0x03, 0xB4, 0x3B, 0xB8, 0x24, 0xD7, 0x06, 0xEE, 0x93, 0x59, 0x9D };
static const uint8_t GCAUTHMGR_0x8003_KEY[0x10] = { 0xE8, 0xBD, 0xDA, 0xFb, 0xF4, 0xA3, 0xB9, 0x9B, 0x54, 0x56, 0x2C, 0x68, 0x21, 0xD8, 0x05, 0x1E };
static const uint8_t GCAUTHMGR_0x1_KEY[0x10]    = { 0x72, 0x50, 0x6A, 0x4B, 0xA8, 0x36, 0xC8, 0x76, 0xC4, 0x48, 0x40, 0x70, 0x1F, 0x0E, 0xA1, 0x02 };
static const uint8_t GCAUTHMGR_0x1_IV[0x10]     = { 0x8b, 0x14, 0xc8, 0xa1, 0xe9, 0x6f, 0x30, 0xa7, 0xf1, 0x01, 0xa9, 0x6a, 0x30, 0x33, 0xc5, 0x5b };


// gcauthmgr_sm

void derive_session_key(uint8_t* session_key_out, uint8_t* cart_random, int key_id) {
	const uint8_t* keyseed;

	switch (key_id) {
		case PROTOTYPE_KEY_ID1:
			keyseed = GCAUTHMGR_0x8001_KEY;
			break;
		case PROTOTYPE_KEY_ID2:
			keyseed = GCAUTHMGR_0x8002_KEY;
			break;
		case PROTOTYPE_KEY_ID3:
			keyseed = GCAUTHMGR_0x8003_KEY;
			break;
		case RETAIL_KEY_ID:
			keyseed = GCAUTHMGR_0x1_KEY;
			break;
		default:
			PRINT_STR("(F00D) invalid key id passed to derive_session_key 0x%x\n", key_id);
			return;
	}

	AES_CMAC_buffer_key(keyseed, cart_random, 0x20, session_key_out);
	
	PRINT_STR("(F00D) CMAC session_key_out: ");
	PRINT_BUFFER_LEN(session_key_out, 0x10);

	if (key_id == 0x1) {
		AES_CBC_decrypt_buffer_key(BIGMAC_KEY_0x348, session_key_out, 0x10, GCAUTHMGR_0x1_IV);

		PRINT_STR("(F00D) session_key: ");
		PRINT_BUFFER_LEN(session_key_out, 0x10);
	}

}

void do_cmd56_cmac_hash(AES_ctx* ctx, void* data, uint32_t header, uint8_t* output, size_t size) {
	uint8_t cmac_input[0x50];
	memset(cmac_input, 0x00, sizeof(cmac_input));

	// aes-128-cmac the whole thing
	memcpy(cmac_input, &header, 0x3);
	memcpy(cmac_input + 0x10, data, size); // copy data to cmac_input + 0x10

	AES_CMAC_buffer(ctx, cmac_input, size + 0x10, output); // caclulate the CMAC ...

	PRINT_STR("(CMD56) CMAC: ");
	PRINT_BUFFER_LEN(output, 0x10);
}

// random number generator
static uint8_t rand_state[0x10] = "TRANS RIGHTS!!!!";

void rand_seed(const void* seed, size_t size) {
	size_t seed_size = (size < sizeof(rand_state)) ? size : sizeof(rand_state);
	for (int i = 0; i < seed_size; i++) rand_state[i % sizeof(rand_state)] ^= ((uint8_t*)seed)[i];
}


void rand_bytes(void* buf, size_t size) {
#ifdef USE_PS3_MODE
	memset(buf, 0xAA, size);
	return;
#else
	for (int i = 0; i < size; i += sizeof(rand_state)) {
		// determine copy size 
		size_t copy_size = ((size - i) < sizeof(rand_state)) ? (size - i) : sizeof(rand_state);

		// cycle rng state.
		AES_CBC_encrypt_buffer_key(rand_state, rand_state, sizeof(rand_state), rand_state);

		// copy rng state to buffer output.
		memcpy((uint8_t*)buf + i, rand_state, copy_size);
	}
#endif
}

uint32_t rand_uint32(int limit) {
	uint32_t i;
	rand_bytes(&i, sizeof(uint32_t));
	return i % limit;
}