#include <stdint.h>
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include "main.h"

#ifdef _WIN32
FILE* fopen_utf8(const char* filename, const char* mode) {
	wchar_t wfname[0x1028] = { 0 };
	wchar_t wmode[0x1028] = { 0 };
	
	MultiByteToWideChar(CP_UTF8, 0, filename, strlen(filename), wfname, sizeof(wfname));
	MultiByteToWideChar(CP_UTF8, 0, mode, strlen(mode), wmode, sizeof(wfname));
	
	return _wfopen(wfname, wmode);
}
#define fopen fopen_utf8
#endif

static char* work_buffer[WORKBUF_SIZE];



void remove_illegal_chars(char* str) {
	int slen = strlen(str);
	
	for(int i = 0; i < slen; i++) {
		if(str[i] == '/' ||
		str[i] == '\\' ||
		str[i] == ':' ||
		str[i] == '?' ||
		str[i] == '*' ||
		str[i] == '"' ||
		str[i] == '|' ||
		str[i] == '>' ||
		str[i] == '\n' ||
		str[i] == '\r' ||
		str[i] == '<')
			str[i] = ' ';		
	}
}

void* receive_file(void* args) {
	int connection_fd = *(int*)args;
	packet packet_data;
	memset(&packet_data, 0, sizeof(packet));
	
	int rd = READ_SOCKET(connection_fd, &packet_data, sizeof(packet));
	
	if(rd == sizeof(packet)) { // check header size
		if(packet_data.magic == SEND_FILE_MAGIC){ // receive packet
			send_file_packet* sendfile = (send_file_packet*)&packet_data;
			
			sendfile->filename[sizeof(sendfile->filename)-1] = 0x00; // prevent buffer overflow
			remove_illegal_chars(sendfile->filename); // remove illegal chars from filename
			
			printf("Receiving ... %s ... %lu bytes.\n", sendfile->filename, sendfile->total_size);
			FILE* outfile_fd = fopen(sendfile->filename, "wb");
			if(outfile_fd != NULL) {
				uint64_t total_read = 0;
				do {
					rd = READ_SOCKET(connection_fd, work_buffer, WORKBUF_SIZE);
					fwrite(work_buffer, rd, 1, outfile_fd);
					total_read += rd;
				} while(total_read < sendfile->total_size);
				
				printf("File ... %s ... received.\n", sendfile->filename);
				fclose(outfile_fd);
				
			}
		}
		
		else if(packet_data.magic == PATCH_FILE_MAGIC) { // patch packet
			patch_file_packet* patchfile = (patch_file_packet*)&packet_data;

			patchfile->filename[sizeof(patchfile->filename)-1] = 0x00; // prevent buffer overflow
			remove_illegal_chars(patchfile->filename); // remove illegal chars from filename
			
			printf("Patching ... %s ... %u bytes at 0x%x\n", patchfile->filename, patchfile->patch_size, patchfile->offset);
			
			FILE* patchfile_fd = fopen(patchfile->filename, "wb");
			if(patchfile_fd != NULL) {
				int pos = fseek(patchfile_fd, patchfile->offset, SEEK_SET);
				if(pos == 0) {
					fwrite(patchfile->patch_data, patchfile->patch_size, 1, patchfile_fd);				
				}
				fclose(patchfile_fd);				
			}
			
		}
		else {
			fprintf(stderr, "Invalid magic: %x\n", packet_data.magic);
		}
	}
	else{
		fprintf(stderr, "Header packet is incorres size: %x\n", rd);
	}
	
	CLOSE_SOCKET(connection_fd);
}



int main(int argc, char *argv[])
{
#ifdef _WIN32
	SetFileApisToOEM();
	setlocale(LC_ALL, ".UTF8");
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif	

	unsigned short port = DEFAULT_PORT;
	if(argc >= 2){
		port = (unsigned short)atoi(argv[1]);		
	}
	
	printf("Listening on port %u\n", port);
	
	
	INIT_SOCKET();
	int listenFd = CREATE_SOCKET();
	BIND_SOCKET(listenFd, port);
	LISTEN_SOCKET(listenFd);

	while(1)
	{
		int connection_fd = ACCEPT_SOCKET(listenFd);
		pthread_t threadFd;
		pthread_create(&threadFd, NULL, receive_file, &connection_fd);
		sleep(1);
	}
	
	TERM_SOCKET();
}