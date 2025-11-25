#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vitasdk.h>

#include "io.h"
#include "device.h"

#include <GcToolKit.h>
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
	kEnableGcEmuMgr();
	
	do {
		
		// check if there is already a GC inserted, if there is 
		// reset the gc device to capture authentication step
		// we, dont do this if there is not a gc inserted, incase someone is using an sd2vita.
		
		if( file_exist("gro0:") || file_exist("grw0:") || device_exist(BLOCK_DEVICE_MEDIAID) ) {
			int res = kResetGc();
			PRINT_STR("kResetGc = %x\n", res);
		}		
				
		sceKernelDelayThread(1000 * 10); // 10ms
	} while(!kIsAuthenticated());
	
	kDisableGcEmuMgr();
}
