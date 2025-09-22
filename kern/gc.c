#include <vitasdkkern.h>
#include <stdint.h>
#include <stdio.h>
#include "GcKernKit.h"
#include "gc.h"
#include "log.h"

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
