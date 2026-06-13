/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "router/aid_router.h"

#include "../../stats.h"

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include "hal/nfc_transport.h"

LOG_MODULE_REGISTER(aid_router, CONFIG_LOG_DEFAULT_LEVEL);

typedef struct {
	uint8_t aid[NFC_AID_MAX_LEN];
	uint8_t aid_len;
	const nfc_service_t *svc;
} aid_router_entry_t;

static aid_router_config_t s_config;
static aid_router_stats_t s_stats;
static struct k_spinlock s_stats_lock;
static aid_router_state_t s_state = AID_ROUTER_STATE_UNINITIALIZED;

static aid_router_entry_t s_table[CONFIG_NFC_ROUTER_MAX_AIDS];
static uint8_t s_table_count;
static const nfc_service_t *s_active_svc;

static const uint8_t s_sw_file_not_found[2] = {
	NFC_SW1_FILE_NOT_FOUND,
	NFC_SW2_FILE_NOT_FOUND,
};

static const uint8_t s_sw_no_ef[2] = {
	NFC_SW1_NO_EF,
	NFC_SW2_NO_EF,
};

static bool apdu_is_select_by_aid(const nfc_apdu_t *apdu)
{
	return (apdu->cla == NFC_CLA_ISO7816) && (apdu->ins == NFC_INS_SELECT) &&
	       (apdu->p1 == NFC_SELECT_P1_BY_AID) && (apdu->lc > 0U);
}

static const aid_router_entry_t *find_aid(const uint8_t *aid, uint8_t aid_len)
{
	for (uint8_t i = 0U; i < s_table_count; i++) {
		if ((s_table[i].aid_len == aid_len) &&
		    (memcmp(s_table[i].aid, aid, (size_t)aid_len) == 0)) {
			return &s_table[i];
		}
	}

	return NULL;
}

static void aid_router_send_sw(const uint8_t *sw, size_t len)
{
	int ret;

	ret = nfc_transport_send_response(sw, len);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
	}
}

int aid_router_init(void)
{
	if (s_state == AID_ROUTER_STATE_INITIALIZED) {
		return -EALREADY;
	}

	STATS_RESET(s_stats);
	s_config.max_aids = (uint8_t)CONFIG_NFC_ROUTER_MAX_AIDS;
	s_table_count = 0U;
	s_active_svc = NULL;
	s_state = AID_ROUTER_STATE_INITIALIZED;
	return 0;
}

int aid_router_shutdown(void)
{
	s_table_count = 0U;
	s_active_svc = NULL;
	s_state = AID_ROUTER_STATE_UNINITIALIZED;
	return 0;
}

int aid_router_register(const uint8_t *aid, size_t aid_len, const nfc_service_t *svc)
{
	if (s_state != AID_ROUTER_STATE_INITIALIZED) {
		return -ENODEV;
	}

	if ((aid == NULL) || (svc == NULL) || (aid_len == 0U) || (aid_len > NFC_AID_MAX_LEN)) {
		return -EINVAL;
	}

	if (s_table_count >= s_config.max_aids) {
		STATS_INC(&s_stats_lock, s_stats, register_table_full_count);
		return -ENOMEM;
	}

	(void)memcpy(s_table[s_table_count].aid, aid, aid_len);
	s_table[s_table_count].aid_len = (uint8_t)aid_len;
	s_table[s_table_count].svc = svc;
	s_table_count++;
	return 0;
}

void aid_router_clear(void)
{
	s_table_count = 0U;
	s_active_svc = NULL;
}

void aid_router_field_off(void)
{
	if ((s_active_svc != NULL) && (s_active_svc->on_field_off != NULL)) {
		s_active_svc->on_field_off(s_active_svc->user_ctx);
	}

	s_active_svc = NULL;
	STATS_INC(&s_stats_lock, s_stats, field_off_count);
}

void aid_router_dispatch(const nfc_apdu_t *apdu)
{
	const aid_router_entry_t *match;

	if (apdu == NULL) {
		STATS_ERROR(&s_stats_lock, s_stats, -EINVAL);
		return;
	}

	if (s_state != AID_ROUTER_STATE_INITIALIZED) {
		STATS_ERROR(&s_stats_lock, s_stats, -ENODEV);
		return;
	}

	if (apdu_is_select_by_aid(apdu)) {
		if ((apdu->data == NULL) || (apdu->lc == 0U)) {
			aid_router_send_sw(s_sw_file_not_found, sizeof(s_sw_file_not_found));
			STATS_INC(&s_stats_lock, s_stats, select_unmatched_count);
			return;
		}

		match = find_aid(apdu->data, apdu->lc);
		if (match == NULL) {
			aid_router_send_sw(s_sw_file_not_found, sizeof(s_sw_file_not_found));
			STATS_INC(&s_stats_lock, s_stats, select_unmatched_count);
			return;
		}

		if ((s_active_svc != NULL) && (s_active_svc != match->svc) &&
		    (s_active_svc->on_deselect != NULL)) {
			s_active_svc->on_deselect(s_active_svc->user_ctx);
		}

		s_active_svc = match->svc;
		if (s_active_svc->on_select != NULL) {
			s_active_svc->on_select(apdu->data, apdu->lc, s_active_svc->user_ctx);
		}

		STATS_INC(&s_stats_lock, s_stats, select_matched_count);
		return;
	}

	if (s_active_svc == NULL) {
		aid_router_send_sw(s_sw_no_ef, sizeof(s_sw_no_ef));
		STATS_INC(&s_stats_lock, s_stats, apdu_no_service_count);
		return;
	}

	if (s_active_svc->on_apdu != NULL) {
		s_active_svc->on_apdu(apdu, s_active_svc->user_ctx);
	}

	STATS_INC(&s_stats_lock, s_stats, apdu_routed_count);
}

const aid_router_config_t *aid_router_get_config(void)
{
	return &s_config;
}

int aid_router_get_stats(aid_router_stats_t *out)
{
	return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

aid_router_state_t aid_router_get_state(void)
{
	return s_state;
}
