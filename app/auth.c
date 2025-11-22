#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vitasdk.h>

#include "aes.h"
#include "aes_cmac.h"

#include "sha256.h"
#include "sha1.h"

#include "io.h"
#include "device.h"

#include <GcToolKit.h>
#include "auth.h"
#include "net.h"
#include "err.h"
#include "log.h"

static uint8_t BIGMAC_KEY_0x345[0x20] = { 0x74, 0xC3, 0x9C, 0xA4, 0xEF, 0x4F, 0x12, 0x29, 0x15, 0xC7, 0x1E, 0xDA, 0x46, 0xC8, 0x8B, 0x55, 0xBB, 0xAD, 0x1F, 0x40, 0x33, 0xD7, 0x55, 0xCE, 0xA0, 0x56, 0x3C, 0xC3, 0x41, 0xF9, 0x2E, 0x66 };
static uint8_t BIGMAC_KEY_0x348[0x20] = { 0xC0, 0x26, 0x28, 0x14, 0x13, 0xFA, 0x46, 0x2C, 0xCD, 0xEE, 0xD4, 0xBD, 0x6D, 0x08, 0xC3, 0x7C, 0xA6, 0xC9, 0x32, 0x2A, 0xBD, 0x4C, 0x40, 0xAD, 0xE7, 0x2A, 0x0F, 0x54, 0x4F, 0x40, 0x13, 0xAD };

static uint8_t GCAUTHMGR_8001_SEED[0x10] = { 0x6f, 0x22, 0x85, 0xed, 0x46, 0x3a, 0x6e, 0x57, 0xc5, 0xf3, 0x55, 0x0d, 0xdc, 0xc8, 0x1f, 0xeb };
static uint8_t GCAUTHMGR_8002_SEED[0x10] = { 0xda, 0x96, 0x08, 0xb5, 0x28, 0x82, 0x5d, 0x6d, 0x13, 0xa7, 0xaf, 0x14, 0x46, 0xb8, 0xec, 0x08 };
static uint8_t GCAUTHMGR_8003_SEED[0x10] = { 0x36, 0x8b, 0x2e, 0xb5, 0x43, 0x7a, 0x82, 0x18, 0x62, 0xa6, 0xc9, 0x55, 0x96, 0xd8, 0xc1, 0x35 };
static uint8_t GCAUTHMGR_1_SEED[0x10] 	 = { 0x7f, 0x1f, 0xd0, 0x65, 0xdd, 0x2f, 0x40, 0xb3, 0xe2, 0x65, 0x79, 0xa6, 0x39, 0x0b, 0x61, 0x6d };

static uint8_t GCAUTHMGR_1_IV[0x10] = { 0x8b, 0x14, 0xc8, 0xa1, 0xe9, 0x6f, 0x30, 0xa7, 0xf1, 0x01, 0xa9, 0x6a, 0x30, 0x33, 0xc5, 0x5b };
static uint8_t ZERO_IV[0x10] = { 0 };

void derive_master_key(uint8_t* cartRandom, uint8_t* masterkey, int key_id) {
	uint8_t* kseed = NULL;
	uint8_t x21[0x10];
	uint8_t cmac[0x10];
	
	switch (key_id) {
	case 0x8001:
		kseed = GCAUTHMGR_8001_SEED;
		break;
	case 0x8002:
		kseed = GCAUTHMGR_8002_SEED;
		break;
	case 0x8003:
		kseed = GCAUTHMGR_8003_SEED;
		break;
	case 0x1:
		kseed = GCAUTHMGR_1_SEED;
		break;
	}

	AES_ECB_decrypt(kseed, x21, sizeof(x21), BIGMAC_KEY_0x345, 0x20);
	aes_cmac(cartRandom, 0x20, x21, cmac);
	
	if (key_id == 0x1) {
		AES_CBC_decrypt(cmac, masterkey, 0x10, BIGMAC_KEY_0x348, 0x10, GCAUTHMGR_1_IV);
	}
	else {
		memcpy(masterkey, cmac, sizeof(cmac));
	}

}

int key_dump_network(char* ip_address, unsigned short port, char* output_file) {
	GcCmd56Keys keys;
	int netwr = -1;
	
	int got_keys = extract_gc_keys(&keys);
	if(got_keys < 0) return got_keys;
	
	SceUID fd = begin_connection(ip_address, port);
	PRINT_STR("fd = %x\n", fd);
	if(fd < 0) return fd;
	
	int res = begin_file_send(fd, output_file, sizeof(GcCmd56Keys));
	if(res < 0) goto error;
	
	netwr = file_send_data(fd, &keys, sizeof(GcCmd56Keys));
	PRINT_STR("netwr = %x (sizeof = %x)\n", netwr, sizeof(GcCmd56Keys));

error:
	if(fd >= 0) end_connection(fd);

	if(netwr == 0) return SIZE_IS_ZERO;
	if(netwr != sizeof(GcCmd56Keys)) return SIZE_NOT_MATCH;
	
	return res;
}

int key_dump(char* output_file) {
	GcCmd56Keys keys;
	make_directories_excluding_last(output_file);

	int got_keys = extract_gc_keys(&keys);
	if(got_keys < 0) return got_keys;
	
	int wr = write_file(output_file, &keys, sizeof(GcCmd56Keys));
	
	if(wr == 0) return SIZE_IS_ZERO;
	if(wr < 0) return wr;
	if(wr != sizeof(GcCmd56Keys)) return SIZE_NOT_MATCH;

	return 0;
}


void decrypt_packet18_key(uint8_t* secondaryKey0, uint8_t* packet18, uint8_t* packet18_key) {
	uint8_t wbuf[0x30];
	
	AES_CBC_decrypt(packet18+3, wbuf, sizeof(wbuf), secondaryKey0, 0x10, ZERO_IV);
	
	memcpy(packet18_key, wbuf+0x10, 0x20);
}

void decrypt_packet20_key(uint8_t* secondaryKey0, uint8_t* packet20, uint8_t* packet20_key) {
	uint8_t wbuf[0x40];
	
	AES_CBC_decrypt(packet20+3, wbuf, sizeof(wbuf), secondaryKey0, 0x10, ZERO_IV);

	memcpy(packet20_key, wbuf+0x18, 0x20);

}

void wait_for_gc_auth() {
	int res = kResetCmd20Input();
	PRINT_STR("kResetCmd20Input = %x\n", res);
	if (res >= 0) {
		do {
			
			// check if there is already a GC inserted, if there is 
			// reset the gc device to capture authentication step
			// we, dont do this if there is not a gc inserted, incase someone is using an sd2vita.
			
			if( file_exist("gro0:") || file_exist("grw0:") || device_exist(BLOCK_DEVICE_MEDIAID) ) {
				res = kResetGc();
				PRINT_STR("kResetGc = %x\n", res);
			}		
					
			sceKernelDelayThread(1000 * 10); // 10ms
		} while(!kHasCmd20Captured());
	}
}

void derive_packet20_hash(GcCmd56Keys* keys, uint8_t* packet20_hash) {
	SHA1_CTX ctx;
	
	PRINT_STR("SHA1_INIT RUN\n");
	sha1_init(&ctx);
	
	PRINT_STR("SHA1_UPDATE RUN\n");
	sha1_update(&ctx, keys->packet20_key, sizeof(keys->packet20_key));
	
	PRINT_STR("SHA256_FINAL RUN\n");
	sha1_final(&ctx, packet20_hash);
}

void derive_cart_secret(GcCmd56Keys* keys, uint8_t* cart_secret) {
	// final cart secret == sha256 of packet20_key+packet18_key
	
	PRINT_STR("derive_cart_secret %p %p\n", keys, cart_secret);
	
	SHA256_CTX ctx;
	
	PRINT_STR("SHA256_INIT RUN\n");
	sha256_init(&ctx);

	PRINT_STR("SHA256_UPDATE RUN\n");
	sha256_update(&ctx, (uint8_t*)keys, sizeof(GcCmd56Keys));

	PRINT_STR("SHA256_FINAL RUN\n");
	sha256_final(&ctx, cart_secret);
	
}

uint8_t verify_packet18_key(GcCmd56Keys* keys) {
	uint8_t got_final_keys[0x20];
	uint8_t expected_final_keys[0x20];
	
	PRINT_STR("verifying packet18_key ...\n");
	
	int res = kGetCartSecret(got_final_keys);
	PRINT_STR("kGetCartSecret res = 0x%X, got_final_keys = %p, expected_final_keys = %p, keys = %p\n", res, got_final_keys, expected_final_keys, keys);
	if(res < 0) goto error;
	
	derive_cart_secret(keys, expected_final_keys);
	
	if(memcmp(got_final_keys, expected_final_keys, sizeof(expected_final_keys)) == 0) {
		PRINT_STR("KEYS VERIFIED SUCCESS!\n");
		return 1;
	}
error:	
	PRINT_STR("verify_packet18_key failed !\n");
	PRINT_STR("got_final_keys ");
	PRINT_BUFFER(got_final_keys);
	
	PRINT_STR("expected_final_keys ");
	PRINT_BUFFER(expected_final_keys);
	
	return 0;
}

uint8_t verify_packet20_key(GcCmd56Keys* keys) {
	uint8_t expected_final_rif_hash[SHA1_BLOCK_SIZE];
	uint8_t got_final_rif_hash[SHA1_BLOCK_SIZE];
	
	derive_packet20_hash(keys, expected_final_rif_hash);
	
	// get title id from license folder
	char folder[MAX_PATH];
	snprintf(folder, MAX_PATH, "gro0:/license/app");
	PRINT_STR("folder = %s\n", folder);
	
	char title_id[12];
	int res = read_first_filename(folder, title_id, sizeof(title_id));
	PRINT_STR("read_first_filename license folder res = 0x%x\n", res);
	if(res < 0) return 0;

	PRINT_STR("title_id = %s\n", title_id);
	snprintf(folder, MAX_PATH, "gro0:/license/app/%s", title_id);
	PRINT_STR("folder = %s\n", folder);
	
	// get rif name from license/titleid folder
	char rif_filename[MAX_PATH];
	res = read_first_filename(folder, rif_filename, sizeof(rif_filename));
	PRINT_STR("read_first_filename license titleid folder res = 0x%x\n",res);
	if(res < 0) return 0;
	
	PRINT_STR("rif_filename = %s\n", rif_filename);
	snprintf(folder, MAX_PATH, "gro0:/license/app/%s/%s", title_id, rif_filename);
	PRINT_STR("folder = %s\n", folder);
	
	// read the hash from the rif file ..
	
	SceUID fd = sceIoOpen(folder, SCE_O_RDONLY, 0777);
	PRINT_STR("rif fd = %x\n", fd);
	if(fd < 0) goto error;
	
	// final rif hash should == 0xE0 in rif file
	uint64_t loc = sceIoLseek(fd, 0xE0, SCE_SEEK_SET);
	PRINT_STR("sceIoLseek loc = %llx\n", loc);
	if(loc != 0xE0) goto error;
	
	res = sceIoRead(fd, got_final_rif_hash, SHA1_BLOCK_SIZE);
	PRINT_STR("sceIoRead res = %x\n", res);
	if(res != SHA1_BLOCK_SIZE) goto error;
	
	sceIoClose(fd);
	
	
	if(memcmp(expected_final_rif_hash, got_final_rif_hash, SHA1_BLOCK_SIZE) == 0){
		return 1;
	}

	PRINT_STR("verify_packet20_key failed!\n");
	
error:	
	if(fd > 0) sceIoClose(fd);
	return 0;
	
}

int extract_gc_keys(GcCmd56Keys* keys) {
	if(kHasCmd20Captured()) {
		// get captured cmd56 authentication data
		CommsData cmdData;
		kGetLastCmd20Input(&cmdData);
		
		int key_id = kGetLastCmd20KeyId();
		PRINT_STR("key_id = %x\n", key_id);
		
		uint8_t masterKey[0x10];
		derive_master_key(cmdData.packet6, masterKey, key_id);

		PRINT_STR("masterKey ");
		PRINT_BUFFER(masterKey);

		uint8_t secondaryKey0[0x10];
		AES_CBC_decrypt(cmdData.packet9, secondaryKey0, sizeof(secondaryKey0), masterKey, sizeof(masterKey), ZERO_IV);

		PRINT_STR("secondaryKey0 ");
		PRINT_BUFFER(secondaryKey0);
			
		// decrypt packet18_key part from packet18
		decrypt_packet18_key(secondaryKey0, cmdData.packet18, keys->packet18_key);

		PRINT_STR("packet18_key: ");
		PRINT_BUFFER(keys->packet18_key);

		// decrypt rif part from packet20
		decrypt_packet20_key(secondaryKey0, cmdData.packet20, keys->packet20_key);
		
		PRINT_STR("packet20_key: ");
		PRINT_BUFFER(keys->packet20_key);
		
		// verify packet18_key key
		if(!verify_packet18_key(keys)) return KEYS_VERIFY_P18_FAILED;
		
		// verify rif buffer
		if(file_exist("gro0:"))
			if(!verify_packet20_key(keys)) return KEYS_VERIFY_P20_FAILED;
		
		
		return 0;
	}
	return KEYS_NOT_CAPTURED;
}