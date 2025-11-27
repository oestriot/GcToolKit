/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#ifndef VITA_H
#define VITA_H 1
#include "compiler_defs.h"
#include "crypto/aes.h"
#include "cmd56.h"
#include "cmd56_sm.h"

typedef void (*send_t)(const uint8_t* data, size_t size);
typedef void (*recv_t)(uint8_t* data, size_t size);

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

typedef struct vita_cmd56_state {
    // send/recv callbacks
    send_t send;
    recv_t recv;

    // cart lock status, unlocked after
    // successful VITA_AUTHENTICITY_CHECK
    cmd56_lock_status lock_status;

    // set to 0x1 to allow using keyid > 0x8001
    // which is the case on prototype firmwares, 
    // on fw 1.04+ this is not allowed, 
    bool allow_prototype_keys;

    // KEY_ID to use, every gc i've ever seen uses RETAIL_KEY_ID (0x1).
    // however a PROTOTYPE_KEY_ID1 0x8001 is also allowed,

    // PROTOTYPE_KEY_ID actually has worse security in place, 
    // however sony could easily check for this in a future update
    // and can use RETAIL_KEY_ID thanks to the racoon exploit anyway. 

    // There also exists a PROTOTYPE_KEY_ID2 and 3,0x8002 and 0x8003, 
    // however these are blacklisted as of fw 1.04.
    cmd56_sm_keyid key_id;

    // per-gc keys, used to derive the rif key,
    // which is used to decrypt the game klicensee,
    // and finally PFS/EBOOT.BIN
    cmd56_keys per_cart_keys;

    // CART_RANDOM is used to derive the secondary_key using bbmac 0x305
    // and 0x308 (only on RETAIL_KEY_ID) 
    uint8_t cart_random[0x20];
    shared_random shared_random;

    AES_ctx session_key;
    AES_ctx secondary_key;
} vita_cmd56_state;

// exposed functions:
void vita_cmd56_init(vita_cmd56_state* state, send_t send_func, recv_t recv_func);
void vita_cmd56_init_ex(vita_cmd56_state* state, send_t send_func, recv_t recv_func, bool allow_prototype_keys);
void vita_cmd56_get_keys(vita_cmd56_state* state, cmd56_keys* keys);
void vita_cmd56_get_keys_ex(vita_cmd56_state* state, uint8_t p20_key[0x20], uint8_t p18_key[0x20]);
int vita_cmd56_run(vita_cmd56_state* state);

#endif