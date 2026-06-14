/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NFC_PROTOCOLS_EMV_POLLER_H_
#define NFC_PROTOCOLS_EMV_POLLER_H_

#include "emv.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

int emv_poller_detect(const nfc_reader_session_t *session);
int emv_poller_read(const nfc_reader_session_t *session, emv_card_image_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_EMV_POLLER_H_ */
