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

#include "protocols/iso15693_3/iso15693_3.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLIX_CMD_GET_NXP_SYSINFO 0xABU
#define SLIX_CMD_READ_SIGNATURE  0xBDU

size_t slix_poller_prepare_request(const uint8_t uid[ISO15693_UID_SIZE], uint8_t command,
				   bool skip_uid, uint8_t *tx, size_t tx_max);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_SLIX_POLLER_I_H_ */
