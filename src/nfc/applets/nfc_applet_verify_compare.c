/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "applets/nfc_applet.h"

#include <errno.h>
#include <string.h>

int nfc_applet_verify_compare(const ndef_data_t *expected, const ndef_data_t *actual,
			      const nfc_uid_t *expected_uid, const nfc_uid_t *actual_uid)
{
	if ((expected == NULL) || (actual == NULL)) {
		return -EINVAL;
	}

	if ((expected->cc_len != actual->cc_len) ||
	    (memcmp(expected->cc, actual->cc, expected->cc_len) != 0)) {
		return -EBADMSG;
	}

	if ((expected->ndef_file_len != actual->ndef_file_len) ||
	    (memcmp(expected->ndef_file, actual->ndef_file, expected->ndef_file_len) != 0)) {
		return -EBADMSG;
	}

	if ((expected_uid != NULL) && (actual_uid != NULL)) {
		if ((expected_uid->len != actual_uid->len) ||
		    (memcmp(expected_uid->bytes, actual_uid->bytes, expected_uid->len) != 0)) {
			return -EBADMSG;
		}
	}

	return 0;
}
