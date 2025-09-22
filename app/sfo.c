#include <vitasdk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sfo.h"
#include "io.h"
#include "err.h"
#include <GcKernKit.h>


int read_sfo_key(char* key, char* out, char* sfo) {
	int res = 0;
	
	uint64_t sfo_size = get_file_size(sfo);
	if(sfo_size <= 0) ERROR(sfo_size);
	
	char* sfo_buffer = malloc(sfo_size);
	if(sfo_buffer == NULL) ERROR(POINTER_WAS_NULL);

	memset(sfo_buffer, 0x00, sfo_size);
	
	// open sfo file
	int sfo_fd = sceIoOpen(sfo, SCE_O_RDONLY, 0777);	
	if(sfo_fd < 0) ERROR(sfo_fd);
	// read sfo
	
	int rd = sceIoRead(sfo_fd, sfo_buffer, sfo_size);
	if(rd != sfo_size) ERROR(SIZE_NOT_MATCH);
	
	// close sfo
	if(sceIoClose(sfo_fd) >= 0)
		sfo_fd = 0;
	
	if(sfo_size <= sizeof(sfo_header)) ERROR(SIZE_NOT_MATCH);
	
	
	// get sfo header
	sfo_header header;
	memcpy(&header, sfo_buffer, sizeof(sfo_header));
	
	if(memcmp(header.magic, "\0PSF", sizeof(header.magic)) != 0) ERROR(INVALID_MAGIC); // check magic	
	if(header.count > 200) ERROR(SFO_TOO_MANY_ENTRIES); // give up if more than 200 keys
	if(sfo_size < (sizeof(sfo_header) + (sizeof(sfo_key) * header.count))) ERROR(SIZE_NOT_MATCH); // check if size is enough for keys + sfo header size
	
	uint32_t ptr = sizeof(sfo_header);
	
	// read keys
	for(int i = 0; i < header.count; i++)
	{
		if(ptr > sfo_size) ERROR(SIZE_NOT_MATCH); // check for overflow

		char key_name[64];
		char key_value[64];
		
		sfo_key s_key;
		
		memcpy(&s_key, sfo_buffer + ptr, sizeof(sfo_key));
		ptr += sizeof(sfo_key);

		if(s_key.type != PSF_TYPE_STR) continue;
		
		// calculate location of key in buffer
		int name_offset = header.key_offset + s_key.name_offset;
		if(name_offset > sfo_size) ERROR(SIZE_NOT_MATCH);
		
		int data_offset = header.value_offset + s_key.data_offset;
		if(data_offset > sfo_size) ERROR(SIZE_NOT_MATCH);

		// copy the key and value into buffers.
		strncpy(key_name, sfo_buffer + name_offset, sizeof(key_name)-1);
		strncpy(key_value, sfo_buffer + data_offset, sizeof(key_value)-1);		

		if(strncmp(key_name, key, sizeof(key_name)) == 0){
			strncpy(out, key_value, sizeof(key_value)-1);
			ERROR(0);
		}

	}
error:
	if(sfo_buffer != NULL)
		free(sfo_buffer);
	if(sfo_fd > 0) 
		sceIoClose(sfo_fd);
	return res;
}