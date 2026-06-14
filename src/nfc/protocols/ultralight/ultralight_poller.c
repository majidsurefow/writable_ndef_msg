/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_poller.c
 *                       flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_poller_i.c
 */

#include "ultralight_poller.h"
#include "ultralight_poller_i.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

static int ultralight_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static bool ultralight_version_is_zero(const uint8_t ver[ULTRALIGHT_VERSION_SIZE])
{
	for (size_t i = 0U; i < ULTRALIGHT_VERSION_SIZE; i++) {
		if (ver[i] != 0U) {
			return false;
		}
	}

	return true;
}

static ultralight_variant_t variant_from_type(ultralight_type_t type)
{
	switch (type) {
	case UL_TYPE_UL11:
		return UL_VARIANT_UL11;
	case UL_TYPE_UL21:
		return UL_VARIANT_UL21;
	case UL_TYPE_ORIGIN:
		return UL_VARIANT_ORIGIN;
	default:
		return UL_VARIANT_UNKNOWN;
	}
}

static ultralight_type_t ultralight_type_from_version(const uint8_t ver[ULTRALIGHT_VERSION_SIZE])
{
	if (ultralight_version_is_zero(ver)) {
		return UL_TYPE_UNKNOWN;
	}

	switch (ver[6]) {
	case 0x0BU:
		return UL_TYPE_UL11;
	case 0x0EU:
		return UL_TYPE_UL21;
	case 0x0FU:
		return UL_TYPE_NTAG213;
	case 0x11U:
		return UL_TYPE_NTAG215;
	case 0x13U:
		return UL_TYPE_NTAG216;
	default:
		return UL_TYPE_UNKNOWN;
	}
}

static int ultralight_poller_try_pwd_auth(nfc_reader_session_t *session,
					  const ultralight_config_t *cfg, bool *authenticated)
{
	static const uint8_t default_pwd[ULTRALIGHT_PAGE_SIZE] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
	int ret;

	if ((session == NULL) || (authenticated == NULL) || *authenticated) {
		return -EINVAL;
	}

	if ((cfg != NULL) && cfg->valid) {
		ret = ultralight_poller_i_pwd_auth(session, cfg->pwd);
		if (ret == 0) {
			*authenticated = true;
			return 0;
		}

		if (cfg->authlim == 0U) {
			ret = ultralight_poller_i_pwd_auth(session, default_pwd);
			if (ret == 0) {
				*authenticated = true;
				return 0;
			}
		}

		return ret;
	}

	return -ENOTSUP;
}

static int ultralight_poller_copy_read_chunk(ultralight_data_t *out, uint16_t start,
					     uint16_t pages_total,
					     const uint8_t page_buf[ULTRALIGHT_READ_RESP_LEN])
{
	for (uint16_t i = 0U; i < 4U; i++) {
		uint16_t page = (uint16_t)(start + i);

		if (page >= pages_total) {
			break;
		}

		(void)memcpy(out->pages[page], &page_buf[i * ULTRALIGHT_PAGE_SIZE],
			     ULTRALIGHT_PAGE_SIZE);
	}

	if ((start + 4U) < pages_total) {
		out->pages_read = (uint16_t)(start + 4U);
	} else {
		out->pages_read = pages_total;
	}

	return 0;
}

static int ultralight_poller_read_pages(nfc_reader_session_t *session, ultralight_data_t *out,
					uint16_t pages_total)
{
	uint8_t page_buf[ULTRALIGHT_READ_RESP_LEN];
	ultralight_config_t cfg;
	bool authenticated = false;
	int ret;

	out->pages_read = 0U;

	for (uint16_t start = 0U; start < pages_total; start += 4U) {
		uint16_t config_start;

		(void)memset(&cfg, 0, sizeof(cfg));
		(void)ultralight_parse_config(out, &cfg);
		ultralight_config_page_indices(pages_total, &config_start, NULL, NULL, NULL);

		if (ultralight_type_has_password_config(out->type) && cfg.valid && !authenticated &&
		    ultralight_read_protection_enabled(&cfg) && (start >= config_start)) {
			(void)ultralight_poller_try_pwd_auth(session, &cfg, &authenticated);
		}

		if (ultralight_page_needs_auth(&cfg, start) && !authenticated) {
			ret = ultralight_poller_try_pwd_auth(session, &cfg, &authenticated);
			if (ret != 0) {
				return 0;
			}
		}

		ret = ultralight_poller_i_read_page(session, (uint8_t)start, page_buf);
		if (ret != 0) {
			if (!authenticated && cfg.valid) {
				(void)ultralight_poller_try_pwd_auth(session, &cfg, &authenticated);
				if (authenticated) {
					ret = ultralight_poller_i_read_page(session, (uint8_t)start,
									    page_buf);
				}
			}

			if (ret != 0) {
				return 0;
			}
		}

		(void)ultralight_poller_copy_read_chunk(out, start, pages_total, page_buf);
	}

	return 0;
}

static int ultralight_poller_identify_type(nfc_reader_session_t *session, ultralight_type_t *type_out,
					   uint8_t version[ULTRALIGHT_VERSION_SIZE],
					   bool *has_version)
{
	uint8_t page_resp[ULTRALIGHT_READ_RESP_LEN];
	int ret;

	*has_version = false;
	*type_out = UL_TYPE_UNKNOWN;

	ret = ultralight_poller_i_get_version(session, version);
	if (ret == 0) {
		*has_version = !ultralight_version_is_zero(version);
		*type_out = ultralight_type_from_version(version);
		if (*type_out != UL_TYPE_UNKNOWN) {
			return 0;
		}
	}

	ret = ultralight_poller_i_authentication_test(session);
	if (ret == 0) {
		*type_out = UL_TYPE_MFUL_C;
		return 0;
	}

	ret = ultralight_poller_i_read_page(session, 41U, page_resp);
	if (ret != 0) {
		*type_out = UL_TYPE_ORIGIN;
		return 0;
	}

	*type_out = UL_TYPE_NTAG203;
	return 0;
}

static int ultralight_poller_auth_ultralight_c(nfc_reader_session_t *session,
					       ultralight_data_t *out, bool *authenticated)
{
	uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE];
	int ret;

	(void)memset(key, 0, sizeof(key));
	if (out->pages_read >= (uint16_t)(ULTRALIGHT_C_KEY_PAGE + 4U)) {
		(void)memcpy(key, out->pages[ULTRALIGHT_C_KEY_PAGE], sizeof(key));
	}

	ret = ultralight_poller_i_authenticate(session, key, authenticated);
	if ((ret == 0) && *authenticated) {
		(void)memcpy(out->pages[ULTRALIGHT_C_KEY_PAGE], key, sizeof(key));
	}

	return ret;
}

int ultralight_poller_detect(const nfc_reader_session_t *session)
{
	uint8_t rx[ULTRALIGHT_READ_RESP_LEN];
	size_t rx_len = 0U;
	uint8_t tx[2] = {ULTRALIGHT_CMD_READ, 0x00U};
	int ret;

	ret = ultralight_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	ret = ultralight_poller_i_transceive((nfc_reader_session_t *)session, tx, sizeof(tx), rx,
					     &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < ULTRALIGHT_READ_RESP_LEN) {
		return -ENOTSUP;
	}

	return 0;
}

int ultralight_poller_read(const nfc_reader_session_t *session, ultralight_data_t *out)
{
	nfc_reader_session_t *sess = (nfc_reader_session_t *)session;
	uint8_t version[ULTRALIGHT_VERSION_SIZE];
	bool has_version = false;
	bool authenticated = false;
	ultralight_type_t type;
	uint16_t pages_total;
	int ret;

	ret = ultralight_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	if (out == NULL) {
		return -EINVAL;
	}

	ultralight_data_reset(out);

	ret = ultralight_poller_identify_type(sess, &type, version, &has_version);
	if (ret != 0) {
		return ret;
	}

	pages_total = ultralight_pages_total_for_type(type);
	if (pages_total == 0U) {
		return -ENOTSUP;
	}

	out->type = type;
	out->variant = variant_from_type(type);
	out->pages_total = pages_total;

	if (has_version) {
		out->has_version = true;
		(void)memcpy(out->version, version, ULTRALIGHT_VERSION_SIZE);
	}

	if (type == UL_TYPE_MFUL_C) {
		(void)ultralight_poller_auth_ultralight_c(sess, out, &authenticated);
	}

	ret = ultralight_poller_read_pages(sess, out, pages_total);
	if (ret != 0) {
		return ret;
	}

	if (out->pages_read < pages_total) {
		return 0;
	}

	if (has_version) {
		ret = ultralight_poller_i_read_signature(sess, out->signature);
		if (ret == 0) {
			out->has_signature = true;
		}

		for (uint8_t i = 0U; i < ULTRALIGHT_COUNTER_NUM; i++) {
			ret = ultralight_poller_i_read_counter(sess, i, out->counters[i]);
			if (ret == 0) {
				out->counter_count = (uint8_t)(i + 1U);
			}
		}

		for (uint8_t i = 0U; i < ULTRALIGHT_TEARING_FLAG_NUM; i++) {
			ret = ultralight_poller_i_read_tearing(sess, i, &out->tearing_flags[i]);
			if (ret == 0) {
				out->tearing_flag_count = (uint8_t)(i + 1U);
			}
		}
	}

	return 0;
}
