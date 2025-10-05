#ifndef GC_CONFIG_H
#define GC_CONFIG_H 1

#define CONFIG_SAVE_LOCATION "savedata0:/config.bin"

#define DEFAULT_IP "192.168.1.0"
#define DEFAULT_PORT 46327
#define DEFAULT_FOLDER_NAME "bak"

#define GCTK_MAJOR_VERISON 1
#define GCTK_MINOR_VERSION 0

#include "io.h"

typedef enum GcMusicSetting {
	MUSIC_USE_SYSTEM_SETTING,
	MUSIC_ENABLE,
	MUSIC_DISABLE
} GcMusicSetting;

typedef enum GcConfirmButtonSetting {
	CONFIRM_USE_SYSTEM_SETTING,
	CONFIRM_CIRCLE,
	CONFIRM_CROSS
} GcConfirmButtonSetting;

typedef struct GcConfig {
	uint16_t major_version;
	uint16_t minor_version;
	
	char ip_address[16];
	uint16_t port;
	char backup_folder[MAX_PATH];
	GcMusicSetting music_setting;
	GcConfirmButtonSetting confirm_setting;
} GcConfig;

extern GcConfig CONFIG;

int is_music_enabled();
int is_circle_confirm();

void init_config();
int save_config();

#endif // GC_CONFIG_H