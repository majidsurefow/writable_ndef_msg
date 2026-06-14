/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Central compile-time NFC stack configuration.
 *
 * All symbols derive from Kconfig (autoconf.h) via IS_ENABLED(). Use these
 * macros instead of scattering CONFIG_NFC_* checks when expressing
 * backend/role policy. Runtime capability bits remain in nfc_transport_caps_t.
 *
 * Overlay matrix (repo root):
 *   overlay-pn7160-stack.conf   — PN7160 reader (Gate 2)
 *   overlay-pn7160-listen.conf  — adds listen + NFC_LISTEN_STACK (Gate 4)
 *   overlay-nfct-stack.conf     — NRFX listen-only (Gate 5)
 */

#ifndef NFC_CONFIG_H_
#define NFC_CONFIG_H_

#include <zephyr/sys/util.h>

/** HAL backend (mutually exclusive NFC_HAL_BACKEND choice in hal/Kconfig). */
#define NFC_HAL_IS_PN7160 IS_ENABLED(CONFIG_NFC_HAL_BACKEND_PN7160)
#define NFC_HAL_IS_NRFX   IS_ENABLED(CONFIG_NFC_HAL_BACKEND_NRFX)

/** Role symbols (hal/Kconfig); backend BUILD_ASSERTs enforce caps ⊆ roles. */
#define NFC_HAS_READER IS_ENABLED(CONFIG_NFC_ROLE_READER)
#define NFC_HAS_LISTEN IS_ENABLED(CONFIG_NFC_ROLE_LISTEN)

/** Orchestrator modules (reader/Kconfig, nfc_stack/Kconfig). */
#define NFC_HAS_READER_STACK IS_ENABLED(CONFIG_NFC_READER)
#define NFC_HAS_LISTEN_STACK IS_ENABLED(CONFIG_NFC_LISTEN_STACK)

#endif /* NFC_CONFIG_H_ */
