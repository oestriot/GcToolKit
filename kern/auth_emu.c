#include <stdio.h>
#include <stdarg.h>
#include <vitasdkkern.h>
#include <taihen.h>

#include <GcToolKit.h>

#include "sha1.h"
#include "sha256.h"
#include "cmd56/vita.h"
#include "log.h"

static vita_cmd56_state vita_state = { 0 };
static SceSdifDeviceContext* ctx = NULL;
static int is_authenticated = 0;


static int authHook = -1;
static tai_hook_ref_t authHookRef;

static int getCartSecretHook = -1;
static tai_hook_ref_t getCartSecretHookRef;

static int clearCartSecretHook = -1;
static tai_hook_ref_t clearCartSecretHookRef;

static int checkCartHashHook = -1;
static tai_hook_ref_t checkCartHashHookRef;

int k_get_hash(uint8_t* hash) {
	SHA1_CTX ctx;
	sha1_init(&ctx);
	sha1_update(&ctx, vita_state.per_cart_keys.packet20_key, sizeof(vita_state.per_cart_keys.packet20_key));
	sha1_final(&ctx, hash);
	
	return 0;
}

int k_get_secret(uint8_t* secret) {
	PRINT_STR("k_get_secret\n");
	
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (uint8_t*)&vita_state.per_cart_keys, sizeof(cmd56_keys));
	sha256_final(&ctx, secret);
	
	return 0;
}

int k_check_hash(const uint8_t* secret) {
	PRINT_STR("k_check_hash\n");
	
	uint8_t k_cart_hash[0x14];
	k_get_hash(k_cart_hash);
	
	return memcmp(secret, k_cart_hash, sizeof(k_cart_hash));	
}

int k_clear_secret() {
	PRINT_STR("k_clear_secret\n");
	memset(&vita_state, 0x00, sizeof(vita_state));
	is_authenticated = 0;
	return 0;
}


void k_gc_cmd56_write(const uint8_t* buf, uint32_t size) {
	PRINT_STR("k_gc_write\n");
	ksceSdifWriteCmd56(ctx, buf, size);
}

void k_gc_cmd56_read(uint8_t* buf, uint32_t size) {
	PRINT_STR("k_gc_read\n");
	ksceSdifReadCmd56(ctx, buf, size);
}

int k_run_authentication(uint16_t key_id) {
	k_clear_secret();
	is_authenticated = 0;
	
	ctx = ksceSdifGetSdContextPartValidateMmc(1);
	PRINT_STR("key_id: %x\n", key_id);
	if(ctx != NULL) {
		vita_cmd56_init(&vita_state, k_gc_cmd56_write, k_gc_cmd56_read); // initalize VITA emu 
		vita_state.allow_prototype_keys = 1;
		
		int ret = vita_cmd56_run(&vita_state);
		
		if(ret == GC_AUTH_OK) {
			PRINT_STR("vita_state.per_cart_keys.packet18_key\n");
			PRINT_BUFFER(vita_state.per_cart_keys.packet18_key);

			PRINT_STR("vita_state.per_cart_keys.packet20_key\n");
			PRINT_BUFFER(vita_state.per_cart_keys.packet20_key);		
			
			is_authenticated = 1;
			return ret;
		}
		else{
			PRINT_STR("ret = 0x%x\n", ret);
			k_clear_secret();
		}
	}
	return POINTER_WAS_NULL;
}


/* user syscalls */

int kIsAuthenticated() {
	return is_authenticated;
}

uint16_t kGetKeyId() {
	return vita_state.key_id;
}

void kGetPerCartKeys(GcCmd56Keys* output) {
	ksceKernelMemcpyKernelToUser(output, (const void*)&vita_state.per_cart_keys, sizeof(vita_state.per_cart_keys));
}

int kClearCartSecret() {
	return k_clear_secret();
}

int kGetCartSecret(uint8_t* keys) {
	uint8_t k_secret[0x20];
	memset(k_secret, 0x00, sizeof(k_secret));
	
	int res = k_get_secret(k_secret);
	if(keys != NULL) ksceKernelMemcpyKernelToUser(keys, (const void*)k_secret, sizeof(k_secret));
		
	return res;
}

int kGetCartHash(uint8_t* hash) {
	uint8_t k_hash[0x14];
	memset(k_hash, 0x00, sizeof(k_hash));
	
	k_get_hash(hash);
	if(hash != NULL) ksceKernelMemcpyKernelToUser(hash, (const void*)k_hash, sizeof(k_hash));
	
}

int kCheckCartHash(const uint8_t* hash) {
	uint8_t k_hash[0x14];
	memset(k_hash, 0x00, sizeof(k_hash));

	if(hash != NULL) ksceKernelMemcpyUserToKernel(k_hash, (const void*)hash, sizeof(k_hash));
	return k_check_hash(k_hash);
}


/* 
*	Swaps the vitas default gamecart auth implementation
* 	-- for a custom one from LibCmd56.
*/

int kEnableGcEmuMgr() {
	PRINT_STR("GcAuthEmuMgr -- VITA mode\n");
	k_clear_secret();
	
	authHook = taiHookFunctionExportForKernel(KERNEL_PID,
		&authHookRef, 
		"SceSblGcAuthMgr",
		0xC6627F5E, // SceSblGcAuthMgrGcAuthForDriver
		0x68781760, // ksceSblGcAuthMgrGcAuthCartAuthentication	
		k_run_authentication);
	PRINT_STR("%x %x\n", authHook, authHookRef);

	getCartSecretHook = taiHookFunctionExportForKernel(KERNEL_PID,
		&getCartSecretHookRef, 
		"SceSblGcAuthMgr",
		0x1926B182, // SceSblGcAuthMgrDrmBBForDriver
		0xBB70DDC0, // ksceSblGcAuthMgrDrmBBGetCartSecret	
		k_get_secret);
	PRINT_STR("%x %x\n", getCartSecretHook, getCartSecretHookRef);

	clearCartSecretHook = taiHookFunctionExportForKernel(KERNEL_PID,
		&clearCartSecretHookRef, 
		"SceSblGcAuthMgr",
		0x1926B182, // SceSblGcAuthMgrDrmBBForDriver
		0xBB451E83, // ksceSblGcAuthMgrDrmBBClearCartSecret	
		k_clear_secret);
	PRINT_STR("%x %x\n", clearCartSecretHook, clearCartSecretHookRef);
	
	checkCartHashHook = taiHookFunctionExportForKernel(KERNEL_PID,
		&checkCartHashHookRef, 
		"SceSblGcAuthMgr",
		0x1926B182, // SceSblGcAuthMgrDrmBBForDriver
		0x22FD5D23, // ksceSblGcAuthMgrDrmBBCheckCartHash	
		k_check_hash);
	PRINT_STR("%x %x\n", checkCartHashHook, checkCartHashHookRef);
	return 0;
}

int kDisableGcEmuMgr() {
	if (authHook >= 0)			  taiHookReleaseForKernel(authHook, authHookRef);
	if (getCartSecretHook >= 0)	  taiHookReleaseForKernel(getCartSecretHook, getCartSecretHookRef);
	if (clearCartSecretHook >= 0) taiHookReleaseForKernel(clearCartSecretHook, clearCartSecretHookRef);
	if (checkCartHashHook >= 0)	  taiHookReleaseForKernel(checkCartHashHook, checkCartHashHookRef);
		
	return 0;
}
