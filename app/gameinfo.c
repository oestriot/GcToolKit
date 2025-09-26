#include <vitasdk.h>
#include <string.h>
#include <stdio.h>

#include "gameinfo.h"
#include "io.h"
#include "sfo.h"
#include "log.h"

#define TITLE_ID_SIZE 12
#define DEFAULT_TITLEID "NPXS99999"
#define DEFAULT_TITLE "UNKNOWN"


int read_gameinfo(char* title_id, char* title, size_t length) {
	wait_for_partition("gro0:");
	// get title id from gro0:/app folder
	int res = read_first_filename("gro0:/app", title_id, TITLE_ID_SIZE);
	PRINT_STR("read_title_id: = %x\n", res);

	if(res >= 0) {
		char param_sfo_path[MAX_PATH];
		snprintf(param_sfo_path, sizeof(param_sfo_path), "gro0:/app/%s/sce_sys/param.sfo", title_id);
		res = read_sfo_key(param_sfo_path, "STITLE", title, length-1);
		if(res == SFO_KEY_NOT_FOUND) // not found
			res = read_sfo_key(param_sfo_path, "TITLE", title, length-1);
		
		read_sfo_key(param_sfo_path, "TITLE_ID", title_id, length-1);
		
		PRINT_STR("read_title: = %x\n", res);
	}
	
	if(res < 0) {
		strncpy(title_id, DEFAULT_TITLEID, length-1);
		strncpy(title, DEFAULT_TITLE, length-1);
	}
	return res;
}