#include <stdint.h>
#define OP_CANCELED (-9530)


void do_gc_insert_prompt();
int do_gc_options();
int do_select_output_location(char* output, uint64_t device_size);
int do_select_input_location();
int do_select_file(char* folder, char* output, char* extension, uint64_t max_size);

int do_device_dump(const char* block_device, char* output_file, uint8_t vci, char* ip_address, unsigned short port);
int do_device_wipe_and_format(const char* block_device, uint8_t full, uint8_t format);
int do_device_restore(const char* block_device, char* input_file);
int do_format_confirm(const char* block_device);

void do_confirm_message(const char* title, const char* msg);
int do_network_options(char* ip_address, unsigned short port);
int do_error(int error);
void do_ime();
void do_kmodule_failed_message(const char* module_name);
void do_device_info();

void init_menus();
void term_menus();

enum insert_menu_options {
	DUMP_WHOLE_GC,
	DUMP_KEYS_ONLY,
	DUMP_MEDIAID,
	WRITE_MEDIAID,
	RESET_MEDIAID,
	DUMP_GRW0,
	WRITE_GRW0,
	RESET_GRW0,
	GET_GC_INFO
};

enum select_network_options {
	CHANGE_IP,
	CHANGE_PORT,
	START_DUMP
};

enum select_format_options {
	FULL_FORMAT,
	QUICK_FORMAT,
	CANCEL_FORMAT
};

enum select_input_options {
	RESTORE_LOCATION_UX0,
	RESTORE_LOCATION_XMC,
	RESTORE_LOCATION_UMA,
	RESTORE_LOCATION_HOST,
	I_RELOAD_DEVICES
};

enum select_output_options {
	DUMP_LOCATION_UX0,
	DUMP_LOCATION_XMC,
	DUMP_LOCATION_UMA,
	DUMP_LOCATION_HOST,
	DUMP_LOCATION_NET,
	CHANGE_FILENAME,
	O_RELOAD_DEVICES
};