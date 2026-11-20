/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#ifndef VITA_H
#define VITA_H 1
#include "compiler_defs.h"
#include "crypto/aes.h"
#include "cmd56.h"


typedef enum vita_error_code {
    GC_AUTH_OK = 0x00,

    GC_INVALID_ARGUMENT = 0x8001000,
    GC_AUTH_ERROR_START_FAIL = 0x8001001,

    GC_AUTH_ERROR_INVALID_KEYID = 0x8002001,
    
    GC_AUTH_ERROR_VERIFY_SHARED_RANDOM_FAIL = 0x8003001,
    GC_AUTH_ERROR_VERIFY_SHARED_RANDOM_INVALID = 0x8003002,

    GC_AUTH_ERROR_VERIFY_VITA_AUTH_PROOF_FAIL = 0x8004001,
    
    GC_AUTH_ERROR_VERIFY_CART_RANDOM_FAIL = 0x8005001,
    GC_AUTH_ERROR_VERIFY_CART_RANDOM_CHALLENGE_INVALID = 0x8005002,
    GC_AUTH_ERROR_VERIFY_CART_RANDOM_INVALID_CART_RANDOM = 0x8005003,

    GC_AUTH_ERROR_P18_KEY_CHALLANGE_FAIL = 0x8006001,
    GC_AUTH_ERROR_P18_KEY_INVALID_CMAC = 0x8006002,

    GC_AUTH_ERROR_P20_KEY_CHALLANGE_FAIL = 0x8007001,
    GC_AUTH_ERROR_P20_KEY_INVALID_CMAC = 0x8007002,
    
    GC_AUTH_ERROR_LOCKED = 0x8008001,
    GC_AUTH_ERROR_UNLOCKED = 0x8008002,

    GC_AUTH_ERROR_REPORTED = 0x800F001
} vita_error_code;



// exposed functions:
void vita_cmd56_init(cmd56_state* state, cmd56_send_t send_func, cmd56_recv_t recv_func);
void vita_cmd56_init_ex(cmd56_state* state, cmd56_send_t send_func, cmd56_recv_t recv_func, boolean allow_prototype_keys);
void vita_cmd56_get_keys(cmd56_state* state, cmd56_keys* keys);
void vita_cmd56_get_keys_ex(cmd56_state* state, uint8_t p20_key[0x20], uint8_t p18_key[0x20]);
cmd56_sm_keyid vita_cmd56_get_key_id(cmd56_state* state);

int vita_cmd56_run(cmd56_state* state);

#endif