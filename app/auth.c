#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vitasdk.h>

#include "io.h"
#include "lock.h"
#include "device.h"

#include <GcToolKit.h>
#include "kernel.h"
#include "auth.h"
#include "net.h"
#include "err.h"
#include "log.h"

int key_dump_network(char* ip_address, unsigned short port, char* output_file) {
	PRINT_FUNC();
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
	PRINT_FUNC();
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
	PRINT_STR("waiting for authentication...\n");
	
	// enable emulated gamecart authentication
	PRINT_STR("enabling GcEmuMgr ...\n");
	kEnableGcEmuMgr();
	
	// reset the current gamecart if there is one already ...
	if( !kIsAuthenticated() && kIsMmcInserted() ) {
		PRINT_STR("resetting gamecart slot ...\n");
		kResetGc();
	}
	
	// wait for gamecart authentication ..
	while(!kIsAuthenticated()) {
		sceKernelDelayThread(1000 * 30); // 10ms
	};

	// revert to regular gc authentication ..
	PRINT_STR("disabling GcEmuMgr ...\n");
	kDisableGcEmuMgr();
	
	PRINT_STR("done ...\n");
}
