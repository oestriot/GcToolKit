#include <stdint.h>
#include <stdio.h>

typedef struct GcInteruptInfo{
	SceUID request_id;
	SceUID op_sync_id;
	char unk[0x20];
} GcInteruptInfo;

int cmd56_patch();
int cmd56_unpatch();