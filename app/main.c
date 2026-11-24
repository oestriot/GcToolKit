#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <taihen.h>
#include <vitasdk.h>
#include <vita2d.h>
#include <GcToolKit.h>

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
#include "sd2vita.h"
#include "config.h"
#include "gc_ident.h"
#include "fs.h"


void get_output_filename(char* output, const char* extension, size_t number, size_t size_output) {
	char title_id[64];
	char title[64];
	read_gameinfo(title_id, title, sizeof(title));
	
	if(number > 0) {
		snprintf(output, size_output, "%s [%s] (%02d)%s", title, title_id, number, extension);
	}
	else {
		snprintf(output, size_output, "%s [%s]%s", title, title_id, extension);
	}
	
	remove_illegal_chars(output);	
	return;
}


int handle_dump_device(BackupWhat what, BackupFormat format, const char* block_device, char* out_file, char* ip_address, unsigned short port) {
	int res = -1;
	char extra_file[MAX_PATH] = { 0 };
	strncpy(extra_file, out_file, sizeof(extra_file)-1);
	
	if(what == WHT_CMD56_KEYS) {
		res = do_dump_cmd56_keys(out_file);
	}
	else if(what == WHT_NPDRM_KEYS) {
		res = do_dump_npdrm_keys(out_file);
	}
	else if(what == WHT_REPORT) {
		res = do_dump_report(out_file);
	}
	else {
		res = do_device_dump(block_device, out_file, format);				
	}
	
	if(res >= OK && what == WHT_WHOLE_GAMECART) { 
		if(!FMT_IS_VCI(format)) { // dump keys.bin if its not VCI.
			change_extension(extra_file, MAX_PATH, wht_to_extension(WHT_CMD56_KEYS)); // get keys file ..
			res = do_dump_cmd56_keys(extra_file);
		}
		
		// create dump report.
		change_extension(extra_file, MAX_PATH, wht_to_extension(WHT_REPORT));
		res = do_dump_report(extra_file);
	}
	
	if(res < OK) {
		do_error(res);
	}
	else {
		char msg[MAX_PATH];
		snprintf(msg, sizeof(msg)-1, "Backup created at: %s ...", out_file);
		do_confirm_message("Backup Success!", msg);
	}

	return res;
}

int handle_menu_set_network_options(BackupWhat what, BackupFormat format, const char* block_device, char* outfile) {

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
				// get port
				uint16_t port = CONFIG.port;
				open_ime_short("Enter PORT", &port);
				CONFIG.port = port;
				
				save_config();
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
	InitalizeNetFs(CONFIG.ip_address, CONFIG.port);
	
	return handle_dump_device(what, format, block_device, outfile, CONFIG.ip_address, CONFIG.port);
}



	
void handle_menu_set_output(const char* ext, BackupFormat format, BackupWhat what, const char* block_device) {
		PRINT_FUNC();
		
		// get output_folder
		int res = 0;
		char output_folder[MAX_PATH];
		char output_filename[MAX_PATH];

		char* output_device = NULL;
		
		// skip if block device already set ..
		if(block_device == NULL) {
			block_device = wht_to_blkdev(what);
			if(block_device != NULL) PRINT_STR("block_device: %s\n", block_device);
		}

		
		// get filename
		get_output_filename(output_filename, ext, 0, sizeof(output_filename));		
		PRINT_STR("output_filename: %s\n", output_filename);

		// get required space for the file
		uint64_t required_space = sizeof(GcCmd56Keys);
		if(block_device != NULL) get_effective_size(block_device, format, &required_space);
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
			snprintf(output_folder, sizeof(output_folder)-1, "%s%s/%s", output_device, CONFIG.backup_folder, output_filename);
				
			// rename file to eg "Minecraft PlayStation Vita Edition [PCSG10023] (01).vci" if the file already exists
			for(size_t num = 1; file_exist(output_folder); num++) {	
				get_output_filename(output_filename, ext, num, sizeof(output_filename));
				snprintf(output_folder, sizeof(output_folder)-1, "%s%s/%s", output_device, CONFIG.backup_folder, output_filename);
			};
			

			if(selected != DUMP_LOCATION_NET) {
				PRINT_STR("DUMP PHYSICAL : what = %x, format = %x, output_folder = %s\n", what, format, output_folder);
				InitalizeDiskFs();

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

void handle_wipe_option(BackupWhat what) {
	InitalizeDiskFs();

	const char* block_device = wht_to_blkdev(what);
			
	int res = 0;
	
	if(WHT_IS_EXFAT(what)) res = handle_format_confirm_and_format(block_device);
	else if(WHT_IS_RAW(what)) res = do_device_wipe_and_format(block_device, 1, 0);
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

void handle_select_file(BackupWhat what, char* folder) {

	const char* block_device = wht_to_blkdev(what);
	const char* extension = wht_to_extension(what);
	
	// get total size
	uint64_t total_device_size = 0;
	if(block_device != NULL) get_effective_size(block_device, BACKUP_FORMAT_RAW, &total_device_size);
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


}

void handle_select_input_device(BackupWhat what) {

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
	
	InitalizeDiskFs();
	
	char input_folder[MAX_PATH];
	snprintf(input_folder, sizeof(input_folder), "%s%s", input_device, CONFIG.backup_folder);
	make_directories(input_folder);
	
	// get input file
	handle_select_file(what, input_folder);
	return;
}

void handle_menu_select_backup_format(BackupWhat what) {
	BackupFormat format = BACKUP_FORMAT_VCI;
	int selected = -1;

	while(1) {
		selected = do_select_backup_format();
		switch(selected) {
			case SELECT_FMT_VCI:
				format = BACKUP_FORMAT_VCI;
				break;
			case SELECT_FMT_VCI_TRIM:
				format = BACKUP_FORMAT_VCI_TRIM;
				break;
			case SELECT_FMT_PSV:
				format = BACKUP_FORMAT_PSV;
				break;
			case SELECT_FMT_PSV_TRIM:
				format = BACKUP_FORMAT_PSV_TRIM;
				break;
			case SELECT_FMT_RAW:
				format = BACKUP_FORMAT_RAW;
				break;
			case OP_CANCELED:
				return;
			default:
				break;
		};
		
		handle_menu_set_output(fmt_to_extension(format), format, what, NULL);
	}
	
	return;
}

void handle_device_info() {
	while(1) {
		int selected = do_device_info();
		switch(selected) {
			case DUMP_REPORT:
				handle_menu_set_output(wht_to_extension(WHT_REPORT), BACKUP_FORMAT_RAW, WHT_REPORT, NULL);
				return;
			case OP_CANCELED:
			case CANCEL_REPORT:
				return;
			default:
				break;
		}
	}
}

void handle_menu_select_option() {
	
	BackupWhat what = WHT_WHOLE_GAMECART;
	int selected = do_gc_options();
	
	switch(selected) {
		case DUMP_WHOLE_GC:
			what = WHT_WHOLE_GAMECART;
			break;
		
		case DUMP_KEYS_ONLY:
			what = WHT_CMD56_KEYS;
			break;
			
		case DUMP_MEDIAID:
		case WRITE_MEDIAID:
		case RESET_MEDIAID:
			what = WHT_MEDIA_ID;
			break;
			
		case DUMP_GRW0:
		case WRITE_GRW0:
		case RESET_GRW0:
			what = WHT_GAMECART_REWRITABLE;
			break;
			
		case GET_GC_INFO:
			break;
			
		default:
			break;
	};


	if(selected == DUMP_WHOLE_GC) 
		handle_menu_select_backup_format(what);
	if(selected == DUMP_KEYS_ONLY || selected == DUMP_MEDIAID || selected == DUMP_GRW0)
		handle_menu_set_output(wht_to_extension(what), BACKUP_FORMAT_RAW, what, NULL);
	if(selected == RESET_MEDIAID || selected == RESET_GRW0)
		handle_wipe_option(what);
	if(selected == WRITE_MEDIAID || selected == WRITE_GRW0)
		handle_select_input_device(what);
	if(selected == GET_GC_INFO)
		handle_device_info();

	if(selected == OP_CANCELED)
		do_gc_insert_prompt();
}

int handle_select_arbitary_device() {
	char dump_from[0x100];
	int ret = do_select_arbitary_device(dump_from, sizeof(dump_from)-1);
	if(ret == OP_CANCELED) return 0;
	
	handle_menu_set_output(wht_to_extension(WHT_RAW_PARTITION), BACKUP_FORMAT_RAW, WHT_WHOLE_GAMECART, dump_from);
	return 0;
}



int main(int argc, char** argv) {
	int has_restarted = (argc >= 2 && argv != NULL && strcmp(argv[1], "-restarted") == 0);
	PRINT_STR("has_restarted: 0x%x\n", has_restarted);

	init_config();
	init_ctrl();
	init_vita2d();
	init_menus();
	init_network();
	init_sound();
	init_shell();
	
	// check if firmware is too old (< 3.60)
	int firmware = get_real_fw_version();
	PRINT_STR("Running firmware version: %x\n", firmware);	
	if(firmware < MIN_FW_VERSION) {
		do_firmware_too_old(firmware);
		return 0;
	}
	
	// check if program has been restarted for kernel modules;
	if(!has_restarted){
		if(load_kernel_modules()) { // load kernel modules, and restart.
			restart_application();
		}
	}
	
	// check if kernel module was loaded ..
	if(!is_module_started(KMODULE_NAME)) {
		do_kmodule_failed_message(KMODULE_NAME);
		return 0;
	}
	
	// check for secret menu
	if(get_key_noblock() & (SCE_CTRL_RTRIGGER | SCE_CTRL_R1 | SCE_CTRL_TRIANGLE)) {
		handle_select_arbitary_device();
	}
	
	// check for sd2vita
	if(check_using_sd2vita()) {
		do_eject_sd_prompt();
	}

	// main loop:
	undo_sd2vita_patches();	
	PRINT_STR("Undone sd2vita patches\n");
	
	do_gc_insert_prompt();
	while(1) {
		handle_menu_select_option();
	}
		
	
	term_shell();
	term_sound();
	term_menus();
	term_vita2d();
	term_network();
	return 0;
}
