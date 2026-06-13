/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC applet capability policy — HAL roles + store flags (Gate 4).
 */

#ifndef NFC_APPLETS_NFC_APPLET_POLICY_H_
#define NFC_APPLETS_NFC_APPLET_POLICY_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t nfc_applet_caps_t;

#define NFC_APPLET_CAP_READ    BIT(0)
#define NFC_APPLET_CAP_EMULATE BIT(1)
#define NFC_APPLET_CAP_VERIFY  BIT(2)

/**
 * @brief Derive applet capabilities for a stored card entry.
 *
 * Clone-only entries (no @c NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE) omit
 * @c NFC_APPLET_CAP_EMULATE. Flipper parity: MF Classic / FeliCa are read-only
 * dumps — emulate returns -ENOTSUP via nfc_applet_check_emulate().
 */
int nfc_applet_caps_for_card(uint8_t persist_id, uint8_t store_flags, nfc_applet_caps_t *caps);

/** @return 0 if emulate allowed, -ENOTSUP for clone-only cards or HAL without listen. */
int nfc_applet_check_emulate(uint8_t persist_id, uint8_t store_flags);

#ifdef __cplusplus
}
#endif

#endif /* NFC_APPLETS_NFC_APPLET_POLICY_H_ */
