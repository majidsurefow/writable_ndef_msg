/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NFC_PROTOCOLS_FELICA_POLLER_H_
#define NFC_PROTOCOLS_FELICA_POLLER_H_

#include "protocols/felica/felica.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

int felica_poller_detect(const nfc_reader_session_t *session);
int felica_poller_read(const nfc_reader_session_t *session, felica_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_FELICA_POLLER_H_ */
