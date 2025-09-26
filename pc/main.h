#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
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
