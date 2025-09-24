#ifndef CRYPTO_H
#define CRYPTO_H 1

#include "vci.h"
#include "psv.h"

int key_dump_network(char* ip_address, unsigned short port, char* output_file);
int key_dump(char* output_file);

int extract_gc_keys(GcCmd56Keys* keys);

void derive_cart_secret(GcCmd56Keys* keys, uint8_t* cart_secret);
void derive_packet20_hash(GcCmd56Keys* keys, uint8_t* packet20_hash);

void wait_for_gc_auth();

#endif //CRYPTO_H