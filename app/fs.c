#include <string.h>
#include <stdint.h>
#include <vitasdk.h>
#include "net.h"
#include "fs.h"

static FsConfig cfg;

int InitalizeNetFs(const char* ip_address, short port) {
	memset(&cfg, 0x00, sizeof(FsConfig));
	strncpy(cfg.ip_address, ip_address, sizeof(cfg.ip_address)-1);
	cfg.port = port;
	cfg.type = FS_TYPE_NETWORK;
	return 0;
}

int InitalizeDiskFs() {
	memset(&cfg, 0x00, sizeof(FsConfig));
	cfg.type = FS_TYPE_DISK;
	return 0;
}

int InitalizeNullFs() {
	memset(&cfg, 0x00, sizeof(FsConfig));
	cfg.type = FS_TYPE_NULL;
	return 0;
}
int FsChangeSize(uint64_t sz) {
	cfg.total_size = sz;
	return 0;
}
inline FsFd FsOpen(const char* file, int flags) {
	strncpy(cfg.name, file, sizeof(cfg.name)-1);
	switch(cfg.type) {
		case FS_TYPE_NETWORK:
			FsFd fd = begin_connection(cfg.ip_address, cfg.port);
			if(fd >= 0) begin_file_send(fd, file, cfg.total_size);
			return fd;
		case FS_TYPE_DISK:
			return sceIoOpen(file, flags, 0777);
		case FS_TYPE_NULL:
			return 1;
	}
	return UNSUPPORTED_OPERATION;
}

inline int FsWrite(FsFd fd, void* data, size_t sz) {
	switch(cfg.type) {
		case FS_TYPE_NETWORK:
			return file_send_data(fd, data, sz);
		case FS_TYPE_NULL:
			memset(data, 0x00, sz);
		case FS_TYPE_DISK:
			return sceIoWrite(fd, data, sz);
	}
	return UNSUPPORTED_OPERATION;

}

inline int FsFwrite(FsFd fd, SceOff offset, void* data, size_t sz) {	
	switch(cfg.type) {
		case FS_TYPE_NETWORK:
			int wr = send_file_patch(fd, cfg.name, offset, data, sz);
			if(wr < 0) return wr;
			if(wr == 0) return SIZE_IS_ZERO;
			return sz;
		case FS_TYPE_NULL:
			memset(data, 0x00, sz);			
		case FS_TYPE_DISK:
			SceOff off = sceIoLseek(fd, offset, SCE_SEEK_SET);
			if(off == offset) return sceIoWrite(fd, data, sz);
			else if(off < 0) return off;
			else if(off == 0) return SIZE_IS_ZERO;
			else return SIZE_NOT_MATCH;				
	}
	return UNSUPPORTED_OPERATION;
}
inline int FsRead(FsFd fd, void* data, size_t sz) {
	switch(cfg.type) {
		case FS_TYPE_NETWORK:
			return UNSUPPORTED_OPERATION;
		case FS_TYPE_DISK:
			return sceIoRead(fd, data, sz);
		case FS_TYPE_NULL:
			memset(data, 0x00, sz);
			return sz;
	}
	return UNSUPPORTED_OPERATION;
}

inline void FsClose(FsFd fd) {
	switch(cfg.type) {
		case FS_TYPE_NETWORK:
			end_connection(fd);
			break;
		case FS_TYPE_DISK:
			sceIoClose(fd);
			break;
		case FS_TYPE_NULL:
			return;
	}
}