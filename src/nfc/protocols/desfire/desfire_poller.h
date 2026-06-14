/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire ISO-DEP poller — partial clone (cookbook §5.4).
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_desfire/mf_desfire_poller.c
 */

#ifndef NFC_PROTOCOLS_DESFIRE_POLLER_H_
#define NFC_PROTOCOLS_DESFIRE_POLLER_H_

#include "desfire.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

int desfire_poller_detect(const nfc_reader_session_t *session);
int desfire_poller_read(const nfc_reader_session_t *session, desfire_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_DESFIRE_POLLER_H_ */
