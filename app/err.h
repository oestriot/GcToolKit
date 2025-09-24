#include "log.h"
#define ERROR(x) { res = x; PRINT_STR("res=0x%x\n", res); goto error; }
const char* get_error_msg(int error);