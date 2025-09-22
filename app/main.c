#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <taihen.h>
#include <vitasdk.h>
#include <vita2d.h>

#include "lock.h"
#include "ctrl.h"
#include "crypto.h"
#include "gameinfo.h"
#include "kernel.h"
#include "draw.h"
#include "menu.h"
#include "ime.h"
#include "device.h"
#include "net.h"
#include "bgm.h"
#include "log.h"
#include <GcKernKit.h>

void get_output_filename(char* output, char* format, int size_output) {
	char title_id[64];
	char title[64];
	read_gameinfo(title_id, title);
		
	snprintf(output, size_output, "%s [%s].%s", title, title_id, format);
	remove_illegal_chars(output);
}


int handle_dump_device(int what, char* block_device, char* outfile, char* ip_address, unsigned short port) {
	int res = 0;
	
	if(what != DUMP_KEYS_ONLY)
		res = do_device_dump(block_device, outfile, (what == DUMP_WHOLE_GC) ? 1 : 0, ip_address, port);		
	else if(ip_address == NULL)
		res = key_dump(outfile);
	else
		res = key_dump_network(ip_address, port, outfile);
	
	
	return res;
}

int handle_menu_set_network_options(int what, char* block_device, char* outfile) {
	unsigned char ip_address[0x1028];
	unsigned short port = DEFAULT_PORT;
	
	memset(ip_address, 0x00, sizeof(ip_address));
	strncpy(ip_address, DEFAULT_IP, sizeof(ip_address));
	
	int selected = -9891;
	while(1) {
		selected = do_network_options(ip_address, port);
		switch(selected) {
			case CHANGE_IP:
				open_ime("Enter IP", ip_address, 15);
				PRINT_STR("ip address: %s\n", ip_address);
				if(!check_ip_address_valid(ip_address))
					strncpy(ip_address, DEFAULT_IP, sizeof(ip_address));
				
				continue;
				break;
			case CHANGE_PORT:
				open_ime_short("Enter PORT", &port);
				continue;
				break;
			case START_DUMP:
				break;
			case OP_CANCELED:
				return OP_CANCELED;
		}
		
		break;
	};
	
	return handle_dump_device(what, block_device, outfile, ip_address, port);
}


	
void handle_menu_set_output(char* fmt, int what) {
	// determine block device	
	PRINT_STR("handle_menu_set_output\n");
	
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
	char output_filename[0x128];
	get_output_filename(output_filename, fmt, MAX_PATH);
	
	PRINT_STR("output_filename: %s\n", output_filename);

	// get total size
	uint64_t total_device_size = sizeof(GcCmd56Keys);
	if(block_device != NULL)
		total_device_size = device_size(block_device);
	PRINT_STR("total_device_size %llx\n", total_device_size);
	
	int selected = -9892;
	while(1) {
		selected = do_select_output_location(output_filename, total_device_size);
		
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
				open_ime("Enter filename", output_filename, MAX_PATH);
				remove_illegal_chars(output_filename);
				continue;
				break;
			case O_RELOAD_DEVICES:
				continue;
				break;
			case OP_CANCELED:
				return;
			default:
				return;
		};
		break;
	};
	PRINT_STR("output_device %s\n", output_device);

	// get outfile
	char output_folder[0x512];
	
	// create "device:bak" folder if not exist
	if(selected != DUMP_LOCATION_NET) {
		snprintf(output_folder, sizeof(output_folder), "%sbak", output_device);
		sceIoMkdir(output_folder, 0777);
	}
	
	// get full output path, device:bak/file.vci 
	snprintf(output_folder, sizeof(output_folder), "%sbak/%s", output_device, output_filename);
	PRINT_STR("what = %x, output_folder = %s\n", what, output_folder);

	int res = 0;
	if(selected != DUMP_LOCATION_NET) {
		res = handle_dump_device(what, block_device, output_folder, NULL, 0);
	}
	else {
		res = handle_menu_set_network_options(what, block_device, output_filename);
	}
	
	if(res == OP_CANCELED) return;
	
	char msg[0x1028];
	if(res < 0) {
		do_error(res);
	}
	else{
		snprintf(msg, sizeof(msg), "Backup created at: %s ...", output_folder);
		do_confirm_message("Backup Success!", msg);
	}


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
	
	char msg[0x1028];	
	if(res < 0) {
		do_error(res);
	}
	else{
		snprintf(msg, sizeof(msg), "Formatted: \"%s\" ...", block_device);
		do_confirm_message("Format Success!", msg);
	}	

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
	if(block_device != NULL)
		total_device_size = device_size(block_device);
	PRINT_STR("total_device_size %llx\n", total_device_size);
	
	// show file selection
	char file[MAX_PATH];
	int selected = do_select_file(folder, file, extension, total_device_size);
	if(selected == OP_CANCELED)
	if(selected < 0) {
		do_error(selected);
		return;
	}
	
	char input_file[MAX_PATH*5];
	snprintf(input_file, sizeof(input_file), "%s/%s", folder, file);
	
	int res = do_device_restore(block_device, input_file);
	char msg[0x1028];	
	if(res < 0) {
		do_error(res);
	}
	else{
		snprintf(msg, sizeof(msg), "Restored from: %s ...", input_file);
		do_confirm_message("Restore Success!", msg);
	}
}

void handle_select_input_device(int what) {

	char* input_device = NULL;
	int selected = -9894;
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
	
	// get infile
	char input_folder[MAX_PATH];
	
	
	snprintf(input_folder, sizeof(input_folder), "%sbak", input_device);
	sceIoMkdir(input_folder, 0777);
	
	handle_select_file(what, input_folder);
}

void handle_menu_select_option() {
	
	char* fmt = "";
	int selected = -9895;
	
	while(1) {
		selected = do_gc_options();
		switch(selected) {
			case DUMP_WHOLE_GC:
				fmt = "vci";
				break;
			case DUMP_KEYS_ONLY:
				fmt = "bin";
				break;
				
			case DUMP_MEDIAID:
				fmt = "mediaid";
				break;
			case WRITE_MEDIAID:
				fmt = "mediaid";
				break;
			case RESET_MEDIAID:
				break;
				
			case DUMP_GRW0:
				fmt = "img";
				break;
			case WRITE_GRW0:
				fmt = "img";
				break;
			case RESET_GRW0:
				break;
			case GET_GC_INFO:
				break;
			default:
				break;
		};

		break;
	};
	if(selected == DUMP_WHOLE_GC || selected == DUMP_KEYS_ONLY || selected == DUMP_MEDIAID || selected == DUMP_GRW0)
		handle_menu_set_output(fmt, selected);
	if(selected == RESET_MEDIAID || selected == RESET_GRW0)
		handle_wipe_option(selected);
	if(selected == WRITE_MEDIAID || selected == WRITE_GRW0)
		handle_select_input_device(selected);
	if(selected == GET_GC_INFO)
		do_device_info();
	if(selected == OP_CANCELED)
		do_gc_insert_prompt();
}

int main(int argc, char** argv) {
	int has_restarted = (argc >= 2 && argv != NULL && strcmp(argv[1], "-restarted") == 0);
	PRINT_STR("has_restarted: 0x%x\n", has_restarted);
	
	if(!has_restarted){
		load_kernel_modules();
	}
	
	init_ctrl();
	init_vita2d();
	init_menus();
	init_network();
	init_sound();
	init_shell();
	
	if(kernel_started()) {
		do_gc_insert_prompt();
		while(1) {
			handle_menu_select_option();
		}		
	}
	else {
		do_kmodule_failed_message(KMODULE_NAME);
	}
		
	
	term_shell();
	term_sound();
	term_menus();
	term_vita2d();
	term_network();
	return 0;
}
