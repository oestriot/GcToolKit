#define KMODULE_NAME "GcKernKit"
#define EBOOT_PATH "app0:/eboot.bin"

void load_kernel_modules();
int is_module_started(const char* module_name);
const char* check_loaded_blacklisted_module();