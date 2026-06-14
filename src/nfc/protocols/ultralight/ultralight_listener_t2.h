/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native Type-2 listener for virtual loopback (Flipper mf_ultralight_listener.c).
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_LISTENER_T2_H_
#define NFC_PROTOCOLS_ULTRALIGHT_LISTENER_T2_H_

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int ultralight_listener_t2_init(void);
int ultralight_listener_t2_shutdown(void);
const nfc_service_t *ultralight_listener_t2_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ULTRALIGHT_LISTENER_T2_H_ */
