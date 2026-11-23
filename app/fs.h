#ifndef FS_H
#define FS_H 1
#include <vitasdk.h>
#include <limits.h>

typedef SceUID FsFd;

typedef enum FsType {
	FS_TYPE_DISK,
	FS_TYPE_NETWORK,
	FS_TYPE_NULL,
} FsType;

typedef struct FsConfig {
	FsType type;
	char name[MAX_PATH];
	char ip_address[16];
	short port;
	uint64_t total_size;
} FsConfig;

int InitalizeNetFs(const char* ip_address, short port);
int InitalizeDiskFs();
int InitalizeNullFs();
int FsChangeSize(uint64_t sz);

FsFd FsOpen(const char* file, int flags);
int FsWrite(FsFd fd, void* data, size_t sz);
int FsFwrite(FsFd fd, SceOff offset, void* data, size_t sz);
int FsRead(FsFd fd, void* data, size_t sz);
void FsClose(FsFd fd);

#endif // FS_H