#include "compiler_defs.h"
#include "cmd56.h"
#include "log.h"
#include "crypto/aes.h"
#include "crypto/aes_cmac.h"

const uint8_t CMD56_MAGIC[0x20] = { 0xDD, 0x10, 0x25, 0x44, 0x15, 0x23, 0xFD, 0xC0, 0xF9, 0xE9, 0x15, 0x26, 0xDC, 0x2A, 0xE0, 0x84, 0xA9, 0x03, 0xA2, 0x97, 0xD4, 0xBB, 0xF8, 0x52, 0xD3, 0xD4, 0x94, 0x2C, 0x89, 0x03, 0xCC, 0x77 };

void cmd56_request_start(cmd56_request* request, cmd56_command cmd, uint8_t additional_data_size, uint32_t expected_response_size, uint32_t expected_response_code) {
	memset(request, 0x00, sizeof(cmd56_request));
	memcpy(request->magic, CMD56_MAGIC, sizeof(CMD56_MAGIC));
	request->command = cmd;
	request->expected_response_code = expected_response_code;
	request->expected_response_size = expected_response_size;
	request->additional_data_size = additional_data_size;
	request->request_size = additional_data_size;
	PRINT_STR("(CMD56) cmd56_request_start cmd=0x%x resp_code=0x%x\n", request->command, request->expected_response_code);
}

void cmd56_response_start(cmd56_request* request_buffer, cmd56_response* response) {
	memset(response, 0x00, sizeof(cmd56_response));
	response->response_code = request_buffer->expected_response_code;
	response->additional_data_size = 0;
	response->response_size = endian_swap((request_buffer->expected_response_size > sizeof(cmd56_response)) ? sizeof(cmd56_response) : (uint16_t)request_buffer->expected_response_size);
	response->error_code = 0;
}

void cmd56_response_error(cmd56_response* response, uint8_t error) {
	response->error_code = error;
	response->response_code = error;

	uint16_t size = endian_swap((uint16_t)response->response_size);
	memset(response->data, 0xFF, size);
	
	PRINT_STR("(CMD56) cmd56_response_error error_code=0x%x\n", response->error_code);
}
