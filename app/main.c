#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <taihen.h>
#include <vitasdk.h>
#include <vita2d.h>

#include "lock.h"
#include "ctrl.h"
#include "auth.h"
#include "gameinfo.h"
#include "kernel.h"
#include "draw.h"
#include "menu.h"
#include "ime.h"
#include "device.h"
#include "net.h"
#include "bgm.h"
#include "log.h"
#include "config.h"

#include <GcToolKit.h>


void get_output_filename(char* output, char* extension, int size_output) {
	char title_id[64];
	char title[64];
	read_gameinfo(title_id, title, sizeof(title));
	
	snprintf(output, size_output, "%s [%s].%s", title, title_id, extension);
	remove_illegal_chars(output);
	return;
}


int handle_dump_device(int what, BackupFormat format, char* block_device, char* out_file, char* ip_address, unsigned short port) {
	int res = -1;
	
	if(what != DUMP_KEYS_ONLY) {
		res = do_device_dump(block_device, out_file, format, ip_address, port);				
	}
	else if(ip_address == NULL) {		
		res = key_dump(out_file);
	}
	else {
		res = key_dump_network(ip_address, port, out_file);		
	}

	// dump keys if format is not VCI ..
	if(what == DUMP_WHOLE_GC && !FMT_IS_VCI(format)) {
		change_extension(out_file, MAX_PATH, ".keys.bin");
		if(ip_address == NULL) { // is this a network dump?
			res = key_dump(out_file);
		}
		else {
			res = key_dump_network(ip_address, port, out_file);		
		}		
	}
	
	if(res < 0) {
		do_error(res);
	}
	else {
		char msg[MAX_PATH];
		snprintf(msg, sizeof(msg)-1, "Backup created at: %s ...", out_file);
		do_confirm_message("Backup Success!", msg);
	}

	return res;
}

int handle_menu_set_network_options(int what, BackupFormat format, char* block_device, char* outfile) {

	int selected = -1;
	while(1) {
		selected = do_network_options(CONFIG.ip_address, CONFIG.port);
		switch(selected) {
			case CHANGE_IP:
				open_ime("Enter IP", CONFIG.ip_address, sizeof(CONFIG.ip_address)-1);
				PRINT_STR("ip address: %s\n", CONFIG.ip_address);
				if(!check_ip_address_valid(CONFIG.ip_address)) {
					strncpy(CONFIG.ip_address, DEFAULT_IP, sizeof(CONFIG.ip_address)-1);
				}
				
				continue;
				break;
			case CHANGE_PORT:
				open_ime_short("Enter PORT", &CONFIG.port);
				continue;
				break;
			case START_DUMP:
				break;
			case OP_CANCELED:
				return OP_CANCELED;
		}
		
		break;
	};
	
	save_config();
	
	return handle_dump_device(what, format, block_device, outfile, CONFIG.ip_address, CONFIG.port);
}


	
void handle_menu_set_output(char* ext, BackupFormat format, int what) {
		// determine block device		
		PRINT_STR("handle_menu_set_output\n");
		
		// get output_folder
		int res = 0;
		char output_folder[MAX_PATH];
		char output_filename[MAX_PATH/2];

		char* block_device = NULL;
		char* output_device = NULL;
		
		switch(what) {
			case DUMP_WHOLE_GC:
				block_device = BLOCK_DEVICE_GC;
				break;
			case DUMP_MEDIAID:
				block_device = BLOCK_DEVICE_MEDIAID;
				break;
			case DUMP_GRW0:
				block_device = BLOCK_DEVICE_GRW0;
				break;
			case DUMP_KEYS_ONLY:
				break;

		}

		PRINT_STR("block_device: %s\n", block_device);

		// get filename
		get_output_filename(output_filename, ext, sizeof(output_filename));		
		PRINT_STR("output_filename: %s\n", output_filename);

		// get required space for the file
		uint64_t required_space = sizeof(GcCmd56Keys);
		if(block_device != NULL) required_space = get_device_size(block_device);
		PRINT_STR("required_space %llx\n", required_space);
		
		int selected = -1;
		do {
			selected = do_select_output_location(output_filename, required_space);
			
			switch(selected) {
				case DUMP_LOCATION_UX0:
					output_device = "ux0:";
					break;
				case DUMP_LOCATION_XMC:
					output_device = "xmc0:";
					break;
				case DUMP_LOCATION_UMA:
					output_device = "uma0:";
					break;
				case DUMP_LOCATION_HOST:
					output_device = "host0:";
					break;
				case DUMP_LOCATION_NET:
					output_device = "";
					break;
				case CHANGE_FILENAME:
					open_ime("Enter filename", output_filename, sizeof(output_filename));
					remove_illegal_chars(output_filename);
					continue;
					break;
				case O_RELOAD_DEVICES:
					continue;
					break;
				case OP_CANCELED:
					return;
					break;
				default:
					return;
					break;
			};
			PRINT_STR("output_device %s\n", output_device);

			if(selected != DUMP_LOCATION_NET) {
				// get full output path, (eg, ux0:bak/game.vci)
				snprintf(output_folder, sizeof(output_folder)-1, "%s%s/%s", output_device, CONFIG.backup_folder, output_filename);
				PRINT_STR("DUMP PHYSICAL : what = %x, format = %x, output_folder = %s\n", what, format, output_folder);

				res = handle_dump_device(what, format, block_device, output_folder, NULL, 0);
				if(res == OP_CANCELED) break;
			}			
			else {
				res = handle_menu_set_network_options(what, format, block_device, output_filename);
				if(res == OP_CANCELED) continue;
			}			

			break;
		} while(1);

		return;
}

int handle_format_confirm_and_format(const char* block_device) {
	
	int selected = do_format_confirm(block_device);
	int full = 0;
	int format = 1;
	
	if(selected == FULL_FORMAT) full = 1;
	if(selected == QUICK_FORMAT) full = 0;
	if(selected == OP_CANCELED || selected == CANCEL_FORMAT) return OP_CANCELED;
	
	int res = do_device_wipe_and_format(block_device, full, format);
		
	return res;
}

void handle_wipe_option(int what) {
	const char* block_device = NULL;
	switch(what) {
		case RESET_MEDIAID:
			block_device = BLOCK_DEVICE_MEDIAID;
			break;
		case RESET_GRW0:
			block_device = BLOCK_DEVICE_GRW0;
			break;
		case OP_CANCELED:
			return;
		default:
			return;
	}
		
	int res = 0;
	
	if(what == RESET_GRW0) res = handle_format_confirm_and_format(block_device);
	else if(what == RESET_MEDIAID) res = do_device_wipe_and_format(block_device, 1, 0);
	if(res == OP_CANCELED) return;
	
	char msg[MAX_PATH];	
	if(res < 0) {
		do_error(res);
	}
	else{
		snprintf(msg, sizeof(msg)-1, "Formatted: \"%s\" ...", block_device);
		do_confirm_message("Format Success!", msg);
	}	
	
	return;
}

void handle_select_file(int what, char* folder) {
	char* block_device = NULL;
	char* extension = NULL;
	switch(what) {
		case WRITE_MEDIAID:
			block_device = BLOCK_DEVICE_MEDIAID;
			extension = ".mediaid";
			break;
		case WRITE_GRW0:
			block_device = BLOCK_DEVICE_GRW0;
			extension = ".img";
			break;
		case OP_CANCELED:
			return;
		default:
			return;
	}
	
	// get total size
	uint64_t total_device_size = 0;
	if(block_device != NULL) total_device_size = get_device_size(block_device);
	PRINT_STR("total_device_size %llx\n", total_device_size);
	
	// show file selection
	char file[MAX_PATH];
	int selected = do_select_file(folder, file, extension, total_device_size);
	if(selected == OP_CANCELED)
	if(selected < 0) {
		do_error(selected);
		return;
	}
	
	char input_file[MAX_PATH];
	snprintf(input_file, sizeof(input_file)-1, "%s/%s", folder, file);
	
	int res = do_device_restore(block_device, input_file);
	
	if(res < 0) {
		do_error(res);
	}
	else{
		char msg[MAX_PATH];	
		snprintf(msg, sizeof(msg)-1, "Restored from: %s ...", input_file);
		do_confirm_message("Restore Success!", msg);
	}
	
	return;
}

void handle_select_input_device(int what) {

	char* input_device = NULL;
	int selected = -1;
	while(1) {
		selected = do_select_input_location();
		
		switch(selected) {
			case RESTORE_LOCATION_UX0:
				input_device = "ux0:";
				break;
			case RESTORE_LOCATION_XMC:
				input_device = "xmc0:";
				break;
			case RESTORE_LOCATION_UMA:
				input_device = "uma0:";
				break;
			case RESTORE_LOCATION_HOST:
				input_device = "host0:";
				break;
			case I_RELOAD_DEVICES:
				continue;
				break;
			case OP_CANCELED:
				return;
			default:
				break;
		};
		
		break;
	};
	
	char input_folder[MAX_PATH];
	snprintf(input_folder, sizeof(input_folder), "%s%s", input_device, CONFIG.backup_folder);
	make_directories(input_folder);
	
	// get input file
	handle_select_file(what, input_folder);
	return;
}

void handle_menu_select_backup_format(int what) {
	BackupFormat format = BACKUP_FORMAT_VCI;
	char* ext = "vci";
	int selected = -1;
	while(1) {
		selected = do_select_backup_format();
		switch(selected) {
			case SELECT_FMT_VCI:
				format = BACKUP_FORMAT_VCI;
				ext = "vci";
				break;
			case SELECT_FMT_VCI_TRIM:
				format = BACKUP_FORMAT_VCI_TRIM;
				ext = "trim.vci";
				break;
			case SELECT_FMT_PSV:
				format = BACKUP_FORMAT_PSV;
				ext = "psv";
				break;
			case SELECT_FMT_PSV_TRIM:
				format = BACKUP_FORMAT_PSV_TRIM;
				ext = "trim.psv";
				break;
			case SELECT_FMT_RAW:
				format = BACKUP_FORMAT_RAW;
				ext = "img";
				break;
			case OP_CANCELED:
				return;
			default:
				break;
		};
		
		handle_menu_set_output(ext, format, what);
		
	}
	
	return;
}

void handle_menu_select_option() {
	
	char* ext = "";
	int selected = do_gc_options();
	switch(selected) {
		case DUMP_WHOLE_GC:
			ext = "vci";
			break;
		case DUMP_KEYS_ONLY:
			ext = "keys.bin";
			break;
			
		case DUMP_MEDIAID:
			ext = "mediaid";
			break;
		case WRITE_MEDIAID:
			ext = "mediaid";
			break;
		case RESET_MEDIAID:
			break;
			
		case DUMP_GRW0:
			ext = "img";
			break;
		case WRITE_GRW0:
			ext = "img";
			break;
		case RESET_GRW0:
			break;
		case GET_GC_INFO:
			break;
		default:
			break;
	};

	if(selected == DUMP_WHOLE_GC) 
		handle_menu_select_backup_format(selected);
	if(selected == DUMP_KEYS_ONLY || selected == DUMP_MEDIAID || selected == DUMP_GRW0)
		handle_menu_set_output(ext, BACKUP_FORMAT_RAW, selected);
	if(selected == RESET_MEDIAID || selected == RESET_GRW0)
		handle_wipe_option(selected);
	if(selected == WRITE_MEDIAID || selected == WRITE_GRW0)
		handle_select_input_device(selected);
	if(selected == GET_GC_INFO)
		do_device_info();
	
	return;
}

int main(int argc, char** argv) {
	int has_restarted = (argc >= 2 && argv != NULL && strcmp(argv[1], "-restarted") == 0);
	const char* blacklisted_module = check_loaded_blacklisted_module();

	PRINT_STR("has_restarted: 0x%x\n", has_restarted);
	
	if(!has_restarted){
		load_kernel_modules();
	}
	
	init_config();
	init_ctrl();
	init_vita2d();
	init_menus();
	init_network();
	init_sound();
	init_shell();
	
	if(blacklisted_module != NULL) {
		do_blacklisted_module_message(blacklisted_module);
	}
	else if(!is_module_started(KMODULE_NAME)) {
		do_kmodule_failed_message(KMODULE_NAME);
	}
	else {
		while(1) {
			do_gc_insert_prompt();
			handle_menu_select_option();
		}		
	}
		
	
	term_shell();
	term_sound();
	term_menus();
	term_vita2d();
	term_network();
	return 0;
}
