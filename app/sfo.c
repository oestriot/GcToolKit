#include <vitasdk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "sfo.h"
#include "io.h"
#include "err.h"
#include <GcKernKit.h>

#define SPSFO_OFFSET (0x600)

int validate_sfo(SfoHandle* sfo) {
	if(sfo->size <= sizeof(SfoHeader)) return SIZE_NOT_MATCH;
	
	// get sfo header
	SfoHeader* header = (SfoHeader*)sfo->buffer;
	
	if(memcmp(header->magic, "\0PSF", sizeof(header->magic)) != 0) return INVALID_MAGIC; // check magic	
	if(header->count > 200) return SFO_TOO_MANY_ENTRIES; // give up if more than 200 keys
	if(sfo->size < (sizeof(SfoHeader) + (sizeof(SfoKey) * header->count))) return SIZE_NOT_MATCH; // check if size is enough for keys + sfo header size
	
	return OK;
}

int open_sfo_or_spsfo(const char* sfo_file, SfoHandle* sfo, int sp_sfo) {
	int res = -1;
	SceUID sfo_fd = 0;
	
	// clear sfo struct
	memset(sfo, 0x00, sizeof(SfoHandle));
	PRINT_STR("open sfo: %s\n", sfo_file);
	
	// skip if file not exist
	if(!file_exist(sfo_file)) ERROR(FILE_NOT_FOUND);
	
	// get sfo size ..
	sfo->size = get_file_size(sfo_file);
	PRINT_STR("sz: %llx\n", sfo->size);
	if(sfo->size <= 0) ERROR(SIZE_IS_ZERO);
	
	// minus the spsfo offset from the file ..
	if(sp_sfo) sfo->size -= SPSFO_OFFSET;
	PRINT_STR("spsfo sz: %llx\n", sfo->size);
	if(sfo->size <= 0) ERROR(SIZE_IS_ZERO);
	
	sfo->buffer = malloc(sfo->size);
	PRINT_STR("buffer: %p\n", sfo->buffer);
	if(sfo->buffer == NULL) ERROR(POINTER_WAS_NULL);

	memset(sfo->buffer, 0x00, sfo->size);
	
	// open sfo file
	sfo_fd = sceIoOpen(sfo_file, SCE_O_RDONLY, 0777);	
	PRINT_STR("sfo fd: %x\n", sfo_fd);
	if(sfo_fd < 0) ERROR(sfo_fd);
	
	
	if(sp_sfo) { // skip to spsfo offset
		SceOff loc = sceIoLseek(sfo_fd, SPSFO_OFFSET, SCE_SEEK_SET);
		PRINT_STR("position : %llx\n", loc);
		if(loc != SPSFO_OFFSET) ERROR(SIZE_NOT_MATCH);
	}
	
	// read sfo
	int rd = sceIoRead(sfo_fd, sfo->buffer, sfo->size);
	PRINT_STR("rd: %x\n", rd);
	if(rd != sfo->size) ERROR(SIZE_NOT_MATCH);
	
	// close sfo
	if(sceIoClose(sfo_fd) >= 0) sfo_fd = 0;
	
	res = validate_sfo(sfo);
	if(res != OK) ERROR(res);
	return OK;
	
error:
	if(sfo_fd > 0) sceIoClose(sfo_fd);
	close_sfo(sfo);
	return res;
}

int open_sp_sfo(const char* sfo_file, SfoHandle* sfo) {
	return open_sfo_or_spsfo(sfo_file, sfo, 1);
}

int open_sfo(const char* sfo_file, SfoHandle* sfo) {
	return open_sfo_or_spsfo(sfo_file, sfo, 0);
}

void close_sfo(SfoHandle* sfo) {
	if(sfo->buffer != NULL) free(sfo->buffer);
	sfo->buffer = NULL;
	sfo->size = 0;		
}

char magic[4];
uint32_t version;
uint32_t key_offset;
uint32_t value_offset;
uint32_t count;

int read_sfo_key(SfoHandle* sfo, const char* sfo_key, char* out_key, size_t out_key_size) {
	
	PRINT_STR("%p: read_sfo(%s)\n", sfo, sfo_key);
	
	PRINT_STR("buf: %p\n", sfo->buffer);
	PRINT_STR("sz: %llu\n", sfo->size);
	
	SfoHeader* header = (SfoHeader*)sfo->buffer;
	SfoKey* skey = (SfoKey*)&header[1];
	
	PRINT_STR("header->magic = %c%c%c%c\n", header->magic[0], header->magic[1], header->magic[2], header->magic[3]);
	PRINT_STR("header->version = %x\n", header->version);
	PRINT_STR("header->key_offset = %x\n", header->key_offset);
	PRINT_STR("header->value_offset = %x\n", header->value_offset);
	PRINT_STR("header->count = %i\n", header->count);

	PRINT_STR("header: %p\n", header);
	PRINT_STR("skey: %p\n", skey);
	
	// read keys
	for(int i = 0; i < header->count; i++)
	{
		char key_name[64];
		char key_value[64];
	
		PRINT_STR("skey[%i].name_offset = %x\n", i, skey[i].name_offset);
		PRINT_STR("skey[%i].alignment = %x\n", i, skey[i].alignment);
		PRINT_STR("skey[%i].type = %i\n", i, skey[i].type);
		PRINT_STR("skey[%i].value_size = %x\n", i, skey[i].value_size);
		PRINT_STR("skey[%i].total_size = %u\n", i, skey[i].total_size);
		PRINT_STR("skey[%i].data_offset = %x\n", i, skey[i].data_offset);
		
		if(skey[i].type != PSF_TYPE_STR) continue;
		
		// calculate location of key in buffer
		uintptr_t name_offset = header->key_offset + skey[i].name_offset;
		if(name_offset > sfo->size) return SIZE_NOT_MATCH;
		
		uintptr_t data_offset = header->value_offset + skey[i].data_offset;
		if(data_offset > sfo->size) return SIZE_NOT_MATCH;

		// copy the key and value into buffers.
		strncpy(key_name, (char*)(sfo->buffer + name_offset), sizeof(key_name)-1);
		strncpy(key_value, (char*)(sfo->buffer + data_offset), sizeof(key_value)-1);		

		PRINT_STR("key_name: %s, key_value: %s, out_key_size: %x\n", key_name, key_value, out_key_size);

		if(strncmp(key_name, sfo_key, out_key_size) == 0){
			strncpy(out_key, key_value, out_key_size-1);
			return OK;
		}
	}
	
	return SFO_KEY_NOT_FOUND;
}