/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual SLIX listener for Tier C / E+ loopback.
 * Behavioral reference: flipperzero/lib/nfc/protocols/slix/slix_listener.c
 */

#ifndef NFC_PROTOCOLS_SLIX_LISTENER_H_
#define NFC_PROTOCOLS_SLIX_LISTENER_H_

#include "protocols/slix/slix.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int slix_listener_init(const slix_data_t *model);
int slix_listener_shutdown(void);
const nfc_service_t *slix_listener_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_SLIX_LISTENER_H_ */
