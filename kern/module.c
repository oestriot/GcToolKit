#include <vitasdkkern.h>
#include <stdint.h>
#include "log.h"

int check_module_version(const char* module) {
	int version = 0x000;
	
	SceUID fd = ksceIoOpen(module, SCE_O_RDONLY, 0777);
	ksceIoLseek(fd, 0x94, SCE_SEEK_SET);
	ksceIoRead(fd, &version, sizeof(version));
	ksceIoClose(fd);
	
	PRINT_STR("%s version: %x\n", module, version);
	
	return version;
}
