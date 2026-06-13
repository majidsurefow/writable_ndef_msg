/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_virtual_loopback.h"

#include "nfc_session_mock.h"
#include "nfc_virtual_rf.h"
#include "store/nfc_store.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/crc.h>

static uint8_t s_blob[NFC_VIRTUAL_LOOPBACK_MAX_BLOB];
static size_t s_blob_len;
static nfc_reader_session_t s_session;

static int mock_save_cb(const char *tag, const uint8_t *blob, size_t len, void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(user_ctx);

	if (len > sizeof(s_blob)) {
		return -ENOSPC;
	}

	(void)memcpy(s_blob, blob, len);
	s_blob_len = len;
	return 0;
}

static int mock_load_cb(const char *tag, uint8_t *out, size_t max, size_t *out_len,
			void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(user_ctx);

	if (s_blob_len == 0U) {
		return -ENOENT;
	}
	if (s_blob_len > max) {
		return -ENOSPC;
	}

	(void)memcpy(out, s_blob, s_blob_len);
	*out_len = s_blob_len;
	return 0;
}

static int verify_envelope(const uint8_t *blob, size_t len)
{
	uint16_t crc_stored;
	uint16_t crc_calc;

	if (len < NFC_STORE_ENVELOPE_OVERHEAD) {
		return -EBADMSG;
	}

	if ((blob[0] != NFC_STORE_BLOB_MAGIC_0) || (blob[1] != NFC_STORE_BLOB_MAGIC_1) ||
	    (blob[2] != NFC_STORE_BLOB_VERSION)) {
		return -EBADMSG;
	}

	crc_stored = (uint16_t)blob[len - 2U] | ((uint16_t)blob[len - 1U] << 8U);
	crc_calc = crc16_ccitt(0xFFFFU, blob, len - NFC_STORE_BLOB_CRC_SIZE);
	if (crc_calc != crc_stored) {
		return -EBADMSG;
	}

	return 0;
}

static int seed_golden(const uint8_t *blob, size_t len)
{
	if ((blob == NULL) || (len == 0U)) {
		return -EINVAL;
	}
	if (len > sizeof(s_blob)) {
		return -ENOSPC;
	}

	(void)memcpy(s_blob, blob, len);
	s_blob_len = len;
	return 0;
}

int nfc_virtual_loopback_init(void)
{
	int ret;

	ret = nfc_store_init(NULL);
	if (ret != 0) {
		return ret;
	}

	ret = nfc_store_register_save_cb(mock_save_cb, NULL);
	if (ret != 0) {
		return ret;
	}

	return nfc_store_register_load_cb(mock_load_cb, NULL);
}

void nfc_virtual_loopback_reset(void)
{
	nfc_session_mock_reset();
	nfc_virtual_rf_disable();
	s_blob_len = 0U;
	s_session.active = false;
}

int nfc_virtual_loopback_last_saved(const uint8_t **blob, size_t *len)
{
	if ((blob == NULL) || (len == NULL)) {
		return -EINVAL;
	}

	if (s_blob_len == 0U) {
		return -ENOENT;
	}

	*blob = s_blob;
	*len = s_blob_len;
	return 0;
}

int nfc_virtual_loopback_run(const nfc_virtual_loopback_params_t *params)
{
	const nfc_service_t *load_svcs[1];
	const nfc_service_t *save_svcs[1];
	int ret;

	if ((params == NULL) || (params->listener_svc == NULL) || (params->save_svc == NULL) ||
	    (params->golden_blob == NULL) || (params->golden_blob_len == 0U) ||
	    (params->golden_slot == NULL) || (params->output_slot == NULL) ||
	    (params->poller_detect == NULL) || (params->poller_read == NULL) ||
	    (params->read_out == NULL) || (params->save_model == NULL) ||
	    (params->copy_read_to_save == NULL)) {
		return -EINVAL;
	}

	if (params->listener_setup != NULL) {
		ret = params->listener_setup(params->listener_user_ctx);
		if (ret != 0) {
			return ret;
		}
	}

	ret = seed_golden(params->golden_blob, params->golden_blob_len);
	if (ret != 0) {
		goto out_teardown;
	}

	load_svcs[0] = params->listener_svc;
	ret = nfc_store_load(params->golden_slot, load_svcs, ARRAY_SIZE(load_svcs));
	if (ret != 0) {
		goto out_teardown;
	}

	ret = nfc_virtual_rf_enable(params->listener_svc);
	if (ret != 0) {
		goto out_teardown;
	}

	s_session.active = true;

	ret = params->poller_detect(&s_session);
	if (ret != 0) {
		goto out_disable;
	}

	ret = params->poller_read(&s_session, params->read_out);
	if (ret != 0) {
		goto out_disable;
	}

	ret = params->copy_read_to_save(params->save_model, params->read_out,
					params->copy_user_ctx);
	if (ret != 0) {
		goto out_disable;
	}

	save_svcs[0] = params->save_svc;
	ret = nfc_store_save(params->output_slot, save_svcs, ARRAY_SIZE(save_svcs));
	if (ret != 0) {
		goto out_disable;
	}

	if (params->compare != NULL) {
		ret = params->compare(params->expected, params->read_out,
				      params->compare_user_ctx);
		if (ret != 0) {
			goto out_disable;
		}
	}

	if (params->verify_envelope) {
		ret = verify_envelope(s_blob, s_blob_len);
		if (ret != 0) {
			goto out_disable;
		}
	}

out_disable:
	nfc_virtual_rf_disable();
out_teardown:
	if (params->listener_teardown != NULL) {
		params->listener_teardown(params->listener_user_ctx);
	}

	return ret;
}
