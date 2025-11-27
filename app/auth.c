#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vitasdk.h>

#include "io.h"
#include "device.h"

#include <GcToolKit.h>
#include "kernel.h"
#include "auth.h"
#include "net.h"
#include "err.h"
#include "log.h"


int key_dump_network(char* ip_address, unsigned short port, char* output_file) {
	GcCmd56Keys keys;
	int netwr = -1;
	
	kGetPerCartKeys(&keys);
	
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

	kGetPerCartKeys(&keys);
	
	int wr = write_file(output_file, &keys, sizeof(GcCmd56Keys));
	
	if(wr == 0) return SIZE_IS_ZERO;
	if(wr < 0) return wr;
	if(wr != sizeof(GcCmd56Keys)) return SIZE_NOT_MATCH;

	return 0;
}


void wait_for_gc_auth() {
	// handle case if an sdcard is inserted ...
	while(kIsSdInserted()) { 
		sceKernelDelayThread(1000 * 30); // 10ms
	}
	
	// after that undo sd2vita patches if appliciable ./
	if(!kUndoneSd2VitaPatches() && check_loaded_blacklisted_module() != NULL) {
		//umount_ux0();
		kUndoSd2Vita();
		kResetGc();
	}
	
	// enable emulated gamecart authentication
	kEnableGcEmuMgr();
	
	// reset the current gamecart if there is one already ...
	if( !kIsAuthenticated() && kIsMmcInserted() ) {
		kResetGc();
	}
	
	while(!kIsAuthenticated()) {
		sceKernelDelayThread(1000 * 30); // 10ms
	};

	// disable custom gc handling ..
	kDisableGcEmuMgr();
	
}
