#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#define PACKET_SIZE 0x210
#define SEND_FILE_MAGIC 38717
#define PATCH_FILE_MAGIC 63215

typedef struct packet {
	uint16_t magic;
	char padding[PACKET_SIZE-sizeof(uint16_t)];
} __attribute__((packed)) packet;
static_assert(sizeof(packet) == PACKET_SIZE);

typedef struct send_file_packet {
	uint16_t magic;
	char filename[0x206];
	uint64_t total_size;
} __attribute__((packed)) send_file_packet;
static_assert(sizeof(send_file_packet) == PACKET_SIZE);

typedef struct patch_file_packet {
	uint16_t magic;
	uint64_t offset;
	uint32_t patch_size;
	char patch_data[0x1B2];
	char filename[0x50];
} __attribute__((packed)) patch_file_packet;
static_assert(sizeof(patch_file_packet) == PACKET_SIZE);

