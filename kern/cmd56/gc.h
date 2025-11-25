/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#ifndef GC_H
#define GC_H 1
#include "compiler_defs.h"
#include "crypto/aes.h"
#include "cmd56.h"
#include "cmd56_sm.h"

typedef struct gc_cmd56_state {
    // cart lock status, unlocked after
    // successful VITA_AUTHENTICITY_CHECK
    cmd56_lock_status lock_status;

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
} gc_cmd56_state;

// exposed functions:
void gc_cmd56_init(gc_cmd56_state* state, const cmd56_keys* keys);
void gc_cmd56_update_keyid(gc_cmd56_state* state, uint16_t key_id);

void gc_cmd56_update_keys(gc_cmd56_state* state, const cmd56_keys* keys);
void gc_cmd56_update_keys_ex(gc_cmd56_state* state, const uint8_t p20_key[0x20], const uint8_t p18_key[0x20]);

void gc_cmd56_run_in_place(gc_cmd56_state* state, uint8_t* buffer);
void gc_cmd56_run(gc_cmd56_state* state, const uint8_t* buffer, uint8_t* response);

#endif /* GC_H */