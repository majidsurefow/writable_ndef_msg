/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual FeliCa listener for Tier C / E+ loopback (Flipper felica_listener.c).
 */

#ifndef NFC_PROTOCOLS_FELICA_LISTENER_H_
#define NFC_PROTOCOLS_FELICA_LISTENER_H_

#include "protocols/felica/felica.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int felica_listener_init(const felica_data_t *model);
int felica_listener_shutdown(void);
const nfc_service_t *felica_listener_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_FELICA_LISTENER_H_ */
