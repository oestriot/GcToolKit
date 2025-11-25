#include "gc_ident.h"
#include "menu.h"
#include "draw.h"
#include "ctrl.h"
#include "auth.h"
#include "device.h"
#include "gameinfo.h"
#include "io.h"
#include "err.h"
#include "net.h"
#include "kernel.h"
#include "lock.h"
#include "log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdk.h>
#include <stdint.h>
#include <GcKernKit.h>

static vita2d_texture* insertgc_tex = NULL;
static uint8_t options[BUFFER_SIZE];

static uint32_t oy = 0;
const uint32_t ygap = 20;
const uint32_t ystart = 200;
const uint32_t ytop = 110;

uint32_t start_y(uint32_t new_y) {
	oy = new_y;
	return oy;
}

uint32_t skip_y(uint32_t val) {
	oy += val;
	return oy;
}

uint32_t next_y() {
	return skip_y(ygap);
}

#define DEFOPT(y) int option = 0;\
				  start_y(y-ygap); \
				  int total = 0; \
				  memset(options, 0x00, sizeof(options))
				  
#define ADDOPT(cond,x) if(cond) { \
					draw_option(next_y(), x, option == *selected); \
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
					  memset(options, 0x00, sizeof(options)); \
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
					 char output_txt[MAX_PATH]; \
					 snprintf(output_txt, sizeof(output_txt), "%s %s ...", title, dev); \
					 draw_title(output_txt); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%s: %s ...", what, to); \
					 draw_text_center(start_y(ystart), output_txt); \
					 draw_progress_bar(skip_y(10), progress, total); \
					 skip_y(10);\
					 \
					 next_y(); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%lluMiB / %lluMiB", (uint64_t)(((float)progress)/(1048576.0f)), (uint64_t)(((float)total)/(1048576.0f))); \
					 draw_text_center(next_y(), output_txt); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%llu bytes copied of %llu total ...", progress, total); \
					 draw_text_center(next_y(), output_txt); \
					 \
					 next_y(); \
					 \
					 snprintf(output_txt, sizeof(output_txt), "%s %u%% completed.", what, (uint32_t)((((float)progress) / ((float)total)) * 100.0)); \
					 draw_text_center(330, output_txt); \
					 \
					 end_draw()
					 
#define WAIT_FOR_CONFIRM() \
					do {\
						int ctrl = get_key(); \
						if(ctrl == SCE_CTRL_CONFIRM) break;\
					} while(1)\

void init_menus() {
	insertgc_tex = load_texture("app0:/res/insertgc.png");
}

void term_menus() {
	free_texture(insertgc_tex);	
}

void draw_wipe_progress(const char* device, const char* unused, uint64_t progress, uint64_t total) {
	RESTORE_MENU("Formatting", "Writing", device, device);
}

void draw_restore_progress(const char* device, const char* input_filename, uint64_t progress, uint64_t total) {
	RESTORE_MENU("Restoring", "Reading", device, input_filename);
}

void draw_dump_progress(const char* device, const char* output_filename, uint64_t progress, uint64_t total) {
	RESTORE_MENU("Backing up", "Writing", device, output_filename);
}

int draw_select_backup_format(int* selected, int* window, const char* unused) {
	start_draw();
	draw_background();
	draw_controls(1);

	draw_title("Select Backup format ...");
	
	DEFOPT(ystart);

	ADDOPT(1, "Backup Vita Cartridge Image (.vci)");
	ADDOPT(1, "Backup Trimmed Vita Cartridge Image (.trim.vci)");
	ADDOPT(1, "Backup PSVGameSD (.psv)");
	ADDOPT(1, "Backup Trimmed PSVGameSD (.trim.psv)");
	ADDOPT(1, "Backup Raw (.img)");
	
	end_draw();
	
	RETURNOPT();
}

int draw_gc_options(int* selected, int* window, char* title, uint8_t has_grw0, uint8_t has_mediaid) {
	start_draw();
	draw_background();
	draw_controls(0);

	char what_title[MAX_PATH];
	snprintf(what_title, sizeof(what_title), "What to do with %s ...", title);
	draw_title(what_title);
	
	DEFOPT(ystart);

	ADDOPT(1, "Backup Entire Game Cart");
	ADDOPT(1, "Backup Game Cart Keys");

	ADDOPT(has_mediaid, "Backup Media Id Section");
	ADDOPT(has_mediaid, "Restore Media Id Section");		
	ADDOPT(has_mediaid, "Format Media Id Section");

	ADDOPT(has_grw0, "Backup Writable Section");
	ADDOPT(has_grw0, "Restore Writable Section");
	ADDOPT(has_grw0, "Format Writable Section");
	
	ADDOPT(1, "Get Game Cart Info");
	
	end_draw();
	
	RETURNOPT();
}

void draw_insert_gc_menu() {
	start_draw();
	draw_background();
	
	start_y(ytop);

	draw_title("Waiting for CMD56 authentication ...");
	draw_texture_center(next_y(), insertgc_tex);
	
	skip_y(200);
	
	draw_text_center(next_y(), "Insert a PlayStation Vita Game Cartridge.");	
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
	
	DEFOPT(ystart);
	
	ADDOPT(have_ux0, "Load from \"ux0:\"");
	ADDOPT(have_xmc, "Load from Sony Memory Card");
	ADDOPT(have_usb, "Load from USB Drive");
	ADDOPT(have_host0, "Load from Devkit Host0");
	ADDOPT(1, "Refresh Devices");
	
	end_draw();
	
	RETURNOPT();
}

int draw_select_output_location(int* selected, int* window, char* output_file, uint8_t have_ux0, uint8_t have_xmc, uint8_t have_usb, uint8_t have_host0, uint8_t save_network) {
	PRINT_STR("start_draw()\n");
	
	start_draw();

	PRINT_STR("draw_background()\n");
	draw_background();

	PRINT_STR("draw_controls()\n");
	draw_controls(1);
	
	PRINT_STR("draw_title()\n");
	draw_title("Select output device ...");

	char output_txt[MAX_PATH];
	snprintf(output_txt, sizeof(output_txt), "%s", output_file);
	draw_text_center(start_y(ystart), output_txt);
	
	PRINT_STR("output_txt: %s\n", output_txt);
	
	next_y();
	
	DEFOPT(next_y());
	
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
	
	char title[MAX_PATH];
	snprintf(title, sizeof(title), "Select a file from: %s ...", input_folder);
	draw_title(title);
	
	DEFOPT(ytop);

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

	draw_text_center(start_y(ystart), "Run the \"GcNetworkBackup\" program");
	draw_text_center(next_y(), "it can be found in the readme for GC ToolKit.");
	draw_text_center(next_y(), "and enter the IP of the device its running on.");

	next_y();
	
	char output_txt[MAX_PATH];
	snprintf(output_txt, sizeof(output_txt), "Current Setting: %s on port %u", ip_address, port);
	draw_text_center(next_y(), output_txt);
	
	next_y();
	
	DEFOPT(next_y());
	
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

	draw_text_center(start_y(ystart), "IME Dialog is opening...");
	
	end_draw();
}

int draw_format_confirm_menu(int* selected, int* window, const char* device) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	char output_txt[MAX_PATH];
	snprintf(output_txt, sizeof(output_txt), "Format %s to TexFAT? ...", device);
	draw_title(output_txt);
	
	draw_text_center(start_y(ystart), "Warning: this will ERASE ALL DATA ...");
	snprintf(output_txt, sizeof(output_txt), "on partition \"%s\" ...", device);
	draw_text_center(next_y(), output_txt);
	
	next_y();
	
	DEFOPT(next_y());
	ADDOPT(1, "Full format");
	ADDOPT(1, "Quick format");
	ADDOPT(1, "Cancel");
	
	end_draw();
	
	RETURNOPT();

}

void draw_blacklisted_module_message(const char* module_name) {
	start_draw();
	draw_background();
	draw_controls(0);
	
	char output_txt[MAX_PATH];
	snprintf(output_txt, sizeof(output_txt), "Conflicting modules found!");
	draw_title(output_txt);

	snprintf(output_txt, sizeof(output_txt), "GcToolKit detected the module \"%s\"", module_name);
	
	draw_text_center(start_y(ystart), output_txt);
	draw_text_center(next_y(), "It is known to cause issues with GcToolKit");
	draw_text_center(next_y(), "and with Game Cartridge Authentication more generally.");
	draw_text_center(next_y(), "Please remove this module then try again.");
	
	end_draw();
}

void draw_kmodule_failed_message(const char* module_name) {
	start_draw();
	draw_background();
	draw_controls(0);
	
	char output_txt[MAX_PATH];
	snprintf(output_txt, sizeof(output_txt), "Failed to load app0:%s.skprx", module_name);
	draw_title(output_txt);
	
	snprintf(output_txt, sizeof(output_txt), "GcToolKit failed to load kernel module \"%s\"", module_name);
	draw_text_center(start_y(ystart), output_txt);
	draw_text_center(next_y(), "Please ensure \"Unsafe Homebrew\" is enabled.");
	draw_text_center(next_y(), "This can also fail on \"Activated\" Dev & Test Kits.");
	
	end_draw();
}


void draw_confirmation_message(const char* title, const char* msg) {
	start_draw();
	draw_background();
	draw_controls(0);
	
	draw_title(title);

	draw_text_center(start_y(ystart), msg);
	
	end_draw();
}

void draw_device_info(GcInfo* info) {
	start_draw();
	draw_background();
	draw_controls(1);
	
	draw_title("GameCart Information");
	
	char hex[MAX_PATH];
	char msg[MAX_PATH];
	
	memset(hex, 0x00, sizeof(hex));
	memset(msg, 0x00, sizeof(msg));
	
	TO_HEX(&info->card_id, sizeof(info->card_id), hex);
	snprintf(msg, sizeof(msg)-1, "MMC CID: %s", hex);
	draw_text_center(start_y(ytop), msg);
	
	TO_HEX(info->card_descriptor, sizeof(info->card_descriptor), hex);
	snprintf(msg, sizeof(msg)-1, "MMC CSD: %s", hex);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Extended CSD Revision: 0x%02X", info->extra_card_descriptor_revision);
	draw_text_center(next_y(), msg);

	// card id breakdown:
	next_y();

	snprintf(msg, sizeof(msg)-1, "Crc7: 0x%02X", info->card_id.crc7);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Manufactured Date: %u/%u (0x%02X)", info->month, info->year, info->card_id.manufacture_date);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Serial Number: 0x%02X", info->card_id.serial_number);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Revision: 0x%02X", info->card_id.revision);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Product Name: %.6s", info->card_id.product_name);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Oem ID: 0x%02X", info->card_id.oem_id);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Device Type: %s (0x%02X)", mmc_device_type_to_string(info->card_id.device_type), info->card_id.device_type);
	draw_text_center(next_y(), msg);

	snprintf(msg, sizeof(msg)-1, "Vendor: %s (0x%02X)", mmc_vendor_id_to_manufacturer(info->card_id.vendor), info->card_id.vendor);
	draw_text_center(next_y(), msg);

	
	// cmd56 breakdown	
	next_y();	
	
	TO_HEX(info->key_set.packet18_key, sizeof(info->key_set.packet18_key), hex);
	snprintf(msg, sizeof(msg)-1, "CMD56 Key18: %s", hex);
	draw_text_center(next_y(), msg);
	
	TO_HEX(info->key_set.packet20_key, sizeof(info->key_set.packet20_key), hex);
	snprintf(msg, sizeof(msg)-1, "CMD56 Key20: %s", hex);
	draw_text_center(next_y(), msg);
	
	snprintf(msg, sizeof(msg)-1, "CMD56 KeyID: %s (0x%02X)", keyid_to_keygroup(info->key_id), info->key_id);
	draw_text_center(next_y(), msg);
	
		
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
	
	// check for sd2vita plugins and patch if its done
	if(!kUndoneSd2VitaPatches() && check_loaded_blacklisted_module() != NULL) {
		kUndoSd2Vita();
		
		// cleanup leftover stuff ...
		umount_ux0();
		lock_shell();
		unlock_shell();
	}
	

	mount_gro0();
	mount_grw0();

	read_gameinfo(title_id, title, sizeof(title));
	
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

void do_blacklisted_module_message(const char* module_name) {
	draw_blacklisted_module_message(module_name);
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

int do_select_backup_format() {
	PROCESS_MENU(draw_select_backup_format, NULL);
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

int do_device_dump(const char* block_device, char* output_file, BackupFormat format, char* ip_address, unsigned short port) {
	
	lock_shell();
	lock_power();
	
	umount_gro0();
	umount_grw0();
	mount_devices();
	int res = 0;
	
	GcCmd56Keys keys;
	NetworkInfo net_info;
	
	kGetPerCartKeys(&keys);
	
	if(ip_address != NULL) {
		strncpy(net_info.ip_address, ip_address, sizeof(net_info.ip_address)-1);
		net_info.port = port;		
	}
	
	res = dump_device(
				block_device,
				output_file,
				format,
				(!FMT_IS_RAW(format)) ? &keys : NULL,
				(ip_address != NULL) ? &net_info : NULL,
				draw_dump_progress);

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
	char msg[MAX_PATH];
	
	snprintf(msg, sizeof(msg), "Error: %s (0x%02X)", get_error_msg(error), error);
	do_confirm_message("An error occured.", msg);
	return 0;
}

int do_select_output_location(char* output, uint64_t dev_size) {
	
	PRINT_STR("mount_devices\n");
	mount_devices();
	
	uint8_t save_network = is_connected();	
	
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
	
	PRINT_STR("save_network = %x\n", save_network);


	PRINT_STR("ux_exist = %x\n", ux_exist);
	PRINT_STR("xmc_exist = %x\n", xmc_exist);
	PRINT_STR("uma_exist = %x\n", uma_exist);
	PRINT_STR("host_exist = %x\n", host_exist);

	PROCESS_MENU(draw_select_output_location, output, 
				(ux_exist  && ux_size >= dev_size), 
				(xmc_exist && xmc_size >= dev_size), 
				(uma_exist && uma_size >= dev_size), 
				host_exist, save_network);
	
	return selected;
}