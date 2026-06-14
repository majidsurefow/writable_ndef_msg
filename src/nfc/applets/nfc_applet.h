/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC product applets — Gate 4 orchestration API.
 */

#ifndef NFC_APPLETS_NFC_APPLET_H_
#define NFC_APPLETS_NFC_APPLET_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "hal/nfc_transport.h"
#include "protocols/ndef/ndef.h"

/* nfc_profile_t is needed by the emulate API in all builds; nfc_applet_emulate()
 * is always defined (returns -ENOTSUP when CONFIG_NFC_LISTEN_STACK is off).
 */
#include "nfc_stack/nfc_stack.h"

struct shell;

#ifdef __cplusplus
extern "C" {
#endif

int nfc_applet_scan_start(k_timeout_t timeout);
bool nfc_applet_scan_busy(void);
int nfc_applet_scan_wait(k_timeout_t deadline);
void nfc_applet_scan_print(const struct shell *sh);

int nfc_applet_read_start(const char *slot, k_timeout_t timeout);
bool nfc_applet_read_busy(void);
int nfc_applet_read_wait(k_timeout_t deadline);

int nfc_applet_emulate(const char *slot, nfc_profile_t profile);

int nfc_applet_verify_start(const char *slot, k_timeout_t timeout);
bool nfc_applet_verify_busy(void);
int nfc_applet_verify_wait(k_timeout_t deadline);
/** @return 0 on PASS, negative errno on FAIL or error. */
int nfc_applet_verify_last_result(void);

int nfc_applet_loop(const struct shell *sh, const char *slot, k_timeout_t timeout);

/**
 * @brief Compare stored vs polled NDEF models (Tier E + verify core).
 *
 * UID pointers may be NULL to skip UID check (store envelope has no UID v1).
 */
int nfc_applet_verify_compare(const ndef_data_t *expected, const ndef_data_t *actual,
			      const nfc_uid_t *expected_uid, const nfc_uid_t *actual_uid);

#ifdef __cplusplus
}
#endif

#endif /* NFC_APPLETS_NFC_APPLET_H_ */
