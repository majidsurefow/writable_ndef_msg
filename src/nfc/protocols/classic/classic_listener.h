/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual MIFARE Classic listener for Tier C / E+ loopback (Flipper mf_classic_listener.c).
 */

#ifndef NFC_PROTOCOLS_CLASSIC_LISTENER_H_
#define NFC_PROTOCOLS_CLASSIC_LISTENER_H_

#include "protocols/classic/classic.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int classic_listener_init(const classic_data_t *model);
int classic_listener_shutdown(void);
const nfc_service_t *classic_listener_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_LISTENER_H_ */
