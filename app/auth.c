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
#include "err.h"
#include "log.h"
#include "fs.h"

int key_dump(char* output_file) {
	int res = -1;
	PRINT_FUNC();
	
	make_directories_excluding_last(output_file);
	FsChangeSize(sizeof(GcCmd56Keys));
	
	SceUID fd = FsOpen(output_file, SCE_O_WRONLY);
	PRINT_STR("fd = %x\n", fd);
	if(fd < 0) ERROR(fd);
	
	GcCmd56Keys keys;
	kGetPerCartKeys(&keys);
	int wr = FsWrite(fd, &keys, sizeof(GcCmd56Keys));
	if(wr == 0) ERROR(SIZE_IS_ZERO);
	if(wr < 0) ERROR(wr);
	if(wr != sizeof(GcCmd56Keys)) ERROR(SIZE_NOT_MATCH);

error:
	if(fd >= 0) FsClose(fd);
	
	return res;
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
