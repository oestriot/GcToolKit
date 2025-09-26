#ifndef GC_CONFIG_H
#define GC_CONFIG_H 1
#define CONFIG_SAVE_LOCATION "savedata0:/config.bin"

#define DEFAULT_IP "192.168.1.0"
#define DEFAULT_PORT 46327
#define DEFAULT_FOLDER_NAME "bak"

#include "io.h"

typedef struct GcConfig {
	char ip_address[16];
	uint16_t port;
	char backup_folder[MAX_PATH];
	uint8_t play_music;
} GcConfig;

extern GcConfig CONFIG;

void init_config();
int save_config();

#endif // GC_CONFIG_H