/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ultralight T4 NDEF adapter listener (cookbook §5.2 emulate path).
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_LISTENER_H_
#define NFC_PROTOCOLS_ULTRALIGHT_LISTENER_H_

#include "ultralight.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int ultralight_listener_init(void);
int ultralight_listener_load(const ultralight_data_t *model);
int ultralight_listener_shutdown(void);
const nfc_service_t *ultralight_listener_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ULTRALIGHT_LISTENER_H_ */
