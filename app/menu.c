#include "menu.h"
#include "draw.h"
#include "ctrl.h"
#include "crypto.h"
#include "device.h"
#include "gameinfo.h"
#include "io.h"
#include "err.h"
#include "net.h"
#include "gc_ident.h"
#include "lock.h"
#include "log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdk.h>
#include <stdint.h>
#include <GcKernKit.h>

static vita2d_texture* insertgc_tex;
static uint8_t options[0x1000];

#define DEFOPT(y) int option = 0;\
				  int opt_y = y; \
				  int increment_y = 20; \
				  int total = 0; \
				  memset(options, 0x00, sizeof(options))
				  
#define ADDOPT(cond,x) if(cond) { \
					draw_option(opt_y, x, option == *selected); \
					opt_y += increment_y; \
					total++; \
					options[option] = 1; \
					PRINT_STR("options[%i] = 1\n", option); \
				   } else { \
					options[option] = 0; \
					PRINT_STR("options[%i] = 0\n", option); \
				   } \
				   option++ 

#define RETURNOPT() return option

#define CALC_FIRST_OPTION() for(first_option = 0; (options[first_option] != 1 && first_option < sizeof(options)); first_option++)

#define CALC_LAST_OPTION() for(last_option = sizeof(options); (options[last_option] != 1 && last_option > 0); last_option--)

#define WINDOW_SIZE (19)

#define PROCESS_MENU(func, ...) \
					  int window = 0; \
					  int selected = 0; \
					  memset(options, 0x00, sizeof(options));\
					  int total_options = func(&selected, &window, __VA_ARGS__); \
					  int first_option = 0;\
					  int last_option = 0;\
					  CALC_FIRST_OPTION(); \
					  CALC_LAST_OPTION(); \
					  selected = first_option; \
					  \
					while (1) { \
					  total_options = func(&selected, &window, __VA_ARGS__); \
					  CALC_FIRST_OPTION(); \
					  CALC_LAST_OPTION(); \
					  int ctrl = get_key(); \
					  \
					  if(ctrl == SCE_CTRL_UP) {\
						do{ \
							selected--; \
						 } while(selected > 0 && options[selected] == 0); \
					  } \
					  else if(ctrl == SCE_CTRL_DOWN) {\
						do{ \
							selected++; \
						} while(selected < sizeof(options) && options[selected] == 0); \
					  } \
					  else if(ctrl == SCE_CTRL_CANCEL) {\
						return OP_CANCELED;\
					  }\
					  else if(ctrl == SCE_CTRL_CONFIRM) {\
						break; \
					  } \
					  \
					  if(selected > last_option)  { \
						selected = last_option; \
						\
						if(total_options > WINDOW_SIZE) \
							window++; \
					  } \
					  if(selected < first_option) { \
						selected = first_option; \
						\
						if(window != first_option) \
							window--; \
					  } \
					  PRINT_STR("selected: %x\n", selected); \
					  PRINT_STR("window: %x\n", window); \
				  }

#define RESTORE_MENU(title, what, dev, to) \
					 start_draw(); \
					 draw_background(); \
					 \
					 char output_txt[512]; \
					 snprintf(output_txt, sizeof(output_txt), "%s %s ...", title, dev); \
					 draw_title(output_txt); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%s: %s ...", what, to); \
					 draw_text_center(200, output_txt); \
					 \
					 draw_progress_bar(210, progress, total); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%lluMiB / %lluMiB", (uint64_t)(((float)progress)/(1048576.0f)), (uint64_t)(((float)total)/(1048576.0f))); \
					 draw_text_center(260, output_txt); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%llu bytes copied of %llu total ...", progress, total); \
					 draw_text_center(280, output_txt); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%s %i%% completed.", what, (int)((((float)progress) / ((float)total)) * 100.0)); \
					 draw_text_center(330, output_txt); \
					 \
					 end_draw()
					 
#define WAIT_FOR_CONFIRM() \
					do {\
						int ctrl = get_key(); \
						if(ctrl == SCE_CTRL_CONFIRM) break;\
						else if(ctrl == SCE_CTRL_CANCEL) break;\
					} while(1)\

void init_menus() {
	insertgc_tex = load_texture("app0:/res/insertgc.png");
}

void term_menus() {
	free_texture(insertgc_tex);	
}

void draw_wipe_progress(const char* device, char* unused, uint64_t progress, uint64_t total) {
	RESTORE_MENU("Formatting", "Writing", device, device);
}

void draw_restore_progress(const char* device, char* input_filename, uint64_t progress, uint64_t total) {
	RESTORE_MENU("Restoring", "Reading", device, input_filename);
}

void draw_dump_progress(const char* device, char* output_filename, uint64_t progress, uint64_t total) {
	RESTORE_MENU("Backing up", "Writing", device, output_filename);
}


int draw_gc_options(int* selected, int* window, char* title, uint8_t has_grw0, uint8_t has_mediaid) {
	start_draw();
	draw_background();
	draw_controls(0);

	char w_title[128];
	snprintf(w_title, sizeof(w_title), "What to do with %s ...", title);
	draw_title(w_title);
	
	DEFOPT(200);

	ADDOPT(1, "Backup Entire Game Cart (.VCI)");
	ADDOPT(1, "Backup Game Cart Keys (.BIN)");

	ADDOPT(has_mediaid, "Backup MediaId Section (.MEDIAID)");
	ADDOPT(has_mediaid, "Restore MediaId Section (.MEDIAID)");		
	ADDOPT(has_mediaid, "Format MediaId Section");

	ADDOPT(has_grw0, "Backup Writable Section (.IMG)");
	ADDOPT(has_grw0, "Restore Writable Section (.IMG)");
	ADDOPT(has_grw0, "Format Writable Section");
	
	ADDOPT(1, "Get Game Cart Info");
	
	end_draw();
	
	RETURNOPT();
}

void draw_insert_gc_menu() {
	start_draw();
	draw_background();
	
	draw_title("Waiting for CMD56 authentication ...");
	draw_texture_center(140, insertgc_tex);
	draw_text_center(360, "Insert a VITA game cart!");	
	end_draw();
}

void do_gc_insert_prompt() {
	unlock_gc();
	draw_insert_gc_menu();
	wait_for_gc_auth();	
}

int draw_select_input_location(int* selected, int* window, uint8_t have_ux0, uint8_t have_xmc, uint8_t have_usb, uint8_t have_host0) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	draw_title("Select input device ...");
	
	DEFOPT(240);
	
	ADDOPT(have_ux0, "Load from \"ux0:\"");
	ADDOPT(have_xmc, "Load from Sony Memory Card");
	ADDOPT(have_usb, "Load from USB Drive");
	ADDOPT(have_host0, "Load from Devkit Host0");
	ADDOPT(1, "Refresh Devices");
	
	end_draw();
	
	RETURNOPT();
}

int draw_select_output_location(int* selected, int* window, char* output_file, uint8_t have_ux0, uint8_t have_xmc, uint8_t have_usb, uint8_t have_host0, uint8_t save_network) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	draw_title("Select output device ...");

	char output_txt[128];
	snprintf(output_txt, sizeof(output_txt), "%s", output_file);
	draw_text_center(200, output_txt);
	
	DEFOPT(240);
	
	ADDOPT(have_ux0, "Save to \"ux0:\"");
	ADDOPT(have_xmc, "Save to Sony Memory Card");
	ADDOPT(have_usb, "Save to USB Drive");
	ADDOPT(have_host0, "Save to Devkit Host0");	
	ADDOPT(save_network, "Save to Network");
	ADDOPT(1, "Change file name");
	ADDOPT(1, "Refresh Devices");
	
	end_draw();
	
	RETURNOPT();
}


int draw_select_file(int* selected, int* window, char* input_folder, char* folders, int total_files) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	char title[128];
	snprintf(title, sizeof(title), "Select a file from: %s ...", input_folder);
	draw_title(title);
	
	DEFOPT(110);

	// check if window - total_files is less than the window size.	
	// reset window to window_size if it is
	if( *window > (total_files % WINDOW_SIZE)-1 ) {
		*window = (total_files % WINDOW_SIZE)-1;
	}
	
	for(int i = *window; i <= *window + WINDOW_SIZE; i++) {
		if(i >= total_files) break;
		
		char file[MAX_PATH];
		snprintf(file, sizeof(file), "%s", folders + (i * MAX_PATH));
		ADDOPT(1, file);
	}
	
	end_draw();
	
	RETURNOPT();		
}


int draw_network_settings(int* selected, int* window, char* ip_address, unsigned short port) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	draw_title("Enter Network Address ...");

	draw_text_center(200, "Run the \"gc_backup_network\" program");
	draw_text_center(220, "it can be found in the readme for GC ToolKit.");
	draw_text_center(250, "and enter the IP of the device its running on.");

	char output_txt[128];
	snprintf(output_txt, sizeof(output_txt), "Current Setting: %s on port %u", ip_address, port);
	draw_text_center(300, output_txt);
	
	DEFOPT(340);
	
	ADDOPT(1, "Change IP Address");
	ADDOPT(1, "Change Port");
	ADDOPT(1, "Start Network Save");
	
	end_draw();
	
	RETURNOPT();
}

void draw_ime() {
	start_draw();
	draw_background();
	
	draw_title("Starting IME Dialog ...");

	draw_text_center(200, "IME Dialog is opening...");
	
	end_draw();
}

int draw_format_confirm_menu(int* selected, int* window, const char* device) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	char output_txt[128];
	snprintf(output_txt, sizeof(output_txt), "Format %s to TexFAT? ...", device);
	draw_title(output_txt);
	
	draw_text_center(250, "Warning: this will ERASE ALL DATA ...");
	snprintf(output_txt, sizeof(output_txt), "on partition \"%s\" ...", device);
	draw_text_center(270, output_txt);
	
	DEFOPT(320);
	ADDOPT(1, "Full format");
	ADDOPT(1, "Quick format");
	ADDOPT(1, "Cancel");
	
	end_draw();
	
	RETURNOPT();

}

void draw_kmodule_failed_message(const char* module_name) {
	start_draw();
	draw_background();
	draw_controls(0);
	
	char output_txt[128];
	snprintf(output_txt, sizeof(output_txt), "Failed to load app0:%s.skprx", module_name);
	draw_title(output_txt);

	snprintf(output_txt, sizeof(output_txt), "GcToolKit failed to load kernel module \"%s\"", module_name);
	draw_text_center(250, output_txt);
	draw_text_center(270, "Please ensure \"Unsafe Homebrew\" is enabled.");
	draw_text_center(290, "This can also fail on \"Activated\" Dev & Test Kits.");
	
	end_draw();
}


void draw_confirmation_message(const char* title, const char* msg) {
	start_draw();
	draw_background();
	draw_controls(0);
	
	draw_title(title);

	draw_text_center(200, msg);
	
	end_draw();
}

void draw_device_info(GcInfo* info) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	draw_title("GC Information");
	
	char hex[0x100];
	char msg[0x200];
	
	memset(hex, 0x00, sizeof(hex));
	memset(msg, 0x00, sizeof(msg));
	
	TO_HEX(info->Cid, sizeof(info->Cid), hex);
	snprintf(msg, sizeof(msg), "MMC CID: %s", hex);
	draw_text_center(120, msg);
	
	TO_HEX(info->Csd, sizeof(info->Cid), hex);
	snprintf(msg, sizeof(msg), "MMC CSD: %s", hex);
	draw_text_center(140, msg);

	snprintf(msg, sizeof(msg), "Extended CSD Revision: 0x%02X", info->ExtCsdRev);
	draw_text_center(160, msg);
	
	snprintf(msg, sizeof(msg), "Device Name: %s", info->DeviceName);
	draw_text_center(180, msg);

	snprintf(msg, sizeof(msg), "Device Serial Number: 0x%X", info->DeviceSerial);
	draw_text_center(200, msg);
	
	snprintf(msg, sizeof(msg), "Device Revision: 0x%X", info->DeviceRev);
	draw_text_center(220, msg);
	
	
	snprintf(msg, sizeof(msg), "Vendor: %s (0x%02X)", mmc_vendor_id_to_manufacturer(info->Vendor), info->Vendor);
	draw_text_center(240, msg);
	
	snprintf(msg, sizeof(msg), "Manufactured Date: %u/%u", info->Month, info->Year);
	draw_text_center(260, msg);
	
	
	TO_HEX(info->KeySet.packet18_key, sizeof(info->KeySet.packet18_key), hex);
	snprintf(msg, sizeof(msg), "CMD56 Key18: %s", hex);
	draw_text_center(310, msg);
	
	TO_HEX(info->KeySet.packet20_key, sizeof(info->KeySet.packet20_key), hex);
	snprintf(msg, sizeof(msg), "CMD56 Key20: %s", hex);
	draw_text_center(330, msg);
	
	snprintf(msg, sizeof(msg), "CMD56 KeyID: %s (0x%02X)", keyid_to_keygroup(info->KeyId), info->KeyId);
	draw_text_center(350, msg);
	
		
	end_draw();
}


void do_device_info() {
	
	GcInfo info;
	
	get_gc_info(&info);
	draw_device_info(&info);		
	
	WAIT_FOR_CONFIRM();
}

int do_network_options(char* ip_address, unsigned short port) {	
	PROCESS_MENU(draw_network_settings, ip_address, port);
	return selected;
}

int do_gc_options() {
	char title_id[64];
	char title[64];
	
	lock_gc();

	mount_gro0();
	mount_grw0();

	read_gameinfo(title_id, title);
	
	remove_illegal_chars(title);
	
	uint8_t has_grw0 = device_exist(BLOCK_DEVICE_GRW0);
	uint8_t has_mediaid = device_exist(BLOCK_DEVICE_MEDIAID);
	
	PROCESS_MENU(draw_gc_options, title, has_grw0, has_mediaid);
	return selected;
}

int do_select_file(char* folder, char* output, char* extension, uint64_t max_size) {
	int total_files = 0;	
	static char files[MAX_PATH * sizeof(options)];
	
	SearchFilter filter;
	memset(&filter, 0x00, sizeof(SearchFilter));
	filter.max_filesize = max_size;
	filter.file_only = 1;
	strncpy(filter.match_extension, extension, sizeof(filter.match_extension)-1);
	
	int res = get_files_in_folder(folder, files, &total_files, &filter, sizeof(options));
	
	PRINT_STR("get_files_in_folder = %x\n", res);
	if(res < 0) return res;
	if(total_files <= 0) return TOTAL_FILES_LESS_THAN_EQ_0;
	
	PRINT_STR("total_files: %x\n", total_files);
	
	PROCESS_MENU(draw_select_file, folder, files, total_files);
	strncpy(output, files + (selected * MAX_PATH), MAX_PATH);	
	return selected;	
}

void do_ime() {
	for(int i = 0; i < 0x5; i++) draw_ime();
}

void do_confirm_message(const char* title, const char* msg) {
	draw_confirmation_message(title, msg);

	WAIT_FOR_CONFIRM();
}

void do_kmodule_failed_message(const char* module_name) {
	draw_kmodule_failed_message(module_name);
	WAIT_FOR_CONFIRM();
}

int do_format_confirm(const char* block_device) {
	PROCESS_MENU(draw_format_confirm_menu, block_device);
	return selected;
}

int do_device_wipe_and_format(const char* block_device, uint8_t full, uint8_t format) {
	lock_shell();
	lock_power();

	umount_gro0();
	umount_grw0();
	mount_devices();
	
	
	int res = 0;
	
	if(full) res = wipe_device(block_device, draw_wipe_progress);
	if(!full) draw_wipe_progress(block_device, NULL, 1, 1);
	if(format) res = kFormatDevice(block_device);

	mount_gro0();
	mount_grw0();
	umount_devices();
	
	unlock_shell();
	unlock_power();
	
	return res;
}

int do_device_restore(const char* block_device, char* input_file) {
	lock_shell();
	lock_power();
	
	umount_gro0();
	umount_grw0();
	mount_devices();
	
	int res = restore_device(block_device, input_file, draw_restore_progress);

	mount_gro0();
	mount_grw0();
	umount_devices();

	unlock_shell();
	unlock_power();
	return res;
}

int do_device_dump(const char* block_device, char* output_file, uint8_t vci, char* ip_address, unsigned short port) {
	
	lock_shell();
	lock_power();
	
	umount_gro0();
	umount_grw0();
	mount_devices();
	
	GcCmd56Keys keys;
	if(vci){
		int res = extract_gc_keys(&keys);
		if(res < 0) return res;
	}
	
	int res = -9190;
	
	if(ip_address == NULL)
		res = dump_device(block_device, output_file, vci ? &keys : NULL, draw_dump_progress);
	else
		res = dump_device_network(ip_address, port, block_device, output_file, vci ? &keys : NULL, draw_dump_progress);


	mount_gro0();
	mount_grw0();
	umount_devices();
	
	unlock_shell();
	unlock_power();

	return res;
}


int do_select_input_location() {
	
	PRINT_STR("mount_devices\n");
	mount_devices();
	
	uint8_t ux_exist = file_exist("ux0:");
	uint8_t xmc_exist = file_exist("xmc0:");
	uint8_t uma_exist = file_exist("uma0:");
	uint8_t host_exist = file_exist("host0:");
	
	PROCESS_MENU(draw_select_input_location, 
				ux_exist, 
				xmc_exist, 
				uma_exist,
				host_exist);
	
	return selected;
}

int do_error(int error) {
	if(error == OP_CANCELED) return error;
	
	char msg[0x1028];	
	snprintf(msg, sizeof(msg), "There was an error ( res = 0x%08X )", error);
	do_confirm_message("Error!", msg);
	return 0;
}

int do_select_output_location(char* output, uint64_t dev_size) {
	
	PRINT_STR("mount_devices\n");
	mount_devices();
	
	uint8_t save_network = is_connected();
	PRINT_STR("save_network = %x\n", save_network);
	
	
	uint64_t xmc_size = get_free_space("xmc0:");
	uint64_t uma_size = get_free_space("uma0:");
	uint64_t ux_size  = get_free_space("ux0:");

	uint8_t ux_exist = file_exist("ux0:");
	uint8_t xmc_exist = file_exist("xmc0:");
	uint8_t uma_exist = file_exist("uma0:");

	uint8_t host_exist = file_exist("host0:");

	PRINT_STR("device_size %llx\n", dev_size);
	PRINT_STR("xmc_size %llx\n", xmc_size);
	PRINT_STR("uma_size %llx\n", uma_size);
	PRINT_STR("ux_size %llx\n", ux_size);
	
	PROCESS_MENU(draw_select_output_location, output, 
				(ux_exist  && ux_size >= dev_size), 
				(xmc_exist && xmc_size >= dev_size), 
				(uma_exist && uma_size >= dev_size), 
				host_exist, 
				save_network);
	
	return selected;
}