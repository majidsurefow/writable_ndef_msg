/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nfc_emulation.h"

#include <errno.h>
#include <string.h>

static bool uid_len_valid(size_t len)
{
	return len == 4U || len == 7U || len == 10U;
}

static uint8_t uid_store[10];

int nfc_on(const nfc_uid_t *uid, nfc_tag_type_t type, const nfc_t4_start_args_t *t4)
{
	int err;

	if (!uid || !uid->bytes || !uid_len_valid(uid->len)) {
		return -EINVAL;
	}

	if (type != NFC_TAG_TYPE_4) {
		return -ENOTSUP;
	}

	if (!t4 || !t4->callback || !t4->ndef_file_buf || t4->ndef_file_buf_size == 0) {
		return -EINVAL;
	}

	/* NLEN + smallest NDEF message (empty record). */
	if (t4->ndef_file_buf_size < 5U) {
		return -EINVAL;
	}

	memcpy(uid_store, uid->bytes, uid->len);

	err = nfc_t4t_setup(t4->callback, t4->callback_context);
	if (err < 0) {
		return err;
	}

	err = nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, uid_store, uid->len);
	if (err < 0) {
		return err;
	}

	err = nfc_t4t_ndef_rwpayload_set(t4->ndef_file_buf, t4->ndef_file_buf_size);
	if (err < 0) {
		return err;
	}

	return nfc_t4t_emulation_start();
}

int nfc_off(void)
{
	return nfc_t4t_emulation_stop();
}

int nfc_apply_uid_and_restart(const nfc_uid_t *uid)
{
	int err;

	if (!uid || !uid->bytes || !uid_len_valid(uid->len)) {
		return -EINVAL;
	}

	memcpy(uid_store, uid->bytes, uid->len);

	(void)nfc_t4t_emulation_stop();

	err = nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, uid_store, uid->len);
	if (err < 0) {
		(void)nfc_t4t_emulation_start();
		return err;
	}

	return nfc_t4t_emulation_start();
}
