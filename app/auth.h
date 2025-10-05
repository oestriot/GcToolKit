#ifndef AUTH_H
#define AUTH_H 1

#include <assert.h>

/*
* GcCmd56Keys are:
*	Keys exchanged as part of the CMD56 authentication process.
*	sha256(GcCmd56Keys) equals cart_secret in (e.g psvgamesd)
*	despite not needing to be; these keys differ per-cart, not per-game.
*	
*	and every gamecart has its own license RIF to go along with this
*	for this reason, every vita gamecart is personalized.
*
*/

typedef struct GcCmd56Keys{
	uint8_t packet20_key[0x20];
	uint8_t packet18_key[0x20];
} __attribute__((packed)) GcCmd56Keys;
static_assert(sizeof(GcCmd56Keys) == 0x40);

int key_dump_network(char* ip_address, unsigned short port, char* output_file);
int key_dump(char* output_file);

int extract_gc_keys(GcCmd56Keys* keys);

void derive_cart_secret(GcCmd56Keys* keys, uint8_t* cart_secret);
void derive_packet20_hash(GcCmd56Keys* keys, uint8_t* packet20_hash);

void wait_for_gc_auth();

#endif //AUTH_H