#include <vitasdkkern.h>
#include <stdint.h>
#include <stdio.h>
#include <taihen.h>
#include <GcToolKit.h>
#include "gc.h"
#include "log.h"

static GcInteruptInfo* interupt_info = NULL;

void get_interupt_location() {
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
}

int kGetCardId(int deviceIndex, void* cardId) {
	sd_context_part_mmc* k_deviceInfo = (sd_context_part_mmc*)ksceSdifGetSdContextPartValidateMmc(deviceIndex);	
	if(k_deviceInfo == NULL) return POINTER_WAS_NULL;

	char k_cardId[sizeof(k_deviceInfo->ctxb.CID)];
	memset(k_cardId, 0x00, sizeof(k_cardId));
	memcpy(k_cardId, k_deviceInfo->ctxb.CID, sizeof(k_deviceInfo->ctxb.CID));
	
	PRINT_STR("cardId: ");
	PRINT_BUFFER(k_cardId);
	
	ksceKernelMemcpyKernelToUser(cardId, (const void*)k_cardId, sizeof(k_cardId));	

	return 0;
}

int kGetCardCsd(int deviceIndex, void* cardCsd) {
	sd_context_part_mmc* k_deviceInfo = (sd_context_part_mmc*)ksceSdifGetSdContextPartValidateMmc(deviceIndex);	
	if(k_deviceInfo == NULL) return POINTER_WAS_NULL;
	
	char k_cardCsd[sizeof(k_deviceInfo->ctxb.CSD)];
	memset(k_cardCsd, 0x00, sizeof(k_cardCsd));
	
	memcpy(k_cardCsd, k_deviceInfo->ctxb.CSD, sizeof(k_deviceInfo->ctxb.CSD));
	
	PRINT_STR("cardCsd: ");
	PRINT_BUFFER(k_cardCsd);
	
	ksceKernelMemcpyKernelToUser(cardCsd, (const void*)k_cardCsd, sizeof(k_cardCsd));	

	return 0;
}

int kGetCardExtCsd(int deviceIndex, void* cardExtCsd) {
	sd_context_part_mmc* k_deviceInfo = (sd_context_part_mmc*)ksceSdifGetSdContextPartValidateMmc(deviceIndex);	
	if(k_deviceInfo == NULL) return POINTER_WAS_NULL;
	
	char k_cardExtCsd[sizeof(k_deviceInfo->EXT_CSD)];
	memset(k_cardExtCsd, 0x00, sizeof(k_cardExtCsd));
	
	memcpy(k_cardExtCsd, k_deviceInfo->EXT_CSD, sizeof(k_deviceInfo->EXT_CSD));
	
	PRINT_STR("cardExtCsd: ");
	PRINT_BUFFER(k_cardExtCsd);
	
	ksceKernelMemcpyKernelToUser(cardExtCsd, (const void*)k_cardExtCsd, sizeof(k_cardExtCsd));	

	return 0;
}

int kIsSdInserted() { 
	return (ksceSdifGetSdContextPartValidateSd(1) != NULL);
}

int kIsMmcInserted() { 
	return (ksceSdifGetSdContextPartValidateMmc(1) != NULL);
}

int kResetGc() {
	int res = 0;	
	PRINT_STR("Resetting GC ...\n");
	
	// power down gc slot
	res = ksceSysconCtrlSdPower(0);
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
	
	ksceKernelDelayThread(1000 * 5); // 5ms
	
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