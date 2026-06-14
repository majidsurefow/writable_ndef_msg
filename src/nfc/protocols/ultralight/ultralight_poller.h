/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ultralight poller — Flipper read-mode port (cookbook §5.2).
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_poller.c
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_POLLER_H_
#define NFC_PROTOCOLS_ULTRALIGHT_POLLER_H_

#include "ultralight.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

int ultralight_poller_detect(const nfc_reader_session_t *session);
int ultralight_poller_read(const nfc_reader_session_t *session, ultralight_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ULTRALIGHT_POLLER_H_ */
