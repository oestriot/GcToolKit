/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#include "compiler_defs.h"
#include "vita.h"
#include "cmd56.h"
#include "cmd56_sm.h"
#include "log.h"

#define check_success(code) do { int ret = code; if(ret != GC_AUTH_OK) return ret; } while(0);
#define send_packet(sobj, req, resp) do { PRINT_STR("send_packet: %s\n", __FUNCTION__); \
										  sobj->send( ((const uint8_t*)req), sizeof(cmd56_request)); \
										  sobj->recv( ((uint8_t*)resp),      sizeof(cmd56_response)); } while(0);



#define GC_AUTH_RETURN_STATUS (response->error_code != 0x0) ? GC_AUTH_ERROR_REPORTED : GC_AUTH_OK;


vita_error_code start_request(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_START, 0x3, calc_size(start_response), 0x31);
	send_packet(state, request, response);
	get_response(start_response);

	if (resp->start[0xD] == 0x1 && resp->start[0xE] == 0x1 && resp->start[0xF] == 0x4) {
		PRINT_STR("(VITA) START Response is what is expected.\n");
		return GC_AUTH_RETURN_STATUS;
	}
	PRINT_STR("(VITA) START Response is not what is expected: ");
	PRINT_BUFFER_LEN(resp->start, sizeof(resp->start));

	return GC_AUTH_ERROR_START_FAIL;
}

vita_error_code get_status(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_GET_STATUS, 0x3, calc_size(get_status_response), 0x23);
	send_packet(state, request, response);
	get_response(get_status_response);

	state->lock_status = resp->status;

	return GC_AUTH_RETURN_STATUS;
}

vita_error_code get_session_key(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_GENERATE_SESSION_KEY, 0x3, calc_size(generate_session_key_response), 0x2);
	send_packet(state, request, response);
	get_response(generate_session_key_response);

	state->key_id = endian_swap(resp->key_id);
	memcpy(state->cart_random, resp->cart_random, sizeof(state->cart_random));

	PRINT_STR("(VITA) Key ID: %x\n", state->key_id);
	PRINT_STR("(VITA) CART_RANDOM: ");
	PRINT_BUFFER_LEN(state->cart_random, sizeof(state->cart_random));

	if (state->allow_prototype_keys == 1 && state->key_id > PROTOTYPE_KEY_ID1) {
		PRINT_STR("(VITA) KeyID is > 0x8001, so PSVITA Firmware 1.04+ will reject this cart ...");
		return GC_AUTH_ERROR_GET_CART_RANDOM_PROTOTYPE_KEY;
	}

	// generate session key
	uint8_t session_key[0x10];
	derive_session_key(session_key, state->cart_random, state->key_id);

	PRINT_STR("(VITA) Session key: ");
	PRINT_BUFFER_LEN(session_key, 0x10);

	AES_init_ctx(&state->session_key, session_key);
	return GC_AUTH_RETURN_STATUS;
}

vita_error_code exchange_shared_random(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_EXCHANGE_SHARED_RANDOM, calc_size(exchange_shared_random_request), calc_size(exchange_shared_random_response), 0x3);
	get_request(exchange_shared_random_request);

	// copy key id into request
	PRINT_STR("(VITA) cart key id: %x\n", state->key_id);
	req->key_id = endian_swap(state->key_id);
	
	// randomize vita portion of shared random
	rand_bytes(req->shared_rand_vita, sizeof(req->shared_rand_vita));
	memcpy(state->shared_random.vita_part, req->shared_rand_vita, sizeof(state->shared_random.vita_part));

	PRINT_STR("(VITA) vita portion of the shared random: ");
	PRINT_BUFFER_LEN(req->shared_rand_vita, sizeof(req->shared_rand_vita));

	PRINT_STR("(VITA) verify_shared_random (request): ");
	PRINT_BUFFER_LEN(req, sizeof(exchange_shared_random_request));

	send_packet(state, request, response);
	get_response(exchange_shared_random_response);

	if (response->error_code == GC_AUTH_OK) {
		decrypt_cbc_zero_iv(&state->session_key, resp, sizeof(exchange_shared_random_response));

		PRINT_STR("(VITA) verify_shared_random (response) plaintext: ");
		PRINT_BUFFER_LEN(resp, sizeof(exchange_shared_random_response));

		if (memcmp(resp->shared_rand_vita + 0x1, state->shared_random.vita_part + 0x1, sizeof(state->shared_random.vita_part)-0x1) == 0) {
			PRINT_STR("(VITA) cart and vita have the same shared_random.vita_part ...\n");
			
			// copy cart part into global state shared_random
			memcpy(state->shared_random.cart_part, resp->shared_rand_cart, sizeof(resp->shared_rand_cart));

			PRINT_STR("(VITA) shared random, cart part: ");
			PRINT_BUFFER_LEN(state->shared_random.cart_part, sizeof(state->shared_random.cart_part));

			PRINT_STR("(VITA) shared random: ");
			PRINT_BUFFER_LEN(&state->shared_random, sizeof(shared_random));

			return GC_AUTH_OK;
		}
		else {
			PRINT_STR("(VITA) invalid shared_random.vita_part! got: ");
			PRINT_BUFFER_LEN(resp->shared_rand_vita, sizeof(resp->shared_rand_vita));
			PRINT_STR("(VITA) expected: ");
			PRINT_BUFFER_LEN(req->shared_rand_vita, sizeof(req->shared_rand_vita));

			return GC_AUTH_ERROR_VERIFY_SHARED_RANDOM_INVALID;
		}
		return GC_AUTH_ERROR_VERIFY_SHARED_RANDOM_FAIL;
	}

	PRINT_STR("(VITA) response->error_code: 0x%X\n", response->error_code);
	return GC_AUTH_ERROR_REPORTED;
}

vita_error_code generate_secondary_key_and_verify_session(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_EXCHANGE_SECONDARY_KEY_AND_VERIFY_SESSION, calc_size(exchange_secondary_key_and_verify_session_request), 0x3, 0x5);
	get_request(exchange_secondary_key_and_verify_session_request);
	
	rand_bytes(req->secondary_key, sizeof(req->secondary_key));
	AES_init_ctx(&state->secondary_key, req->secondary_key);
	PRINT_STR("(VITA) secondary_key: ");
	PRINT_BUFFER_LEN(req->secondary_key, sizeof(req->secondary_key));
	
	// copy shared_random to challenge
	or_w_80(&state->shared_random, sizeof(state->shared_random));
	memcpy(&req->challenge_bytes, &state->shared_random, sizeof(state->shared_random));
	PRINT_STR("(VITA) challenge bytes: ");
	PRINT_BUFFER_LEN(&req->challenge_bytes, sizeof(req->challenge_bytes));

	PRINT_STR("(VITA) plaintext secondary_key_and_verify_session: ");
	PRINT_BUFFER_LEN(req, sizeof(exchange_secondary_key_and_verify_session_request));

	encrypt_cbc_zero_iv(&state->session_key, req, sizeof(exchange_secondary_key_and_verify_session_request));

	PRINT_STR("(VITA) ciphertext secondary_key_and_verify_session: ");
	PRINT_BUFFER_LEN(req, sizeof(exchange_secondary_key_and_verify_session_request));

	send_packet(state, request, response);

	if (response->error_code != GC_AUTH_OK) {
		PRINT_STR("(VITA) session_key challenge failed: (error code = 0x%X)\n", response->error_code);
		return GC_AUTH_ERROR_REPORTED;
	}
	else {
		PRINT_STR("(VITA) session_key challenge passed!!\n");
		return GC_AUTH_OK;
	}
}

vita_error_code verify_secondary_key(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_VERIFY_SECONDARY_KEY, calc_size(verify_secondary_key_request), calc_size(verify_secondary_key_response), 0x7);
	get_request(verify_secondary_key_request);

	// generate challenge bytes 
	rand_bytes_or_w_80(req->challenge_bytes, sizeof(req->challenge_bytes));
	PRINT_STR("(VITA) generated challenge bytes: ");
	PRINT_BUFFER_LEN(req->challenge_bytes, sizeof(req->challenge_bytes));

	send_packet(state, request, response);
	get_response(verify_secondary_key_response);

	// decrypt response ...
	decrypt_cbc_zero_iv(&state->secondary_key, resp, sizeof(verify_secondary_key_response));
	PRINT_STR("(VITA) decrypted secondary_key challenge: ");
	PRINT_BUFFER_LEN(resp, sizeof(verify_secondary_key_response));

	// replicate off-by-one bug in the vita kernel when comparing challenge bytes
	if (memcmp(resp->challenge_bytes+0x1, req->challenge_bytes+0x1, sizeof(resp->challenge_bytes) - 1) == 0) { 
		PRINT_STR("(VITA) decrypted secondary_key challenge matches !\n");

		if (memcmp(resp->cart_random, state->cart_random, sizeof(state->cart_random)) == 0) {
			PRINT_STR("(VITA) cart_random matches!\n");
			return GC_AUTH_RETURN_STATUS;
		}
		else {
			PRINT_STR("(VITA) cart_random invalid! got: ");
			PRINT_BUFFER_LEN(resp->cart_random, sizeof(state->cart_random));
			PRINT_STR("expected: ");
			PRINT_BUFFER_LEN(state->cart_random, sizeof(state->cart_random));

			return GC_AUTH_ERROR_VERIFY_CART_RANDOM_INVALID_CART_RANDOM;
		}
	}
	else {
		PRINT_STR("(VITA) invalid challenge bytes! got: ");
		PRINT_BUFFER_LEN(resp->challenge_bytes+1, 0xF);
		PRINT_STR("(VITA) expected: ");
		PRINT_BUFFER_LEN(req->challenge_bytes+1, 0xF);

		return GC_AUTH_ERROR_VERIFY_CART_RANDOM_CHALLENGE_INVALID;
	}

	return GC_AUTH_ERROR_VERIFY_CART_RANDOM_FAIL;
}

vita_error_code get_packet18_key(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response, uint8_t type) {
	cmd56_request_start(request, CMD_GET_P18_KEY_AND_CMAC_SIGNATURE, calc_size(get_p18_key_and_cmac_signature_request), calc_size(get_p18_key_and_cmac_signature_response), 0x11);
	get_request(get_p18_key_and_cmac_signature_request);

	uint8_t expected_challenge[0x10];
	rand_bytes_or_w_80(expected_challenge, sizeof(expected_challenge));
	
	memcpy(req->challenge_bytes, expected_challenge, sizeof(expected_challenge));
	memset(req->pad0, 0x00, sizeof(req->pad0));
	
	// i dont know what this is for, its just all that changes between the two calls to it, 
	// honestly i dont know why this is command is issued twice;
	req->type = type;
	PRINT_STR("(VITA) get_p18_key: type 0x%x\n", type);

	// log before encrypt
	PRINT_STR("(VITA) p18 exp_challenge decrypted: ");
	PRINT_BUFFER_LEN(req, sizeof(get_p18_key_and_cmac_signature_request));

	encrypt_cbc_zero_iv(&state->secondary_key, req, sizeof(get_p18_key_and_cmac_signature_request));

	// create a cmac of all the p18 data, place it at the end of the request.
	do_cmd56_cmac_hash(&state->secondary_key, 
						req, 
						make_int24(request->command, 0x00, request->additional_data_size), 
						req->cmac_signature,
						offsetof(get_p18_key_and_cmac_signature_request, cmac_signature));

	// log after encrypt
	PRINT_STR("(VITA) plaintext p18 request data: ");
	PRINT_BUFFER_LEN(request->data, 0x30);

	send_packet(state, request, response);
	get_response(get_p18_key_and_cmac_signature_response);

	if (response->error_code == GC_AUTH_OK) { // check status from gc
		uint8_t expected_cmac[0x10];

		// generate p18 cmac
		do_cmd56_cmac_hash(&state->secondary_key, 
						   resp, 
			               response->response_size, 
			               expected_cmac, 
			               offsetof(get_p18_key_and_cmac_signature_response, cmac_signature));

		if (memcmp(expected_cmac, resp->cmac_signature, sizeof(expected_cmac)) == 0) { // check cmac
			PRINT_STR("(VITA) CMAC Matches!\n");

			// decrypt buffer
			decrypt_cbc_zero_iv(&state->secondary_key, resp, offsetof(get_p18_key_and_cmac_signature_response, cmac_signature));

			PRINT_STR("(VITA) decrypted p18 response: ");
			PRINT_BUFFER_LEN(resp, sizeof(get_p18_key_and_cmac_signature_response));

			// the first byte doesnt have to match.
			if (memcmp(expected_challenge+0x1, resp->challenge_bytes+0x1, sizeof(expected_challenge) - 0x1) == 0) { 
				PRINT_STR("(VITA) p18 challenge success!\n");

				memcpy(state->per_cart_keys.packet18_key, resp->p18_key, sizeof(state->per_cart_keys.packet18_key));
				PRINT_STR("(VITA) state->per_cart_keys.packet18_key: ");
				PRINT_BUFFER_LEN(state->per_cart_keys.packet18_key, sizeof(state->per_cart_keys.packet18_key));

				return GC_AUTH_OK;

			}
			else {
				PRINT_STR("(VITA) Invalid p18 challenge response! got: ");
				PRINT_BUFFER_LEN(resp->challenge_bytes+0x1, sizeof(expected_challenge)-0x1);
				PRINT_STR("(VITA) expected: ");
				PRINT_BUFFER_LEN(expected_challenge+0x1, sizeof(expected_challenge)-0x1);

				return GC_AUTH_ERROR_P18_KEY_CHALLANGE_FAIL;
			}
		}
		else {
			PRINT_STR("(VITA) Invalid p18 CMAC! got: ");
			PRINT_BUFFER_LEN(resp->cmac_signature, 0x10);
			PRINT_STR("(VITA) expected: ");
			PRINT_BUFFER_LEN(expected_cmac, 0x10);

			return GC_AUTH_ERROR_P18_KEY_INVALID_CMAC;
		}
	}
	PRINT_STR("(VITA) response->error_code: 0x%X\n", response->error_code);
	return GC_AUTH_ERROR_REPORTED;
}

vita_error_code get_packet20_key(vita_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_request_start(request, CMD_GET_P20_KEY_AND_CMAC_SIGNATURE, calc_size(get_p20_key_and_cmac_signature_request), calc_size(get_p20_key_and_cmac_signature_response), 0x19);
	get_request(get_p20_key_and_cmac_signature_request);
	rand_bytes_or_w_80(req->challenge_bytes, sizeof(req->challenge_bytes));

	PRINT_STR("(VITA) p20 request: ");
	PRINT_BUFFER_LEN(req, sizeof(get_p20_key_and_cmac_signature_request));

	send_packet(state, request, response);
	get_response(get_p20_key_and_cmac_signature_response);

	if (response->error_code == GC_AUTH_OK) {
		uint8_t expected_cmac[0x10];
		
		// generate p20 cmac
		do_cmd56_cmac_hash(&state->secondary_key, 
							resp, 
							response->response_size, 
							expected_cmac, 
							offsetof(get_p20_key_and_cmac_signature_response, cmac_signature));

		if (memcmp(expected_cmac, resp->cmac_signature, sizeof(expected_cmac)) == 0) { // cmac check
			PRINT_STR("(VITA) p20 cmac check pass\n");

			// decrypt response
			decrypt_cbc_zero_iv(&state->secondary_key, resp, offsetof(get_p20_key_and_cmac_signature_response, cmac_signature));

			PRINT_STR("(VITA) decrypted p20 response data:");
			PRINT_BUFFER_LEN(resp, sizeof(get_p20_key_and_cmac_signature_response));

			if (memcmp(req->challenge_bytes+0x1, resp->challenge_bytes+0x1, sizeof(resp->challenge_bytes)-1) == 0) { // challenge check
				PRINT_STR("(VITA) p20 challenge matches!\n");

				memcpy(state->per_cart_keys.packet20_key, resp->p20_key, sizeof(state->per_cart_keys.packet20_key));
				PRINT_STR("(VITA) state->per_cart_keys.packet20_key: ");
				PRINT_BUFFER_LEN(state->per_cart_keys.packet20_key, sizeof(state->per_cart_keys.packet20_key));

				return GC_AUTH_OK;
			}
			else {
				PRINT_STR("(VITA) Invalid Challenge Response! got: ");
				PRINT_BUFFER_LEN(resp->challenge_bytes+0x1, 0xF);
				PRINT_STR("expected: ");
				PRINT_BUFFER_LEN(req->challenge_bytes+0x1, 0xF);

				return GC_AUTH_ERROR_P20_KEY_CHALLANGE_FAIL;
			}
		}
		else {
			PRINT_STR("(VITA) Invalid CMAC! got: ");
			PRINT_BUFFER_LEN(resp->cmac_signature, 0x10);
			PRINT_STR("expected: ");
			PRINT_BUFFER_LEN(expected_cmac, 0x10);

			return GC_AUTH_ERROR_P20_KEY_INVALID_CMAC;
		}
	}
	PRINT_STR("(VITA) response->error_code: 0x%X\n", response->error_code);
	return GC_AUTH_ERROR_REPORTED;
}

// exposed functions

void vita_cmd56_init(vita_cmd56_state* state, send_t send_func, recv_t recv_func) {
	if (state == NULL) return;

	memset(state, 0x00, sizeof(vita_cmd56_state));
	state->send = send_func;
	state->recv = recv_func;
										 // false, replicates the functionality of 1.04+ firmware, prototype carts not allowed.
	state->allow_prototype_keys = false; // true, it acts more like a prototype <1.04 console, allowing prototype carts.
}

void vita_cmd56_init_ex(vita_cmd56_state* state, send_t send_func, recv_t recv_func, bool allow_prototype_keys) {
	if (state == NULL) return;

	vita_cmd56_init(state, send_func, recv_func);
	state->allow_prototype_keys = true;
}

void vita_cmd56_get_keyid(vita_cmd56_state* state, cmd56_sm_keyid* key_id) {
	if (state == NULL) return;
	if (key_id != NULL) memcpy(key_id, &state->key_id, sizeof(state->key_id));
}

void vita_cmd56_get_keys(vita_cmd56_state* state, cmd56_keys* per_cart_keys) {
	if (state == NULL) return;

	if (per_cart_keys != NULL) memcpy(per_cart_keys, &state->per_cart_keys, sizeof(cmd56_keys));
}
void vita_cmd56_get_keys_ex(vita_cmd56_state* state, uint8_t p20_key[0x20], uint8_t p18_key[0x20]) {
	if (state == NULL) return;

	if (p20_key != NULL) memcpy(p20_key, state->per_cart_keys.packet20_key, sizeof(state->per_cart_keys.packet20_key));
	if (p18_key != NULL) memcpy(p18_key, state->per_cart_keys.packet18_key, sizeof(state->per_cart_keys.packet18_key));
}

int vita_cmd56_run(vita_cmd56_state* state) {
	if (state == NULL) return GC_INVALID_ARGUMENT;

	cmd56_request request;
	cmd56_response response;

	memset(&request, 0x00, sizeof(cmd56_request));
	memset(&response, 0x00, sizeof(cmd56_response));

	check_success(start_request(state, &request, &response)); // initalize gc
	check_success(get_status(state, &request, &response)); // check is locked
	if (state->lock_status != GC_LOCKED) return GC_AUTH_ERROR_UNLOCKED; // error if is not locked

	check_success(get_session_key(state, &request, &response)); // get cart random, and keyid 
	check_success(exchange_shared_random(state, &request, &response)); // send vita portion of shared random and receive gc portion.
	check_success(generate_secondary_key_and_verify_session(state, &request, &response)); // generate vita authenticity proof

	check_success(get_status(state, &request, &response)); // check is unlocked
	if (state->lock_status != GC_UNLOCKED) return GC_AUTH_ERROR_LOCKED; // error if is not unlocked
	
	check_success(verify_secondary_key(state, &request, &response)); // check if secondary_key was obtained by the cart.
	check_success(get_packet18_key(state, &request, &response, 0x2)); // get packet18 key, and verify cmac
	check_success(get_packet18_key(state, &request, &response, 0x3)); // for some reason this gets sent twice,
	check_success(get_packet20_key(state, &request, &response)); // get packet20 key.
	
	return GC_AUTH_OK;
}