#include <vitasdk.h>
#include <string.h>
#include <stdio.h>

#include "gameinfo.h"
#include "io.h"
#include "sfo.h"
#include "log.h"
#include "err.h"

#define SPSFO_PATH "gro0:gc/param.sfo"

#define TITLE_ID_SIZE 12
#define DEFAULT_TITLEID "NPXS99999"
#define DEFAULT_TITLE "UNKNOWN"

#define IS_SET(v) (strncmp(v, "", length) != 0)


static const char* PARAM_LOCATIONS[] = { 
	"gro0:/patch",
	"grw0:/patch",
	"gro0:/app",
	"gro0:/appinst",
	"gro0:/acinst",
	NULL,
};


// reads a title id from the SpSfo on the cartridge.
void parse_sp_sfo(char* title_id, char* title, size_t length) {
	PRINT_STR("parsing sp sfo.\n");
	if(IS_SET(title_id) && IS_SET(title)) return; 	// skip if already set.

	SfoHandle sfo;
	if(open_sp_sfo(SPSFO_PATH, &sfo) == OK) {
		
		if(!IS_SET(title)) read_sfo_key(&sfo, "ACINST_TITLE", title, length-1);
		if(!IS_SET(title)) read_sfo_key(&sfo, "APPINST_TITLE", title, length-1);
		if(!IS_SET(title_id)) read_sfo_key(&sfo, "TITLE_ID", title_id, length-1);
		
		close_sfo(&sfo);
	}
}

void parse_app_sfo(char* title_id, char* title, size_t length) {
	PRINT_STR("parsing app sfo.\n");
	if(IS_SET(title_id) && IS_SET(title)) return; 	// skip if already set.
	
	char next_folder[MAX_PATH] = { 0 };
	SfoHandle sfo;
	
	// check each location there could be a SFO at.
	for(int i = 0; PARAM_LOCATIONS[i] != NULL; i++) {
		PRINT_STR("location = %s\n", PARAM_LOCATIONS[i]);
		if(read_first_filename(PARAM_LOCATIONS[i], next_folder, sizeof(next_folder)) >= SCE_OK) {
			PRINT_STR("next_folder = %s\n", next_folder);
			
			char param_sfo_path[MAX_PATH];
			snprintf(param_sfo_path, sizeof(param_sfo_path), "%s/%s/sce_sys/param.sfo", PARAM_LOCATIONS[i], next_folder);
			PRINT_STR("param_sfo_path: = %s\n", param_sfo_path);
			
			if(open_sfo(param_sfo_path, &sfo) == OK) {
				
				if(!IS_SET(title)) { // get title .. 
					if(read_sfo_key(&sfo, "STITLE", title, length-1) != OK) {
						read_sfo_key(&sfo, "TITLE", title, length-1);							
					}
				}

				// get title id
				if(!IS_SET(title_id)) read_sfo_key(&sfo, "TITLE_ID", title_id, length-1);
									
				close_sfo(&sfo);
			}
		}
	}
}

void read_gameinfo(char* title_id, char* title, size_t length) {
	if(length < 0) return;
	if(title == NULL || title_id == NULL) return;

	PRINT_STR("read_gameinfo: %p %p %zu\n", title_id, title, length);

	// set both to 0
	memset(title_id, 0x00, length);
	memset(title, 0x00, length);

	mount_gamecart();
	
	// get title or titleid from cartridge 
	parse_sp_sfo(title_id, title, length);
	parse_app_sfo(title_id, title, length);
	
	// fallback on defaults if nothing worked.
	if(!IS_SET(title_id)) strncpy(title_id, DEFAULT_TITLEID, length-1);
	if(!IS_SET(title)) strncpy(title, DEFAULT_TITLE, length-1);
	
}