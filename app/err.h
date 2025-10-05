#ifndef ERR_H
#define ERR_H 1

#include "log.h"
#define ERROR(x) do { res = x; PRINT_STR("ERROR: 0x%x\n", res); goto error; } while(0);
const char* get_error_msg(int error);

#endif // ERR_H