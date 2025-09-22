#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "main.h"

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
	char* workBuffer = malloc(WORKBUF_SIZE);
	int connectionFd = *(int*)args;
	send_file_packet header;
	
	int rd = READ_SOCKET(connectionFd, &header, sizeof(send_file_packet));
	
	if(rd == sizeof(send_file_packet)) { // check header size
		if(header.magic == SEND_FILE_MAGIC){ // check magic number
			header.filename[sizeof(header.filename)-1] = 0x00; // prevent buffer overflow
			remove_illegal_chars(header.filename); // remove illegal chars from filename
			
			printf("Receiving ... %s ... %llu bytes.\n", header.filename, header.total_size);
			FILE* outfileFd = fopen(header.filename, "wb");
			
			uint64_t total_read = 0;
			do {
				rd = READ_SOCKET(connectionFd, workBuffer, WORKBUF_SIZE);
				fwrite(workBuffer, rd, 1, outfileFd);
				total_read += rd;
			} while(total_read < header.total_size);
			
			printf("File ... %s ... received.\n", header.filename);
			fclose(outfileFd);
		}
		else {
			fprintf(stderr, "Invalid magic: %x\n", header.magic);
		}
	}
	else{
		fprintf(stderr, "Header packet is incorres size: %x\n", rd);
	}
	
	CLOSE_SOCKET(connectionFd);
	free(workBuffer);
}



int main(int argc, char *argv[])
{
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
		int connectionFd = ACCEPT_SOCKET(listenFd);
		pthread_t threadFd;
		pthread_create(&threadFd, NULL, receive_file, &connectionFd);
		sleep(1);
	}
	
	TERM_SOCKET();
}