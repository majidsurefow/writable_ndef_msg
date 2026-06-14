/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private SLIX poller frame builders.
 * Behavioral reference: flipperzero/lib/nfc/protocols/slix/slix_poller_i.c
 */

#ifndef NFC_PROTOCOLS_SLIX_POLLER_I_H_
#define NFC_PROTOCOLS_SLIX_POLLER_I_H_

#include <stddef.h>
#include <stdint.h>

#include "reader/nfc_reader_engine.h"
#include "protocols/iso15693_3/iso15693_3.h"
#include "protocols/slix/slix.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLIX_CMD_GET_NXP_SYSINFO 0xABU
#define SLIX_CMD_READ_SIGNATURE  0xBDU
#define SLIX_CMD_GET_RANDOM      0xB2U
#define SLIX_CMD_SET_PASSWORD    0xB3U

typedef enum {
	SLIX_PASSWORD_READ = 0U,
	SLIX_PASSWORD_WRITE,
	SLIX_PASSWORD_PRIVACY,
	SLIX_PASSWORD_EAS_AFI,
	SLIX_PASSWORD_DESTROY,
} slix_password_type_t;

size_t slix_poller_prepare_request(const uint8_t uid[ISO15693_UID_SIZE], uint8_t command,
				   bool skip_uid, uint8_t *tx, size_t tx_max);
int slix_poller_get_random_number(nfc_reader_session_t *session, uint16_t *random_out);
int slix_poller_set_password(nfc_reader_session_t *session, const uint8_t uid[ISO15693_UID_SIZE],
			     slix_password_type_t type, const uint8_t password[SLIX_PASSWORD_SIZE],
			     uint16_t random);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_SLIX_POLLER_I_H_ */
