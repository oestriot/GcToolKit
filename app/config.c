#include <string.h>
#include <vitasdk.h>

#include "config.h"
#include "io.h"

GcConfig CONFIG;

int is_music_enabled() {
	if(CONFIG.music_setting == MUSIC_USE_SYSTEM_SETTING) {
		int bgm = 0;
		int res = sceRegMgrGetKeyInt("/CONFIG/SHELL", "bgm", &bgm);
		
		if(res < 0) return 0;
		return bgm;
	}
	else if(CONFIG.music_setting == MUSIC_ENABLE) {
		return 1;
	}
	else if(CONFIG.music_setting == MUSIC_DISABLE) {
		return 0;
	}
	
	return 0;
	
}

int is_circle_confirm() {
	if(CONFIG.confirm_setting == CONFIRM_USE_SYSTEM_SETTING) {
		int cross_confirm = 0;
		int res = sceRegMgrGetKeyInt("/CONFIG/SYSTEM", "button_assign", &cross_confirm);
		
		if(res < 0) return 0;
		return !cross_confirm;
	}
	else if(CONFIG.confirm_setting == CONFIRM_CIRCLE) {
		return 1;
	}
	else if(CONFIG.confirm_setting == CONFIRM_CROSS) {
		return 0;
	}
	
	return 0;
}


void reset_config() {
	memset(&CONFIG, 0x00, sizeof(GcConfig));
	CONFIG.major_version = GCTK_MAJOR_VERISON;
	CONFIG.minor_version = GCTK_MINOR_VERSION;
	
	strncpy(CONFIG.ip_address, DEFAULT_IP, sizeof(CONFIG.ip_address));
	strncpy(CONFIG.backup_folder, DEFAULT_FOLDER_NAME, sizeof(CONFIG.backup_folder));
	CONFIG.port = DEFAULT_PORT;
	CONFIG.music_setting = MUSIC_USE_SYSTEM_SETTING;
	CONFIG.confirm_setting = CONFIRM_USE_SYSTEM_SETTING;

	save_config();
}

void init_config() {
	if(!file_exist(CONFIG_SAVE_LOCATION)) reset_config();
	size_t wr = read_file(CONFIG_SAVE_LOCATION, &CONFIG, sizeof(GcConfig));
	if(wr != sizeof(GcConfig) || CONFIG.major_version < GCTK_MAJOR_VERISON) { 
		reset_config();
	}
}

int save_config() {
	return write_file(CONFIG_SAVE_LOCATION, &CONFIG, sizeof(GcConfig));	
}