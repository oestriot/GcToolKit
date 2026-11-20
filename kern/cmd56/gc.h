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


void gc_cmd56_init(cmd56_state* state, const cmd56_keys* keys);
void gc_cmd56_update_keyid(cmd56_state* state, uint16_t key_id);

void gc_cmd56_update_keys(cmd56_state* state, const cmd56_keys* keys);
void gc_cmd56_update_keys_ex(cmd56_state* state, const uint8_t p20_key[0x20], const uint8_t p18_key[0x20]);

cmd56_sm_keyid gc_cmd56_get_keyid(cmd56_state* state);
void gc_cmd56_get_keys_ex(cmd56_state* state, cmd56_keys* per_cart_keys, uint8_t p20_key[0x20], uint8_t p18_key[0x20]);
void gc_cmd56_get_keys(cmd56_state* state, cmd56_keys* per_cart_keys);

void gc_cmd56_run_in_place(cmd56_state* state, void* buffer);
void gc_cmd56_run(cmd56_state* state, const void* buffer, void* response);



#endif /* GC_H */