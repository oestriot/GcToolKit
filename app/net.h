#include "io.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#define DEFAULT_IP "192.168.1.0"
#define DEFAULT_PORT 46327

#define PACKET_SIZE 0x210
#define MAX_FILENAME_SIZE 0x50
#define MAX_PATCH_SIZE 0x50
#define SEND_FILE_MAGIC 38717
#define PATCH_FILE_MAGIC 63215

typedef struct packet {
	uint16_t magic;
	char padding[PACKET_SIZE-sizeof(uint16_t)];
} packet;

typedef struct send_file_packet {
	uint16_t magic;
	char filename[MAX_FILENAME_SIZE];
	char padding[0x1B0];
	uint64_t total_size;
} send_file_packet;

typedef struct patch_file_packet {
	uint16_t magic;
	uint32_t offset;
	uint32_t patch_size;
	char patch_data[MAX_PATCH_SIZE];
	char filename[MAX_FILENAME_SIZE];
	char padding[0x164];
} patch_file_packet;



int init_network();
void term_network();
uint8_t is_connected();
uint8_t check_ip_address_valid(char* ip_address);

int begin_file_send(const char* ip_address, unsigned short port, const char* filename, uint64_t total_size);
int file_send_data(int fstream, void* data, size_t data_sz);
int end_file_send(int fstream);

int send_packet_file(SceUID socket, const char* filename, uint64_t file_size);
int send_packet_patch(SceUID socket, const char* filename, uint32_t offset, const char* data, uint32_t size);