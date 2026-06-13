/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "framing/apdu_assembler.h"

#include "../../stats.h"

#include <errno.h>
#include <string.h>

#include <zephyr/net_buf.h>
#include <zephyr/logging/log.h>

#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"
#include "router/aid_router.h"

LOG_MODULE_REGISTER(apdu_assembler, CONFIG_LOG_DEFAULT_LEVEL);

typedef enum {
	APDU_PARSE_OK = 0,
	APDU_PARSE_REJECT_TOO_SHORT,
	APDU_PARSE_REJECT_LENGTH_MISMATCH,
	APDU_PARSE_REJECT_EXTENDED_DISABLED,
} apdu_parse_result_t;

static apdu_assembler_config_t s_config = {
	.extended_apdu_support = IS_ENABLED(CONFIG_NFC_APDU_EXTENDED_SUPPORT),
};
static apdu_assembler_stats_t s_stats;
static struct k_spinlock s_stats_lock;
static apdu_assembler_state_t s_state = APDU_ASSEMBLER_STATE_UNINITIALIZED;

static const uint8_t s_sw_reject_buf[2] = {
	NFC_SW1_WRONG_LENGTH,
	NFC_SW2_WRONG_LENGTH,
};

static void apdu_assembler_on_field_on_handler(void *user_ctx);
static void apdu_assembler_on_field_off_handler(void *user_ctx);
static void apdu_assembler_on_apdu_handler(struct net_buf *apdu, void *user_ctx);

static const nfc_transport_ops_t s_ops_impl = {
	.on_field_on = apdu_assembler_on_field_on_handler,
	.on_field_off = apdu_assembler_on_field_off_handler,
	.on_apdu = apdu_assembler_on_apdu_handler,
};

static apdu_parse_result_t apdu_parse(const uint8_t *data, size_t len, bool extended_ok,
				      nfc_apdu_t *out)
{
	uint8_t lc_s;

	if ((data == NULL) || (out == NULL)) {
		return APDU_PARSE_REJECT_TOO_SHORT;
	}

	if (len < 4U) {
		return APDU_PARSE_REJECT_TOO_SHORT;
	}

	out->cla = data[0];
	out->ins = data[1];
	out->p1 = data[2];
	out->p2 = data[3];
	out->data = NULL;
	out->lc = 0U;
	out->le = 0U;
	out->has_le = false;
	out->extended = false;

	if (len == 4U) {
		return APDU_PARSE_OK;
	}

	if (len == 5U) {
		out->has_le = true;
		out->le = data[4];
		return APDU_PARSE_OK;
	}

	if (data[4] != 0U) {
		lc_s = data[4];

		if (len == (size_t)(5U + lc_s)) {
			out->lc = lc_s;
			out->data = &data[5];
			return APDU_PARSE_OK;
		}

		if (len == (size_t)(6U + lc_s)) {
			out->lc = lc_s;
			out->data = &data[5];
			out->has_le = true;
			out->le = data[5U + lc_s];
			return APDU_PARSE_OK;
		}

		return APDU_PARSE_REJECT_LENGTH_MISMATCH;
	}

	if (!extended_ok) {
		return APDU_PARSE_REJECT_EXTENDED_DISABLED;
	}

	if (len == 7U) {
		out->extended = true;
		out->has_le = true;
		out->le = (uint16_t)(((uint16_t)data[5] << 8U) | (uint16_t)data[6]);
		return APDU_PARSE_OK;
	}

	return APDU_PARSE_REJECT_LENGTH_MISMATCH;
}

static void apdu_assembler_on_field_on_handler(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	STATS_INC(&s_stats_lock, s_stats, field_on_count);
}

static void apdu_assembler_on_field_off_handler(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	aid_router_field_off();
	STATS_INC(&s_stats_lock, s_stats, field_off_count);
}

static void apdu_assembler_on_apdu_handler(struct net_buf *apdu, void *user_ctx)
{
	nfc_apdu_t parsed = { 0 };
	apdu_parse_result_t result;

	ARG_UNUSED(user_ctx);

	if (apdu == NULL) {
		return;
	}

	if (s_state != APDU_ASSEMBLER_STATE_INITIALIZED) {
		net_buf_unref(apdu);
		return;
	}

	STATS_INC(&s_stats_lock, s_stats, apdu_rx_count);

	result = apdu_parse(apdu->data, (size_t)apdu->len, s_config.extended_apdu_support, &parsed);
	if (result != APDU_PARSE_OK) {
		(void)nfc_transport_send_response(s_sw_reject_buf, sizeof(s_sw_reject_buf));
		STATS_INC(&s_stats_lock, s_stats, apdu_parse_reject_count);
		net_buf_unref(apdu);
		return;
	}

	STATS_INC(&s_stats_lock, s_stats, apdu_parse_ok_count);
	aid_router_dispatch(&parsed);
	net_buf_unref(apdu);
}

int apdu_assembler_init(const apdu_assembler_config_t *cfg)
{
	if (s_state == APDU_ASSEMBLER_STATE_INITIALIZED) {
		return -EALREADY;
	}

	STATS_RESET(s_stats);
	if (cfg != NULL) {
		s_config = *cfg;
	} else {
		s_config.extended_apdu_support = IS_ENABLED(CONFIG_NFC_APDU_EXTENDED_SUPPORT);
	}

	s_state = APDU_ASSEMBLER_STATE_INITIALIZED;
	return 0;
}

int apdu_assembler_shutdown(void)
{
	s_state = APDU_ASSEMBLER_STATE_UNINITIALIZED;
	return 0;
}

const nfc_transport_ops_t *apdu_assembler_get_ops(void)
{
	return &s_ops_impl;
}

const apdu_assembler_config_t *apdu_assembler_get_config(void)
{
	return &s_config;
}

int apdu_assembler_get_stats(apdu_assembler_stats_t *out)
{
	return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

apdu_assembler_state_t apdu_assembler_get_state(void)
{
	return s_state;
}
