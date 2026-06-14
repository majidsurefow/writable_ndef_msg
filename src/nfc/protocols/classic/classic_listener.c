/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic_listener.c
 */

#include "protocols/classic/classic_listener.h"

#include "protocols/classic/classic_i.h"
#include "protocols/classic/crypto1.h"
#include "protocols/classic/iso14443_crc.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#define CLASSIC_LISTENER_NT_SIZE 4U
#define CLASSIC_LISTENER_NRAR_SIZE 8U

static classic_data_t s_model;
static bool s_initialized;
static struct crypto1 s_crypto;
static uint8_t s_nt[CLASSIC_LISTENER_NT_SIZE];
static uint8_t s_auth_block;
static bool s_pending_auth2;
static bool s_auth_complete;
static uint8_t s_resp[32];

static void classic_listener_reset_auth(void)
{
	crypto1_reset(&s_crypto);
	s_pending_auth2 = false;
	s_auth_complete = false;
	s_auth_block = 0U;
}

static uint64_t classic_listener_key_a_for_block(uint8_t block_num)
{
	uint8_t sector = classic_get_sector_by_block(block_num);
	uint8_t first = classic_get_first_block_of_sector(sector);
	uint8_t trailer = (uint8_t)(first + classic_get_blocks_in_sector(sector) - 1U);
	uint64_t key = 0ULL;

	for (size_t i = 0U; i < CLASSIC_KEY_SIZE; i++) {
		key = (key << 8) | s_model.blocks[trailer][i];
	}

	return key;
}

static void classic_listener_make_nt(uint8_t sector, uint8_t nt[CLASSIC_LISTENER_NT_SIZE])
{
	uint32_t nt_num = 0x10000000U + (uint32_t)sector;

	nt[0] = (uint8_t)((nt_num >> 24) & 0xFFU);
	nt[1] = (uint8_t)((nt_num >> 16) & 0xFFU);
	nt[2] = (uint8_t)((nt_num >> 8) & 0xFFU);
	nt[3] = (uint8_t)(nt_num & 0xFFU);
}

static void classic_listener_send(const uint8_t *data, size_t len)
{
	(void)nfc_transport_send_response(data, len);
}

static bool classic_listener_block_valid(uint8_t block_num)
{
	uint16_t total = classic_get_total_blocks(s_model.type);

	return (s_model.type != CLASSIC_TYPE_UNKNOWN) && (block_num < total);
}

static void classic_listener_handle_auth_first(const uint8_t *tx, size_t tx_len, bool key_b)
{
	uint8_t block_num;
	uint8_t sector;
	uint64_t key;
	uint32_t cuid;
	uint32_t nt_num;

	ARG_UNUSED(key_b);

	if (tx_len != 2U) {
		return;
	}

	classic_listener_reset_auth();
	block_num = tx[1];
	if (!classic_listener_block_valid(block_num)) {
		return;
	}

	sector = classic_get_sector_by_block(block_num);
	key = classic_listener_key_a_for_block(block_num);
	classic_listener_make_nt(sector, s_nt);
	s_auth_block = block_num;

	cuid = classic_cuid_from_uid(s_model.uid, s_model.uid_len);
	nt_num = ((uint32_t)s_nt[0] << 24) | ((uint32_t)s_nt[1] << 16) |
		 ((uint32_t)s_nt[2] << 8) | (uint32_t)s_nt[3];

	crypto1_reset(&s_crypto);
	crypto1_init(&s_crypto, key);
	crypto1_word(&s_crypto, nt_num ^ cuid, 0);

	classic_listener_send(s_nt, sizeof(s_nt));
	s_pending_auth2 = true;
	s_auth_complete = false;
}

static void classic_listener_handle_auth_second(const uint8_t *tx, size_t tx_len)
{
	uint32_t nr_num;
	uint32_t ar_num;
	uint32_t nt_num;
	uint32_t secret_poller;
	uint32_t at_num;
	uint8_t at_plain[CLASSIC_LISTENER_NT_SIZE];
	uint8_t at_enc[CLASSIC_LISTENER_NT_SIZE];

	if ((tx_len != CLASSIC_LISTENER_NRAR_SIZE) || !s_pending_auth2) {
		return;
	}

	nr_num = ((uint32_t)tx[0] << 24) | ((uint32_t)tx[1] << 16) | ((uint32_t)tx[2] << 8) |
		 (uint32_t)tx[3];
	ar_num = ((uint32_t)tx[4] << 24) | ((uint32_t)tx[5] << 16) | ((uint32_t)tx[6] << 8) |
		 (uint32_t)tx[7];

	crypto1_word(&s_crypto, nr_num, 1);
	nt_num = ((uint32_t)s_nt[0] << 24) | ((uint32_t)s_nt[1] << 16) |
		 ((uint32_t)s_nt[2] << 8) | (uint32_t)s_nt[3];
	secret_poller = ar_num ^ crypto1_word(&s_crypto, 0U, 0);
	if (secret_poller != crypto1_prng_successor(nt_num, 64U)) {
		classic_listener_reset_auth();
		return;
	}

	at_num = crypto1_prng_successor(nt_num, 96U);
	at_plain[0] = (uint8_t)((at_num >> 24) & 0xFFU);
	at_plain[1] = (uint8_t)((at_num >> 16) & 0xFFU);
	at_plain[2] = (uint8_t)((at_num >> 8) & 0xFFU);
	at_plain[3] = (uint8_t)(at_num & 0xFFU);
	crypto1_encrypt(&s_crypto, NULL, at_plain, sizeof(at_plain), at_enc);
	classic_listener_send(at_enc, sizeof(at_enc));

	s_pending_auth2 = false;
	s_auth_complete = true;
}

static void classic_listener_handle_read(const uint8_t *tx, size_t tx_len)
{
	uint8_t plain[20];
	size_t plain_len;
	uint8_t block_num;
	uint8_t auth_sector;
	uint8_t read_sector;

	if (!s_auth_complete || (tx_len != 4U)) {
		return;
	}

	crypto1_decrypt(&s_crypto, tx, tx_len, plain);
	plain_len = tx_len;
	if (!iso14443_crc_check_a(plain, plain_len)) {
		return;
	}

	iso14443_crc_trim_a(plain, &plain_len);
	if ((plain_len != 2U) || (plain[0] != CLASSIC_CMD_READ_BLOCK)) {
		return;
	}

	block_num = plain[1];
	if (!classic_listener_block_valid(block_num)) {
		return;
	}

	auth_sector = classic_get_sector_by_block(s_auth_block);
	read_sector = classic_get_sector_by_block(block_num);
	if (auth_sector != read_sector) {
		return;
	}

	plain_len = CLASSIC_BLOCK_SIZE;
	(void)memcpy(plain, s_model.blocks[block_num], CLASSIC_BLOCK_SIZE);
	iso14443_crc_append_a(plain, &plain_len);
	crypto1_encrypt(&s_crypto, NULL, plain, plain_len, s_resp);
	classic_listener_send(s_resp, plain_len);
}

static void classic_listener_on_tag_cmd(const uint8_t *tx, size_t tx_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized || (tx == NULL) || (tx_len == 0U)) {
		return;
	}

	if (s_pending_auth2 && (tx_len == CLASSIC_LISTENER_NRAR_SIZE)) {
		classic_listener_handle_auth_second(tx, tx_len);
		return;
	}

	if (tx_len == 2U) {
		switch (tx[0]) {
		case CLASSIC_CMD_AUTH_KEY_A:
			classic_listener_handle_auth_first(tx, tx_len, false);
			break;
		case CLASSIC_CMD_AUTH_KEY_B:
			classic_listener_handle_auth_first(tx, tx_len, true);
			break;
		case CLASSIC_CMD_READ_BLOCK:
			break;
		default:
			break;
		}
		return;
	}

	if (s_auth_complete && (tx_len == 4U)) {
		classic_listener_handle_read(tx, tx_len);
	}
}

static int classic_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return classic_serialize(&s_model, out, out_max, out_len);
}

static int classic_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return classic_deserialize(&s_model, in, in_len);
}

static const nfc_service_t s_classic_listener_service = {
	.on_tag_cmd = classic_listener_on_tag_cmd,
	.serialize = classic_listener_serialize,
	.deserialize = classic_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_CLASSIC,
	.user_ctx = NULL,
};

int classic_listener_init(const classic_data_t *model)
{
	if (model != NULL) {
		s_model = *model;
	} else {
		classic_data_reset(&s_model);
	}

	classic_listener_reset_auth();
	s_initialized = true;
	return 0;
}

int classic_listener_shutdown(void)
{
	classic_data_reset(&s_model);
	classic_listener_reset_auth();
	s_initialized = false;
	return 0;
}

const nfc_service_t *classic_listener_get(void)
{
	return &s_classic_listener_service;
}
