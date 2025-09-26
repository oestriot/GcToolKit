#include <vitasdk.h>
#include <string.h>
#include <stdio.h>

#include "gameinfo.h"
#include "io.h"
#include "sfo.h"
#include "log.h"

#define DEFAULT_TITLEID "NPXS99999"
#define DEFAULT_TITLE "UNKNOWN"

int read_titleid(char* title_id) {
	return read_first_filename("gro0:/app", title_id, 12);
}

int read_gameinfo(char* title_id, char* title) {
	wait_for_partition("gro0:");
	// get title id from gro0:/app folder
	int res = read_titleid(title_id);
	PRINT_STR("read_title_id: = %x\n", res);

	if(res >= 0) {
		char param_sfo_path[MAX_PATH];
		snprintf(param_sfo_path, sizeof(param_sfo_path), "gro0:/app/%s/sce_sys/param.sfo", title_id);
		res = read_sfo_key("STITLE", title, param_sfo_path);
		if(res == -9) // not found
			res = read_sfo_key("TITLE", title, param_sfo_path);
		
		PRINT_STR("read_title: = %x\n", res);
	}
	
	if(res < 0) {
		strncpy(title_id, DEFAULT_TITLEID, 64);
		strncpy(title, DEFAULT_TITLE, 64);
	}
	return res;
}