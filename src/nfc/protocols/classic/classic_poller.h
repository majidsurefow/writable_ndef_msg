/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic_poller.h
 */

#ifndef NFC_PROTOCOLS_CLASSIC_POLLER_H_
#define NFC_PROTOCOLS_CLASSIC_POLLER_H_

#include "protocols/classic/classic.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

int classic_poller_detect(const nfc_reader_session_t *session);
int classic_poller_read(const nfc_reader_session_t *session, classic_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_POLLER_H_ */
