#include <stdlib.h>
#include <GcToolKit.h>
#include "config.h"
#include "io.h"

int begin_file_send(SceUID socket, const char* filename, uint64_t file_size);
int file_send_data(SceUID socket, void* data, size_t data_sz);
int send_file_patch(SceUID socket, const char* filename, uint32_t offset, const void* data, uint32_t size);

int init_network();
void term_network();
uint8_t is_connected();
uint8_t check_ip_address_valid(char* ip_address);

int begin_connection(const char* ip_address, unsigned short port);
int end_connection(SceUID socket);

