/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic_poller_i.c
 */

#include "protocols/classic/classic_poller.h"

#include "protocols/classic/classic_i.h"
#include "protocols/classic/crypto1.h"
#include "protocols/classic/iso14443_crc.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define CLASSIC_POLLER_TIMEOUT K_MSEC(5000)
#define CLASSIC_AUTH_NR_SIZE     4U
#define CLASSIC_AUTH_AT_SIZE     4U
#define CLASSIC_NT_SIZE          4U

#if defined(CONFIG_NFC_CLASSIC_TEST_DETERMINISTIC_NR)
static const uint8_t s_test_nr[CLASSIC_AUTH_NR_SIZE] = {0x01U, 0x02U, 0x03U, 0x04U};
#endif

static int classic_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static int classic_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx,
				     size_t tx_len, uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     CLASSIC_POLLER_TIMEOUT);
}

static int classic_poller_get_nt(nfc_reader_session_t *session, uint8_t block_num,
				 uint8_t nt[CLASSIC_NT_SIZE])
{
	uint8_t tx[2] = {CLASSIC_CMD_AUTH_KEY_A, block_num};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = classic_poller_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len != CLASSIC_NT_SIZE) {
		return -EIO;
	}

	(void)memcpy(nt, rx, CLASSIC_NT_SIZE);
	return 0;
}

static int classic_poller_auth(nfc_reader_session_t *session, struct crypto1 *crypto,
			       const classic_data_t *data, uint8_t block_num, uint64_t key)
{
	uint8_t nt[CLASSIC_NT_SIZE];
	uint8_t nr[CLASSIC_AUTH_NR_SIZE];
	uint8_t tx_auth[8];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	uint32_t cuid;
	int ret;

	ret = classic_poller_get_nt(session, block_num, nt);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_NFC_CLASSIC_TEST_DETERMINISTIC_NR)
	(void)memcpy(nr, s_test_nr, sizeof(nr));
#else
	for (size_t i = 0U; i < sizeof(nr); i++) {
		nr[i] = (uint8_t)(k_cycle_get_32() & 0xFFU);
	}
#endif

	cuid = classic_cuid_from_uid(data->uid, data->uid_len);
	crypto1_encrypt_reader_nonce(crypto, key, cuid, nt, nr, tx_auth);

	ret = classic_poller_transceive(session, tx_auth, sizeof(tx_auth), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len != CLASSIC_AUTH_AT_SIZE) {
		return -EIO;
	}

	{
		uint32_t nt_num = ((uint32_t)nt[0] << 24) | ((uint32_t)nt[1] << 16) |
				  ((uint32_t)nt[2] << 8) | (uint32_t)nt[3];

		crypto1_init(crypto, key);
		crypto1_word(crypto, nt_num ^ cuid, 0);
		for (size_t i = 0U; i < sizeof(nr); i++) {
			(void)crypto1_byte(crypto, nr[i], 0);
		}
		nt_num = crypto1_prng_successor(nt_num, 32U);
		for (size_t i = 0U; i < 4U; i++) {
			nt_num = crypto1_prng_successor(nt_num, 8U);
			(void)crypto1_byte(crypto, 0U, 0);
		}
		crypto1_word(crypto, 0U, 0);
	}

	return 0;
}

static int classic_poller_read_block(nfc_reader_session_t *session, struct crypto1 *crypto,
				     uint8_t block_num, uint8_t out[CLASSIC_BLOCK_SIZE])
{
	uint8_t tx_plain[4];
	size_t tx_len = 2U;
	uint8_t tx_enc[8];
	uint8_t rx_enc[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	uint8_t rx_plain[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t plain_len;
	int ret;

	tx_plain[0] = CLASSIC_CMD_READ_BLOCK;
	tx_plain[1] = block_num;
	iso14443_crc_append_a(tx_plain, &tx_len);
	crypto1_encrypt(crypto, NULL, tx_plain, tx_len, tx_enc);

	ret = classic_poller_transceive(session, tx_enc, tx_len, rx_enc, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < (CLASSIC_BLOCK_SIZE + ISO14443_CRC_SIZE)) {
		return -EIO;
	}

	crypto1_decrypt(crypto, rx_enc, rx_len, rx_plain);
	plain_len = rx_len;
	if (!iso14443_crc_check_a(rx_plain, plain_len)) {
		return -EIO;
	}

	iso14443_crc_trim_a(rx_plain, &plain_len);
	if (plain_len != CLASSIC_BLOCK_SIZE) {
		return -EIO;
	}

	(void)memcpy(out, rx_plain, CLASSIC_BLOCK_SIZE);
	return 0;
}

static classic_type_t classic_poller_detect_type(nfc_reader_session_t *session)
{
	uint8_t nt[CLASSIC_NT_SIZE];

	if (classic_poller_get_nt(session, 254U, nt) == 0) {
		return CLASSIC_TYPE_4K;
	}
	if (classic_poller_get_nt(session, 62U, nt) == 0) {
		return CLASSIC_TYPE_1K;
	}

	return CLASSIC_TYPE_MINI;
}

int classic_poller_detect(const nfc_reader_session_t *session)
{
	uint8_t nt[CLASSIC_NT_SIZE];
	int ret;

	ret = classic_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	ret = classic_poller_get_nt((nfc_reader_session_t *)session, 0U, nt);
	if (ret != 0) {
		return -ENOTSUP;
	}

	return 0;
}

int classic_poller_read(const nfc_reader_session_t *session, classic_data_t *out)
{
	nfc_reader_session_t *sess = (nfc_reader_session_t *)session;
	struct crypto1 crypto;
	uint8_t sectors;
	uint16_t blocks_total;
	uint8_t block_buf[CLASSIC_BLOCK_SIZE];
	int ret;

	ret = classic_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	if (out == NULL) {
		return -EINVAL;
	}

	classic_data_reset(out);
	if (sess->tag.valid && (sess->tag.uid.len > 0U)) {
		out->uid_len = sess->tag.uid.len;
		if (out->uid_len > sizeof(out->uid)) {
			out->uid_len = sizeof(out->uid);
		}
		(void)memcpy(out->uid, sess->tag.uid.bytes, out->uid_len);
	}

	out->type = classic_poller_detect_type(sess);
	if (out->type == CLASSIC_TYPE_UNKNOWN) {
		return -ENOTSUP;
	}

	sectors = classic_get_total_sectors(out->type);
	blocks_total = classic_get_total_blocks(out->type);

	for (uint8_t sector = 0U; sector < sectors; sector++) {
		uint8_t first_block = classic_get_first_block_of_sector(sector);
		uint8_t block_count = classic_get_blocks_in_sector(sector);
		bool authed = false;

		crypto1_reset(&crypto);
		ret = classic_poller_auth(sess, &crypto, out, first_block, CLASSIC_DEFAULT_KEY);
		if (ret != 0) {
			continue;
		}

		classic_set_key_found(out, sector, false, CLASSIC_DEFAULT_KEY);
		authed = true;

		for (uint8_t i = 0U; i < block_count; i++) {
			uint8_t block_num = (uint8_t)(first_block + i);

			if (block_num >= blocks_total) {
				break;
			}

			ret = classic_poller_read_block(sess, &crypto, block_num, block_buf);
			if (ret != 0) {
				if (authed) {
					return -EIO;
				}
				break;
			}

			classic_set_block_read(out, block_num, block_buf);

			if ((block_num == 0U) && (block_buf[0] != 0U)) {
				out->uid_len = (block_buf[0] == 0x88U) ? 7U : 4U;
				if (out->uid_len > sizeof(out->uid)) {
					out->uid_len = 4U;
				}
				(void)memcpy(out->uid, block_buf, out->uid_len);
				out->sak = block_buf[out->uid_len + 1U];
				out->atqa[0] = block_buf[out->uid_len + 2U];
				out->atqa[1] = block_buf[out->uid_len + 3U];
			}

			if (classic_is_sector_trailer(block_num)) {
				classic_set_key_found(out, sector, true, CLASSIC_DEFAULT_KEY);
			}
		}
	}

	if (!classic_is_block_read(out, 0U)) {
		return -EIO;
	}

	return 0;
}
