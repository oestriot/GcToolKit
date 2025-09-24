#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

// define socket functions for each operating system
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winsock2.h>

#define INIT_SOCKET()	WORD versionWanted = MAKEWORD(1, 1); \
						WSADATA wsaData; \
						WSAStartup(versionWanted, &wsaData)
						
#define CREATE_SOCKET()	socket(AF_INET, SOCK_STREAM, 0)
#define READ_SOCKET(socket_fd, buffer, buffer_size) recv(socket_fd, (char*)buffer, buffer_size, 0)
#define BIND_SOCKET(socket_fd, port)	struct sockaddr_in serverAddress; \
										memset(&serverAddress, 0x00, sizeof(serverAddress)); \
										serverAddress.sin_family = AF_INET; \
										serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); \
										serverAddress.sin_port = htons(port); \
										bind(socket_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress))
#define LISTEN_SOCKET(socket_fd) listen(socket_fd, 50)
#define ACCEPT_SOCKET(socket_fd) accept(socket_fd, (struct sockaddr*)NULL, NULL)
#define CLOSE_SOCKET(socket_fd) close(socket_fd)
#define TERM_SOCKET() WSACleanup()
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INIT_SOCKET() /**/
#define CREATE_SOCKET() socket(AF_INET, SOCK_STREAM, 0)
#define READ_SOCKET(socket_fd, buffer, buffer_size) read(socket_fd, buffer, buffer_size)
#define BIND_SOCKET(socket_fd, port)	struct sockaddr_in serverAddress; \
										memset(&serverAddress, 0x00, sizeof(serverAddress)); \
										serverAddress.sin_family = AF_INET; \
										serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); \
										serverAddress.sin_port = htons(port); \
										bind(socket_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress))
#define LISTEN_SOCKET(socket_fd) listen(socket_fd, 50)
#define ACCEPT_SOCKET(socket_fd) accept(socket_fd, (struct sockaddr*)NULL, NULL)
#define CLOSE_SOCKET(socket_fd) close(socket_fd)
#define TERM_SOCKET() /**/

#endif


#define WORKBUF_SIZE 0x20000
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
static_assert(sizeof(packet) == PACKET_SIZE);

typedef struct send_file_packet {
	uint16_t magic;
	char filename[MAX_FILENAME_SIZE];
	char padding[0x1B0];
	uint64_t total_size;
} send_file_packet;
static_assert(sizeof(send_file_packet) == PACKET_SIZE);

typedef struct patch_file_packet {
	uint16_t magic;
	uint32_t offset;
	uint32_t patch_size;
	char patch_data[MAX_PATCH_SIZE];
	char filename[MAX_FILENAME_SIZE];
	char padding[0x164];
} patch_file_packet;
static_assert(sizeof(patch_file_packet) == PACKET_SIZE);

