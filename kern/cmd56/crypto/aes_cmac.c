#include "aes.h"
#include "aes_cmac.h"

#define AES_BLOCKSIZE 16

static void left_shift(uint8_t *dest, uint8_t *src)
{
    uint8_t overflow = 0;

    for(int i = 15; i >= 0; i--)
    {
        dest[i] = src[i] << 1;
        dest[i] |= overflow;
        overflow = (src[i] >> 7) & 1;
    }

}

//put 0x80, 0x00, 0x00 after the first len bytes of block
static inline void add_padding(uint8_t *block, int len)
{
    for(int i = len; i < AES_BLOCKSIZE; i++)
        block[i] = 0;
    block[len] = 0x80;
}

static inline void block_xor_triple(uint8_t *a, uint8_t *b, uint8_t *c)
{
    for(int i = 0; i < AES_BLOCKSIZE; i++)
        c[i] = a[i] ^ b[i];
}

static inline void gen_subkey(AES_ctx *aes_ctx, uint8_t *subkey_1, uint8_t *subkey_2)
{
    uint8_t L[16] = {0};
    AES_ECB_encrypt(aes_ctx, L);

    left_shift(subkey_1, L);
    if(L[0] & 0x80)
        subkey_1[15] ^= 0x87;

    left_shift(subkey_2, subkey_1);
    if(subkey_1[0] & 0x80)
        subkey_2[15] ^= 0x87;

}


void AES_CMAC_buffer(AES_ctx* aes_ctx, void *input_p, size_t length, uint8_t *mac_value)
{
    uint8_t* input = input_p;
    uint8_t subkey_1[AES_BLOCKSIZE];
    uint8_t subkey_2[AES_BLOCKSIZE];
    uint8_t previous_block_ciphertext[AES_BLOCKSIZE] = {0};
    uint8_t temp[AES_BLOCKSIZE];

    gen_subkey(aes_ctx, subkey_1, subkey_2);

    for(uint32_t i = 0; i < length; i+= AES_BLOCKSIZE)
    {

        block_xor_triple(input, previous_block_ciphertext, temp);

        if(i + AES_BLOCKSIZE == length)
        {
            //the last block if full, xor with subkey_1
            block_xor_triple(temp, subkey_1, temp);
        }
        else if(i + AES_BLOCKSIZE > length)
        {
            //last block is not full, add padding
            add_padding(temp, length - i);
            block_xor_triple(temp, subkey_2, temp);
        }

        AES_ECB_encrypt(aes_ctx, temp);
        memcpy(previous_block_ciphertext, temp, AES_BLOCKSIZE);
        input += AES_BLOCKSIZE;
    }
    memcpy(mac_value, previous_block_ciphertext, AES_BLOCKSIZE);
}

void AES_CMAC_buffer_key(const uint8_t* key, void* input, size_t length, uint8_t* output) {
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_CMAC_buffer(&ctx, input, length, output);
}