#include <string.h>

#include "config.h"
#include "io.h"

GcConfig CONFIG;

void reset_config() {
	memset(&CONFIG, 0x00, sizeof(GcConfig));
	strncpy(CONFIG.ip_address, DEFAULT_IP, sizeof(CONFIG.ip_address));
	strncpy(CONFIG.backup_folder, DEFAULT_FOLDER_NAME, sizeof(CONFIG.backup_folder));
	CONFIG.port = DEFAULT_PORT;
	CONFIG.play_music = 1;

	save_config();
}

void init_config() {
	if(!file_exist(CONFIG_SAVE_LOCATION)) reset_config();
	size_t wr = read_file(CONFIG_SAVE_LOCATION, &CONFIG, sizeof(GcConfig));
	if(wr != sizeof(GcConfig)) { 
		reset_config();
	}
}

int save_config() {
	return write_file(CONFIG_SAVE_LOCATION, &CONFIG, sizeof(GcConfig));	
}