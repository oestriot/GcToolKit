#ifndef KERNEL_H
#define KERNEL_H 1

#define KMODULE_NAME "GcKernKit"
#define EBOOT_PATH "app0:/eboot.bin"

#define MIN_FW_VERSION (0x360)
#define MAX_FW_VERSION (0x374)

int load_kernel_modules();
int is_module_started(const char* module_name);
const char* check_loaded_blacklisted_module();
int check_module_version(const char* module);
int get_real_fw_version();
void restart_application();

#endif // KERNEL_H