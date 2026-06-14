/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "desfire_listener.h"

#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

typedef enum {
	DESFIRE_PENDING_NONE = 0,
	DESFIRE_PENDING_GET_VERSION,
	DESFIRE_PENDING_READ_DATA,
} desfire_pending_cmd_t;

static desfire_data_t s_model;
static bool s_initialized;
static desfire_auth_state_t s_auth_state;
static int8_t s_selected_app;
static desfire_pending_cmd_t s_pending_cmd;
static uint8_t s_pending_frame;
static uint8_t s_pending_file_id;
static uint16_t s_pending_offset;
static uint8_t s_version_buf[DESFIRE_VERSION_TOTAL_BYTES];
static uint8_t s_resp_buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];

static void desfire_listener_session_reset(void)
{
	s_auth_state = DESFIRE_AUTH_STATE_IDLE;
	s_selected_app = -1;
	s_pending_cmd = DESFIRE_PENDING_NONE;
	s_pending_frame = 0U;
}

static void desfire_listener_send_status(uint8_t sw2)
{
	s_resp_buf[0] = DESFIRE_SW1;
	s_resp_buf[1] = sw2;
	(void)nfc_transport_send_response(s_resp_buf, 2U);
}

static void desfire_listener_send_data_status(const uint8_t *data, size_t data_len, uint8_t sw2)
{
	if (data_len > (NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
		desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
		return;
	}

	if ((data_len > 0U) && (data != NULL)) {
		(void)memcpy(s_resp_buf, data, data_len);
	}

	s_resp_buf[data_len] = DESFIRE_SW1;
	s_resp_buf[data_len + 1U] = sw2;
	(void)nfc_transport_send_response(s_resp_buf, data_len + 2U);
}

static const desfire_application_t *desfire_listener_selected_app(void)
{
	if ((s_selected_app < 0) ||
	    (s_selected_app >= (int8_t)s_model.app_count)) {
		return NULL;
	}

	return &s_model.apps[s_selected_app];
}

static int desfire_listener_find_file(const desfire_application_t *app, uint8_t file_id,
				      uint8_t *idx_out)
{
	for (uint8_t i = 0U; i < app->file_count; i++) {
		if (app->file_ids[i] == file_id) {
			*idx_out = i;
			return 0;
		}
	}

	return -ENOENT;
}

static bool desfire_listener_has_master_key(void)
{
	for (uint8_t i = 0U; i < DESFIRE_AES_KEY_SIZE; i++) {
		if (s_model.master_key[i] != 0U) {
			return true;
		}
	}

	return false;
}

static void desfire_listener_cmd_get_version(void)
{
	uint8_t chunk_len;
	uint8_t sw2;

	if (s_pending_cmd != DESFIRE_PENDING_GET_VERSION) {
		desfire_build_version_bytes(&s_model, s_version_buf);
		s_pending_cmd = DESFIRE_PENDING_GET_VERSION;
		s_pending_frame = 0U;
	}

	switch (s_pending_frame) {
	case 0U:
		chunk_len = 7U;
		sw2 = DESFIRE_STATUS_ADDITIONAL_FRAME;
		break;
	case 1U:
		chunk_len = 7U;
		sw2 = DESFIRE_STATUS_ADDITIONAL_FRAME;
		break;
	default:
		chunk_len = (uint8_t)(DESFIRE_VERSION_TOTAL_BYTES - 14U);
		sw2 = DESFIRE_STATUS_OK;
		break;
	}

	desfire_listener_send_data_status(&s_version_buf[s_pending_frame * 7U], chunk_len, sw2);

	if (sw2 == DESFIRE_STATUS_OK) {
		s_pending_cmd = DESFIRE_PENDING_NONE;
	} else {
		s_pending_frame++;
	}
}

static void desfire_listener_cmd_get_free_memory(void)
{
	uint8_t buf[4];

	buf[0] = (uint8_t)(s_model.free_memory & 0xFFU);
	buf[1] = (uint8_t)((s_model.free_memory >> 8U) & 0xFFU);
	buf[2] = (uint8_t)((s_model.free_memory >> 16U) & 0xFFU);
	buf[3] = (uint8_t)((s_model.free_memory >> 24U) & 0xFFU);
	desfire_listener_send_data_status(buf, sizeof(buf), DESFIRE_STATUS_OK);
}

static void desfire_listener_cmd_get_key_settings(void)
{
	const desfire_application_t *app = desfire_listener_selected_app();
	uint8_t buf[2];

	if (app != NULL) {
		buf[0] = app->key_settings;
		buf[1] = app->key_count;
	} else {
		buf[0] = s_model.master_key_settings;
		buf[1] = 0x0FU;
	}

	desfire_listener_send_data_status(buf, sizeof(buf), DESFIRE_STATUS_OK);
}

static void desfire_listener_cmd_get_application_ids(void)
{
	size_t len = 0U;

	for (uint8_t i = 0U; i < s_model.app_count; i++) {
		if ((len + DESFIRE_APP_ID_SIZE) > (NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
			desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
			return;
		}

		(void)memcpy(&s_resp_buf[len], s_model.apps[i].app_id, DESFIRE_APP_ID_SIZE);
		len += DESFIRE_APP_ID_SIZE;
	}

	desfire_listener_send_data_status(s_resp_buf, len, DESFIRE_STATUS_OK);
}

static void desfire_listener_cmd_get_file_ids(void)
{
	const desfire_application_t *app = desfire_listener_selected_app();
	size_t len = 0U;

	if (app == NULL) {
		desfire_listener_send_status(DESFIRE_STATUS_APP_NOT_FOUND);
		return;
	}

	for (uint8_t i = 0U; i < app->file_count; i++) {
		if ((len + 1U) > (NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
			desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
			return;
		}

		s_resp_buf[len++] = app->file_ids[i];
	}

	desfire_listener_send_data_status(s_resp_buf, len, DESFIRE_STATUS_OK);
}

static void desfire_listener_cmd_get_file_settings(const nfc_apdu_t *apdu)
{
	const desfire_application_t *app = desfire_listener_selected_app();
	uint8_t file_idx;
	uint8_t buf[8];
	const desfire_file_settings_t *fs;

	if (app == NULL) {
		desfire_listener_send_status(DESFIRE_STATUS_APP_NOT_FOUND);
		return;
	}

	if ((apdu->lc < 1U) || (apdu->data == NULL)) {
		desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
		return;
	}

	if (desfire_listener_find_file(app, apdu->data[0], &file_idx) != 0) {
		desfire_listener_send_status(DESFIRE_STATUS_FILE_NOT_FOUND);
		return;
	}

	fs = &app->file_settings[file_idx];
	(void)memset(buf, 0, sizeof(buf));
	buf[0] = (uint8_t)fs->type;
	buf[1] = (uint8_t)fs->comm;
	buf[2] = (uint8_t)(fs->access_rights & 0xFFU);
	buf[3] = (uint8_t)((fs->access_rights >> 8U) & 0xFFU);

	switch (fs->type) {
	case DESFIRE_FILE_TYPE_STANDARD:
	case DESFIRE_FILE_TYPE_BACKUP:
		buf[4] = (uint8_t)(fs->settings.data.size & 0xFFU);
		buf[5] = (uint8_t)((fs->settings.data.size >> 8U) & 0xFFU);
		buf[6] = (uint8_t)((fs->settings.data.size >> 16U) & 0xFFU);
		break;
	default:
		desfire_listener_send_status(DESFIRE_STATUS_ILLEGAL_CMD);
		return;
	}

	desfire_listener_send_data_status(buf, sizeof(buf), DESFIRE_STATUS_OK);
}

static void desfire_listener_cmd_get_key_version(const nfc_apdu_t *apdu)
{
	if ((apdu->lc < 1U) || (apdu->data == NULL)) {
		desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
		return;
	}

	if (apdu->data[0] != 0U) {
		desfire_listener_send_status(DESFIRE_STATUS_NO_SUCH_KEY);
		return;
	}

	desfire_listener_send_data_status((const uint8_t[]){0x00U}, 1U, DESFIRE_STATUS_OK);
}

static void desfire_listener_cmd_select_application(const nfc_apdu_t *apdu)
{
	if ((apdu->lc < DESFIRE_APP_ID_SIZE) || (apdu->data == NULL)) {
		desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
		return;
	}

	desfire_listener_session_reset();

	for (uint8_t i = 0U; i < s_model.app_count; i++) {
		if (memcmp(apdu->data, s_model.apps[i].app_id, DESFIRE_APP_ID_SIZE) == 0) {
			s_selected_app = (int8_t)i;
			desfire_listener_send_status(DESFIRE_STATUS_OK);
			return;
		}
	}

	desfire_listener_send_status(DESFIRE_STATUS_APP_NOT_FOUND);
}

static void desfire_listener_cmd_read_data(const nfc_apdu_t *apdu)
{
	const desfire_application_t *app = desfire_listener_selected_app();
	uint8_t file_idx;
	uint32_t offset;
	uint32_t length;
	size_t chunk;
	uint8_t sw2;

	if (app == NULL) {
		desfire_listener_send_status(DESFIRE_STATUS_PERMISSION_DENIED);
		return;
	}

	if ((apdu->lc < 7U) || (apdu->data == NULL)) {
		desfire_listener_send_status(DESFIRE_STATUS_LENGTH_ERROR);
		return;
	}

	if (desfire_listener_find_file(app, apdu->data[0], &file_idx) != 0) {
		desfire_listener_send_status(DESFIRE_STATUS_FILE_NOT_FOUND);
		return;
	}

	if (app->file_settings[file_idx].comm != DESFIRE_COMM_PLAIN) {
		if (s_auth_state != DESFIRE_AUTH_STATE_AUTHENTICATED) {
			desfire_listener_send_status(DESFIRE_STATUS_AUTH_ERROR);
			return;
		}
	}

	offset = (uint32_t)apdu->data[1] | ((uint32_t)apdu->data[2] << 8U) |
		 ((uint32_t)apdu->data[3] << 16U);
	length = (uint32_t)apdu->data[4] | ((uint32_t)apdu->data[5] << 8U) |
		 ((uint32_t)apdu->data[6] << 16U);

	if ((offset + length) > app->file_data_len[file_idx]) {
		desfire_listener_send_status(DESFIRE_STATUS_BOUNDARY_ERROR);
		return;
	}

	if (s_pending_cmd == DESFIRE_PENDING_READ_DATA) {
		offset = s_pending_offset;
		length = app->file_data_len[file_idx] - offset;
	} else {
		s_pending_file_id = apdu->data[0];
		s_pending_offset = (uint16_t)offset;
	}

	chunk = length;
	if (chunk > (NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
		chunk = NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U;
		sw2 = DESFIRE_STATUS_ADDITIONAL_FRAME;
		s_pending_cmd = DESFIRE_PENDING_READ_DATA;
		s_pending_offset = (uint16_t)(offset + chunk);
	} else {
		sw2 = DESFIRE_STATUS_OK;
		s_pending_cmd = DESFIRE_PENDING_NONE;
	}

	desfire_listener_send_data_status(&app->file_data[file_idx][offset], chunk, sw2);
}

static void desfire_listener_cmd_authenticate(void)
{
	if (!desfire_listener_has_master_key()) {
		desfire_listener_send_status(DESFIRE_STATUS_AUTH_ERROR);
		return;
	}

	s_auth_state = DESFIRE_AUTH_STATE_STEP1;
	desfire_listener_send_data_status((const uint8_t[]){0xAAU, 0xBBU, 0xCCU, 0xDDU}, 4U,
					  DESFIRE_STATUS_ADDITIONAL_FRAME);
}

static void desfire_listener_on_select(const uint8_t *aid, size_t aid_len, void *user_ctx)
{
	ARG_UNUSED(aid);
	ARG_UNUSED(aid_len);
	ARG_UNUSED(user_ctx);

	desfire_listener_session_reset();
	s_resp_buf[0] = NFC_SW1_OK;
	s_resp_buf[1] = NFC_SW2_OK;
	(void)nfc_transport_send_response(s_resp_buf, 2U);
}

static void desfire_listener_on_apdu(const nfc_apdu_t *apdu, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if ((apdu == NULL) || !s_initialized) {
		desfire_listener_send_status(DESFIRE_STATUS_ILLEGAL_CMD);
		return;
	}

	if (apdu->cla != DESFIRE_CLA_NATIVE) {
		desfire_listener_send_status(DESFIRE_STATUS_ILLEGAL_CMD);
		return;
	}

	switch (apdu->ins) {
	case DESFIRE_CMD_GET_VERSION:
		desfire_listener_cmd_get_version();
		break;
	case DESFIRE_CMD_GET_FREE_MEMORY:
		desfire_listener_cmd_get_free_memory();
		break;
	case DESFIRE_CMD_GET_KEY_SETTINGS:
		desfire_listener_cmd_get_key_settings();
		break;
	case DESFIRE_CMD_GET_APPLICATION_IDS:
		desfire_listener_cmd_get_application_ids();
		break;
	case DESFIRE_CMD_GET_FILE_IDS:
		desfire_listener_cmd_get_file_ids();
		break;
	case DESFIRE_CMD_GET_FILE_SETTINGS:
		desfire_listener_cmd_get_file_settings(apdu);
		break;
	case DESFIRE_CMD_GET_KEY_VERSION:
		desfire_listener_cmd_get_key_version(apdu);
		break;
	case DESFIRE_CMD_SELECT_APPLICATION:
		desfire_listener_cmd_select_application(apdu);
		break;
	case DESFIRE_CMD_READ_DATA:
		desfire_listener_cmd_read_data(apdu);
		break;
	case DESFIRE_CMD_AUTHENTICATE_AES:
	case DESFIRE_CMD_AUTH_EV2_FIRST:
		desfire_listener_cmd_authenticate();
		break;
	case DESFIRE_CMD_ADDITIONAL_FRAME:
		if (s_pending_cmd == DESFIRE_PENDING_GET_VERSION) {
			desfire_listener_cmd_get_version();
		} else if (s_pending_cmd == DESFIRE_PENDING_READ_DATA) {
			desfire_listener_cmd_read_data(apdu);
		} else {
			desfire_listener_send_status(DESFIRE_STATUS_ILLEGAL_CMD);
		}
		break;
	default:
		desfire_listener_send_status(DESFIRE_STATUS_ILLEGAL_CMD);
		break;
	}
}

static void desfire_listener_on_deselect(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	desfire_listener_session_reset();
}

static void desfire_listener_on_field_off(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	desfire_listener_session_reset();
}

static int desfire_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return desfire_serialize(&s_model, out, out_max, out_len);
}

static int desfire_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	desfire_listener_session_reset();
	return desfire_deserialize(&s_model, in, in_len);
}

static nfc_service_t s_service = {
	.on_select = desfire_listener_on_select,
	.on_apdu = desfire_listener_on_apdu,
	.on_deselect = desfire_listener_on_deselect,
	.on_field_off = desfire_listener_on_field_off,
	.serialize = desfire_listener_serialize,
	.deserialize = desfire_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_DESFIRE,
};

int desfire_listener_init(const desfire_data_t *model)
{
	if (model != NULL) {
		s_model = *model;
	} else {
		desfire_data_reset(&s_model);
#ifndef CONFIG_NFC_DESFIRE_FREE_MEMORY
		s_model.free_memory = 7520U;
#else
		s_model.free_memory = (uint32_t)CONFIG_NFC_DESFIRE_FREE_MEMORY;
#endif
	}

	desfire_listener_session_reset();
	s_initialized = true;
	return 0;
}

int desfire_listener_shutdown(void)
{
	desfire_listener_session_reset();
	s_initialized = false;
	return 0;
}

const nfc_service_t *desfire_listener_get(void)
{
	return &s_service;
}

desfire_auth_state_t desfire_listener_auth_state(void)
{
	return s_auth_state;
}
