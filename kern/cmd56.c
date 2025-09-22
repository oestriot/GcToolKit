#include <stdint.h>
#include <stdio.h>
#include <taihen.h>
#include <vitasdkkern.h>
#include "GcKernKit.h"
#include "cmd56.h"
#include "log.h"
#include "mod.h"

// from taihen module.h
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

// static variables
static uint8_t LAST_CAPTURED_CMD20_INPUT[0x116];
static uint32_t LAST_CAPTURED_CMD20_KEYID = 0;
static uint8_t HAS_CAPTURED_CMD20 = 0;

// prototype keyid check patch
static int proto_keyid_check_inject = -1;

// hooks for capturing GC comms
static int ksceSblSmCommCallFuncHook = -1;
static tai_hook_ref_t ksceSblSmCommCallFuncHookRef;

GcInteruptInfo* interupt_info = NULL;

int ksceSblSmCommCallFunc_patch(SceSblSmCommId id, SceUInt32 service_id, SceUInt32 *service_result, SceSblSmCommGcData *data, SceSize size) {
	if(data->command == 0x20) {
		PRINT_STR("copy auth input, data->data %x\n", data->data);
		memcpy(LAST_CAPTURED_CMD20_INPUT, data->data, sizeof(LAST_CAPTURED_CMD20_INPUT));
		
		PRINT_STR("keyid %x\n", data->key_id);
		LAST_CAPTURED_CMD20_KEYID = data->key_id;
		HAS_CAPTURED_CMD20 = 1;
	}
	
	return TAI_CONTINUE(int, ksceSblSmCommCallFuncHookRef, id, service_id, service_result, data, size);
}

int cmd56_patch() {
	memset(LAST_CAPTURED_CMD20_INPUT, 0x00, sizeof(LAST_CAPTURED_CMD20_INPUT));
	
	tai_module_info_t gc_authmgr_info;
	gc_authmgr_info.size = sizeof(tai_module_info_t);
	int gcauthmgr_get_info = taiGetModuleInfoForKernel(KERNEL_PID, "SceSblGcAuthMgr", &gc_authmgr_info);
	int gcauthmgr_version = check_module_version("os0:/kd/gcauthmgr.skprx");
	
	PRINT_STR("get module SceSblGcAuthMgr 0x%04X (version: 0x%x)\n", gcauthmgr_get_info, gcauthmgr_version);
	
	if(gcauthmgr_get_info >= 0) {
		// prototype game carts use key ids != 1,
		// 3.60 firmware checks if ((key_id & 0xffff7fff) == 1)
		// it would be nice if we could dump prototype gamecarts tho ..
	
		uint16_t nop_instruction = 0xBF00;
		
		if(gcauthmgr_version >= 0x363) {
			proto_keyid_check_inject = taiInjectDataForKernel(KERNEL_PID, gc_authmgr_info.modid, 0, 0x95F8, &nop_instruction, sizeof(uint16_t));
		}
		else {
			proto_keyid_check_inject = taiInjectDataForKernel(KERNEL_PID, gc_authmgr_info.modid, 0, 0x9376, &nop_instruction, sizeof(uint16_t));
		}

		
		PRINT_STR("proto_keyid_check_inject 0x%04X\n", proto_keyid_check_inject);
		
		// capture sm communications
		ksceSblSmCommCallFuncHook = taiHookFunctionImportForKernel(KERNEL_PID,
			&ksceSblSmCommCallFuncHookRef, 
			"SceSblGcAuthMgr", 
			0xCD3C89B6, // SceSblSmCommForKernel
			0xDB9FC204, // ksceSblSmCommCallFunc
			ksceSblSmCommCallFunc_patch);
		
		PRINT_STR("ksceSblSmCommCallFuncHook 0x%04X\n", ksceSblSmCommCallFuncHook);
		PRINT_STR("ksceSblSmCommCallFuncHookRef 0x%04X\n", ksceSblSmCommCallFuncHookRef);
	}
	
	tai_module_info_t sdstor_info;
	sdstor_info.size = sizeof(tai_module_info_t);
	int res = taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info);
	PRINT_STR("get module SceSdstor 0x%04X\n", res);
	
	if(res >= 0){
		PRINT_STR("sdstor_info.modid 0x%04X\n", sdstor_info.modid);
		res = module_get_offset(KERNEL_PID, sdstor_info.modid, 1, 0x1B24, (uintptr_t*)&interupt_info);
		
		PRINT_STR("module_get_offset sdstor_info 0x%04X\n", res);
		PRINT_STR("interupt_info 0x%04X\n", interupt_info);
	}
	return res;	
}

int cmd56_unpatch() {
	if (ksceSblSmCommCallFuncHook >= 0)		 taiHookReleaseForKernel(ksceSblSmCommCallFuncHook, ksceSblSmCommCallFuncHookRef);
	if (proto_keyid_check_inject >= 0)		 taiInjectReleaseForKernel(proto_keyid_check_inject);
	return 0;
}

// user syscalls

int kResetCmd20Input() {
	// remove the flag saying we have captured one so another GC can have data captured.
	memset(LAST_CAPTURED_CMD20_INPUT, 0x00, sizeof(LAST_CAPTURED_CMD20_INPUT));
	HAS_CAPTURED_CMD20 = 0;
	return 0;
}

int kGetLastCmd20Input(void* cmd20_input) {
	ksceKernelMemcpyKernelToUser(cmd20_input, (const void*)LAST_CAPTURED_CMD20_INPUT, sizeof(LAST_CAPTURED_CMD20_INPUT));
	return 0;
}

int kGetLastCmd20KeyId() {
	return LAST_CAPTURED_CMD20_KEYID;
}

int kHasCmd20Captured() {
	return HAS_CAPTURED_CMD20;
}

int kClearCartSecret() {
	return ksceSblGcAuthMgrDrmBBClearCartSecret();
}

int kGetCartSecret(uint8_t* keys) {
	uint8_t k_keys[0x20];
	memset(k_keys, 0x00, sizeof(k_keys));
	
	int res = ksceSblGcAuthMgrDrmBBGetCartSecret(k_keys);
	if(keys != NULL) ksceKernelMemcpyKernelToUser(keys, (const void*)k_keys, sizeof(k_keys));
		
	return res;
}


int kResetGc() {
	if(interupt_info != NULL) {
		PRINT_STR("Resetting GC ...\n");
		
		// power down gc slot
		int res = ksceSysconCtrlSdPower(0);
		PRINT_STR("ksceSysconCtrlSdPower(0) 0x%04X\n", res);
		if(res < 0) return res;
		
		// trigger gc remove interupt	
		res = ksceKernelSetEventFlag(interupt_info[1].request_id, 0x100);
		PRINT_STR("ksceKernelSetEventFlag(0x%02X, 0x100) 0x%04X\n", interupt_info[1].request_id, res);
		if(res < 0) return res;
		
		// wait for event to finish.
		res = ksceKernelWaitEventFlag(interupt_info[1].op_sync_id, 0x100,5,0,0);
		PRINT_STR("ksceKernelWaitEventFlag(0x%02X, 0x100,5,0,0) 0x%04X\n", interupt_info[1].op_sync_id, res);
		if(res < 0) return res;
		
		// power up gc slot
		res = ksceSysconCtrlSdPower(1);
		PRINT_STR("ksceSysconCtrlSdPower(1) 0x%04X\n", res);
		if(res < 0) return res;
		
		// trigger gc insert interupt	
		res = ksceKernelSetEventFlag(interupt_info[1].request_id, 0x1000);
		PRINT_STR("ksceKernelSetEventFlag(0x%02X, 0x1000) 0x%04X\n", interupt_info[1].request_id, res);
		if(res < 0) return res;
		
		// wait for event to finish
		res = ksceKernelWaitEventFlag(interupt_info[1].op_sync_id,0x1000,5,0,0);
		PRINT_STR("ksceKernelWaitEventFlag(0x%02X, 0x1000,5,0,0) 0x%04X\n", interupt_info[1].op_sync_id, res);
		return res;
	}
	return INTER_INFO_WAS_NULL;
}
