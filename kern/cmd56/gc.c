/*
*	LibCmd56 from the Estroit team!
*	the only functional implementation of vita gamecart authentication!
*/

#include "log.h"
#include "gc.h"
#include "cmd56.h"
#include "cmd56_sm.h"

#include "crypto/aes.h"
#include "crypto/aes_cmac.h"

void handle_cmd_start(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response){
	cmd56_response_start(request, response);
	get_response(start_response);
	state->lock_status = GC_LOCKED;

	resp->start[0xD] = 0x1;
	resp->start[0xE] = 0x1;
	resp->start[0xF] = 0x4;
}

void handle_cmd_status(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_response(get_status_response);

	resp->status = state->lock_status;
}

void handle_generate_session_key(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_response(generate_session_key_response);

	resp->unk0 = endian_swap(0xE000);

	// specify key_id
	PRINT_STR("(GC) Key ID %x\n", state->key_id);
	resp->key_id = endian_swap(state->key_id);

	// unknown paramaters, copied values from "Smart As."; they seem to be the same for most*
	// games i have, with few exceptions (noted below)

	// the vita does nothing with these, so i can't easily know what there for
	// this is just included incase they decide to do something with them in a later firmware..
	// also seems to be a bug where only half of the CART_RANDDOM is actually random, its weird

	resp->unk1 = endian_swap(0x2);
	resp->unk2 = endian_swap(0x3);

	rand_bytes(resp->cart_random, sizeof(resp->cart_random));

	resp->cart_random[0x0] = 0x00;
	resp->cart_random[0x1] = 0x01;

	resp->cart_random[0x2] = 0x00;
	resp->cart_random[0x3] = 0x01;

	resp->cart_random[0x4] = 0x00;
	resp->cart_random[0x5] = 0x00;
	resp->cart_random[0x6] = 0x00;
	resp->cart_random[0x7] = 0x00;

	resp->cart_random[0x8] = 0x00;
	resp->cart_random[0x9] = 0x00;
	resp->cart_random[0xA] = 0x00;
	resp->cart_random[0xB] = 0x04; // 0x1 in Final Fantasy X (JPN/CN), 
								   // 0x1 in Dungeon Travelers 2 (JPN), 
								   // 0x1 in Diabolik Lovers (JPN), 
								   // 0x1 in Sen No Kieski 2 (JPN),
								   // 0x1 in Sen No Kieski (JPN),
								   // 0x3 in Superdimension Neptune vs Sega Hard Girls (USA), 
								   // 0x4 in Smart As (PAL),
								   // 0x4 in Hyperdevotion Noire (PAL)
									
	resp->cart_random[0xC] = 0x00; // 0x0 in all except:
								   // Minecraft PSVita (JPN) where its 0x1.
								   // Diabolik Lovers(JPN) where its 0x2.

	memcpy(state->cart_random, resp->cart_random, sizeof(state->cart_random));

	PRINT_STR("(GC) cart_random: ");
	PRINT_BUFFER_LEN(state->cart_random, sizeof(state->cart_random));

	// generate session key
	uint8_t session_key[0x10];
	derive_session_key(session_key, state->cart_random, state->key_id);

	PRINT_STR("(GC) session_key: ");
	PRINT_BUFFER_LEN(session_key, sizeof(session_key));

	AES_init_ctx(&state->session_key, session_key);
}

void handle_exchange_shared_random(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_response(exchange_shared_random_response);
	get_request(exchange_shared_random_request);

	if (endian_swap(req->key_id) == state->key_id) {
		PRINT_STR("(GC) got_key_id == state->key_id\n");

		memcpy(state->shared_random.vita_part, req->shared_rand_vita, sizeof(req->shared_rand_vita));
		or_w_80(state->shared_random.vita_part, sizeof(state->shared_random.vita_part));

		PRINT_STR("(GC) read vita portion of the shared random: ");
		PRINT_BUFFER_LEN(req->shared_rand_vita, sizeof(req->shared_rand_vita));

		// gamecart decides the lower portion of shared random ...
		rand_bytes_or_w_80(state->shared_random.cart_part, sizeof(state->shared_random.cart_part));

		PRINT_STR("(GC) generated gc portion of the shared random: ");
		PRINT_BUFFER_LEN(state->shared_random.cart_part, sizeof(state->shared_random.cart_part));

		// this is sent back to the console in reverse order ...
		memcpy(resp->shared_rand_cart, state->shared_random.cart_part, sizeof(resp->shared_rand_cart));
		memcpy(resp->shared_rand_vita, state->shared_random.vita_part, sizeof(resp->shared_rand_vita));

		PRINT_STR("(GC) handle_shared_random plaintext: ");
		PRINT_BUFFER_LEN(resp, sizeof(exchange_shared_random_response));

		encrypt_cbc_zero_iv(&state->session_key, resp, sizeof(exchange_shared_random_response));
	}
	else {
		PRINT_STR("(GC) key_id from vita not acknowledged? (got: 0x%02X, expected: 0x%02X)\n", endian_swap(req->key_id), state->key_id);
		cmd56_response_error(response, 0x11);
	}

}

void handle_exchange_secondary_key_and_verify_session(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_request(exchange_secondary_key_and_verify_session_request);

	// decrypt the request ...
	PRINT_STR("(GC) encrypted request buffer: ");
	PRINT_BUFFER_LEN(req, sizeof(exchange_secondary_key_and_verify_session_request));

	decrypt_cbc_zero_iv(&state->session_key, req, sizeof(exchange_secondary_key_and_verify_session_request));

	// log everything
	PRINT_STR("(GC) decrypted request buffer: ");
	PRINT_BUFFER_LEN(req, sizeof(exchange_secondary_key_and_verify_session_request));

	PRINT_STR("(GC) got_challenge: ");
	PRINT_BUFFER_LEN(&req->challenge_bytes, sizeof(req->challenge_bytes));

	PRINT_STR("(GC) secondary_key: ");
	PRINT_BUFFER_LEN(req->secondary_key, sizeof(req->secondary_key));
	AES_init_ctx(&state->secondary_key, req->secondary_key);

	if (memcmp(&req->challenge_bytes, &state->shared_random, sizeof(shared_random)) == 0) {
		PRINT_STR("(GC) session key validated, unlocking cart\n");
		state->lock_status = GC_UNLOCKED;
	}
	else {
		PRINT_STR("(GC) session key not valid, cart remaining locked.\n");

		PRINT_STR("(GC) expected: ");
		PRINT_BUFFER_LEN(&state->shared_random, sizeof(shared_random));

		PRINT_STR("(GC) got: ");
		PRINT_BUFFER_LEN(&req->challenge_bytes, sizeof(shared_random));

		state->lock_status = GC_LOCKED;
		cmd56_response_error(response, 0xF1);
	}
}

void handle_verify_secondary_key(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_response(verify_secondary_key_response);
	get_request(verify_secondary_key_request);

	// padding is random, so we randomize the entire response first.
	rand_bytes(resp, sizeof(verify_secondary_key_response));

	// copy challenge data
	memcpy(resp->challenge_bytes, req->challenge_bytes, sizeof(req->challenge_bytes));
	or_w_80(resp->challenge_bytes, sizeof(resp->challenge_bytes));

	PRINT_STR("(GC) Got challenge bytes: ");
	PRINT_BUFFER_LEN(resp->challenge_bytes, sizeof(resp->challenge_bytes));

	// copy cart random
	memcpy(resp->cart_random, state->cart_random, sizeof(state->cart_random));

	PRINT_STR("(GC) Plaintext verify_secondary_key response: ");
	PRINT_BUFFER_LEN(resp, sizeof(verify_secondary_key_response));

	encrypt_cbc_zero_iv(&state->secondary_key, response->data, 0x40);
}


void handle_p18key_and_cmac_signature(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_response(get_p18_key_and_cmac_signature_response);
	get_request(get_p18_key_and_cmac_signature_request);

	// calcluate cmac and get expected cmac
	uint8_t got_cmac[0x10];
	
	do_cmd56_cmac_hash(&state->secondary_key, 
						req,
						make_int24(request->command, 0x0, request->additional_data_size), 
						got_cmac, 
						offsetof(get_p18_key_and_cmac_signature_request, cmac_signature));

	if (memcmp(got_cmac, req->cmac_signature, sizeof(got_cmac)) == 0) {
		PRINT_STR("(GC) p18 cmac validated success\n");
		decrypt_cbc_zero_iv(&state->secondary_key, req, offsetof(get_p18_key_and_cmac_signature_request, cmac_signature));

		// check type really is 0x2 or 0x3, 
		// and that the challenge value is or'd by 0x80 ...
		if ((req->type != 0x2 || req->type != 0x3) &&
			(req->challenge_bytes[0x00] | 0x80) != req->challenge_bytes[0x00]) {
			PRINT_STR("(GC) invalid p18 request, 0x1F is not 0x2 or 0x3, OR challenge_bytes[0] is not logical or'd with 0x80.\n");
			cmd56_response_error(response, 0x11);
			return;
		}
	
		PRINT_STR("(GC) decrypted p18 request buffer: ");
		PRINT_BUFFER_LEN(req, sizeof(get_p18_key_and_cmac_signature_request));

		// copy challange to response
		memcpy(resp->challenge_bytes, req->challenge_bytes, sizeof(req->challenge_bytes));
		PRINT_STR("(GC) response challenge_bytes: ");
		PRINT_BUFFER_LEN(resp->p18_key, sizeof(resp->p18_key));

		// copy request18 key
		memcpy(resp->p18_key, state->per_cart_keys.packet18_key, sizeof(resp->p18_key));
		PRINT_STR("(GC) p18_key: ");
		PRINT_BUFFER_LEN(resp->p18_key, sizeof(resp->p18_key));

		// encrypt buffer
		encrypt_cbc_zero_iv(&state->secondary_key, resp, offsetof(get_p18_key_and_cmac_signature_response, cmac_signature));

		// aes-128-cmac the whole thing
		do_cmd56_cmac_hash(&state->secondary_key, 
						    resp, 
						    response->response_size, 
						    resp->cmac_signature, 
						    offsetof(get_p18_key_and_cmac_signature_response, cmac_signature));
	}
	else {
		PRINT_STR("(GC) p18 cmac validation failed!!\n");
		PRINT_STR("(GC) expected: ");
		PRINT_BUFFER_LEN(req->cmac_signature, sizeof(req->cmac_signature));
		PRINT_STR("(GC) got:");
		PRINT_BUFFER_LEN(got_cmac, sizeof(got_cmac));
		cmd56_response_error(response, 0xF4);
	}

}

void handle_p20key_and_cmac_signature(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	cmd56_response_start(request, response);
	get_response(get_p20_key_and_cmac_signature_response);
	get_request(get_p20_key_and_cmac_signature_request);

	// padding is random, so randomize the entire response.
	rand_bytes(resp, sizeof(get_p20_key_and_cmac_signature_response));
	
	// copy challenge bytes
	memcpy(resp->challenge_bytes, req->challenge_bytes, sizeof(req->challenge_bytes));
	or_w_80(resp->challenge_bytes, sizeof(resp->challenge_bytes));

	PRINT_STR("(GC) p20_challenge_value: ");
	PRINT_BUFFER_LEN(resp->challenge_bytes, sizeof(resp->challenge_bytes));

	// copy p20 key
	PRINT_STR("(GC) copying p20 key.\n");
	memcpy(resp->p20_key, state->per_cart_keys.packet20_key, sizeof(resp->p20_key));

	PRINT_STR("(GC) p20 plaintext response: ");
	PRINT_BUFFER_LEN(resp, offsetof(get_p20_key_and_cmac_signature_response, cmac_signature));

	// encrypt buffer
	encrypt_cbc_zero_iv(&state->secondary_key, resp, offsetof(get_p20_key_and_cmac_signature_response, cmac_signature));

	// aes-128-cmac the whole thing
	do_cmd56_cmac_hash(&state->secondary_key, 
						resp, 
						response->response_size, 
						resp->cmac_signature,
						offsetof(get_p20_key_and_cmac_signature_response, cmac_signature));
}


void handle_unknown_request(gc_cmd56_state* state, cmd56_request* request, cmd56_response* response) {
	PRINT_STR("(GC) Unknown command: 0x%02X\n", request->command);
	cmd56_response_start(request, response);
	cmd56_response_error(response, 0x11);
}

void handle_request(gc_cmd56_state* state, cmd56_request* request, cmd56_response* request_response) {
	switch(request->command) {
		case CMD_START: //packet1, packet2
			handle_cmd_start(state, request, request_response);
			break;
		case CMD_GET_STATUS: // packet3, packet4
			handle_cmd_status(state, request, request_response);
			break;
		case CMD_GENERATE_SESSION_KEY: // packet5, packet6
			handle_generate_session_key(state, request, request_response);
			break;
		case CMD_EXCHANGE_SHARED_RANDOM: // packet7, packet8
			handle_exchange_shared_random(state, request, request_response);
			break;
		case CMD_EXCHANGE_SECONDARY_KEY_AND_VERIFY_SESSION: // packet9, packet10
			handle_exchange_secondary_key_and_verify_session(state, request, request_response);
			break;
			
		// packet11, packet12 -> CMD_GET_STATUS again
		// checking if the cart is unlocked for reading / writing.

		case CMD_VERIFY_SECONDARY_KEY: // packet13, packet14
			handle_verify_secondary_key(state, request, request_response);
			break;

		case CMD_GET_P18_KEY_AND_CMAC_SIGNATURE: // packet15, packet16
			handle_p18key_and_cmac_signature(state, request, request_response);
			break;

		// packet17, packet18 -> P18_KEY_AND_CMAC_SIGNATURE again

		case CMD_GET_P20_KEY_AND_CMAC_SIGNATURE: // packet19, packet20
			handle_p20key_and_cmac_signature(state, request, request_response);
			break;
		default:
			handle_unknown_request(state, request, request_response);
			break;
	}
}

// exposed functions :

void gc_cmd56_update_keyid(gc_cmd56_state* state, uint16_t key_id) {
	if (state == NULL) return;
	state->key_id = key_id;

	// seed the rng from per game key id ...
	rand_seed(&key_id, sizeof(key_id));
}

void gc_cmd56_update_keys_ex(gc_cmd56_state* state, const uint8_t p20_key[0x20], const uint8_t p18_key[0x20]) {
	if (state == NULL) return;
	if (p20_key != NULL) memcpy(&state->per_cart_keys.packet20_key, p20_key, sizeof(state->per_cart_keys.packet20_key));
	if (p18_key != NULL) memcpy(&state->per_cart_keys.packet18_key, p18_key, sizeof(state->per_cart_keys.packet18_key));
	
	// seed the rng from per game keys ...
	if (p20_key != NULL) rand_seed(p20_key, sizeof(state->per_cart_keys.packet20_key));
	if (p18_key != NULL) rand_seed(p18_key, sizeof(state->per_cart_keys.packet18_key));
}

void gc_cmd56_update_keys(gc_cmd56_state* state, const cmd56_keys* per_cart_keys) {
	if (state == NULL) return;
	if (per_cart_keys != NULL) gc_cmd56_update_keys_ex(state, per_cart_keys->packet20_key, per_cart_keys->packet18_key);
}

void gc_cmd56_init(gc_cmd56_state* state, const cmd56_keys* per_cart_keys) {
	if (state == NULL) return;
	memset(state, 0x00, sizeof(gc_cmd56_state)); 

	// lock "cart" for reading/writing
	state->lock_status = GC_LOCKED;

	// set default key id
	gc_cmd56_update_keyid(state, RETAIL_KEY_ID);

	// set cart specific keys
	gc_cmd56_update_keys(state, per_cart_keys);
}

void gc_cmd56_run_in_place(gc_cmd56_state* state, uint8_t* buffer) {
	if (state == NULL) return;
	uint8_t cmd56_request_response[0x200];
	gc_cmd56_run(state, buffer, cmd56_request_response);
	memcpy(buffer, cmd56_request_response, sizeof(cmd56_request_response));
}

void gc_cmd56_run(gc_cmd56_state* state, const uint8_t* buffer, uint8_t* response) {
	if (state == NULL) return;
	cmd56_request* request = (cmd56_request*)buffer;
	if(memcmp(request->magic, CMD56_MAGIC, sizeof(request->magic)) != 0) {
		return;
	}

	handle_request(state, (cmd56_request*)buffer, (cmd56_response*)response);
}