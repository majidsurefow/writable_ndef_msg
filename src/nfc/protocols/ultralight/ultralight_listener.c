/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * T4 NDEF adapter — deserialize pages → ndef_listener (cookbook §5.2).
 */

#include "ultralight_listener.h"

#include "ndef_listener.h"
#include "protocols/ndef/ndef.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

static ultralight_data_t s_model;
static bool s_initialized;

static int ultralight_listener_bind_ndef(void)
{
	const uint8_t *msg = NULL;
	size_t msg_len = 0U;
	ndef_data_t ndef_model;
	int ret;

	ndef_data_reset(&ndef_model);
	ret = ultralight_extract_ndef(&s_model, &msg, &msg_len);
	if (ret != 0) {
		return ret;
	}

	if (msg != NULL) {
		if (msg_len > CONFIG_NFC_NDEF_MAX_SIZE) {
			return -EBADMSG;
		}

		ndef_model.ndef_file[0] = (uint8_t)(msg_len >> 8U);
		ndef_model.ndef_file[1] = (uint8_t)(msg_len & 0xFFU);
		(void)memcpy(&ndef_model.ndef_file[2], msg, msg_len);
		ndef_model.ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + msg_len);
	}

	return ndef_listener_init(&ndef_model, &(ndef_listener_config_t){ .writable = false });
}

static void ultralight_listener_on_select(const uint8_t *aid, size_t aid_len, void *user_ctx)
{
	const nfc_service_t *ndef_svc = ndef_listener_get();

	ARG_UNUSED(user_ctx);

	if ((ndef_svc != NULL) && (ndef_svc->on_select != NULL)) {
		ndef_svc->on_select(aid, aid_len, ndef_svc->user_ctx);
	}
}

static void ultralight_listener_on_apdu(const nfc_apdu_t *apdu, void *user_ctx)
{
	const nfc_service_t *ndef_svc = ndef_listener_get();

	ARG_UNUSED(user_ctx);

	if ((ndef_svc != NULL) && (ndef_svc->on_apdu != NULL)) {
		ndef_svc->on_apdu(apdu, ndef_svc->user_ctx);
	}
}

static void ultralight_listener_on_deselect(void *user_ctx)
{
	const nfc_service_t *ndef_svc = ndef_listener_get();

	ARG_UNUSED(user_ctx);

	if ((ndef_svc != NULL) && (ndef_svc->on_deselect != NULL)) {
		ndef_svc->on_deselect(ndef_svc->user_ctx);
	}
}

static void ultralight_listener_on_field_off(void *user_ctx)
{
	const nfc_service_t *ndef_svc = ndef_listener_get();

	ARG_UNUSED(user_ctx);

	if ((ndef_svc != NULL) && (ndef_svc->on_field_off != NULL)) {
		ndef_svc->on_field_off(ndef_svc->user_ctx);
	}
}

static int ultralight_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len,
					 void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return ultralight_serialize(&s_model, out, out_max, out_len);
}

static int ultralight_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	int ret;

	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	(void)ndef_listener_shutdown();

	ret = ultralight_deserialize(&s_model, in, in_len);
	if (ret != 0) {
		return ret;
	}

	return ultralight_listener_bind_ndef();
}

static const nfc_service_t s_ultralight_listener_service = {
	.on_select = ultralight_listener_on_select,
	.on_apdu = ultralight_listener_on_apdu,
	.on_deselect = ultralight_listener_on_deselect,
	.on_field_off = ultralight_listener_on_field_off,
	.serialize = ultralight_listener_serialize,
	.deserialize = ultralight_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_ULTRALIGHT,
	.user_ctx = NULL,
};

int ultralight_listener_init(void)
{
	if (s_initialized) {
		return -EALREADY;
	}

	ultralight_data_reset(&s_model);
	s_initialized = true;
	return 0;
}

int ultralight_listener_load(const ultralight_data_t *model)
{
	if (!s_initialized) {
		return -ENODEV;
	}

	if (model == NULL) {
		return -EINVAL;
	}

	(void)ndef_listener_shutdown();
	s_model = *model;
	return ultralight_listener_bind_ndef();
}

int ultralight_listener_shutdown(void)
{
	if (!s_initialized) {
		return 0;
	}

	(void)ndef_listener_shutdown();
	ultralight_data_reset(&s_model);
	s_initialized = false;
	return 0;
}

const nfc_service_t *ultralight_listener_get(void)
{
	return &s_ultralight_listener_service;
}
