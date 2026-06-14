/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual ISO15693-3 listener for Tier C / E+ loopback.
 * Behavioral reference: flipperzero/lib/nfc/protocols/iso15693_3/iso15693_3_listener.c
 */

#ifndef NFC_PROTOCOLS_ISO15693_3_LISTENER_H_
#define NFC_PROTOCOLS_ISO15693_3_LISTENER_H_

#include "protocols/iso15693_3/iso15693_3.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int iso15693_3_listener_init(const iso15693_3_data_t *model);
int iso15693_3_listener_shutdown(void);
const nfc_service_t *iso15693_3_listener_get(void);
const iso15693_3_data_t *iso15693_3_listener_model(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ISO15693_3_LISTENER_H_ */
