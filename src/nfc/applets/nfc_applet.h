/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC product applets — compatibility umbrella header.
 *
 * The headless Layer-1 API now lives in nfc_applet_service.h (LOCKED 2026-06-14,
 * Phase A). This header used to declare shell-typed prototypes
 * (nfc_applet_scan_print / nfc_applet_loop(struct shell *, ...)); those were
 * removed when scan rendering and loop orchestration were split into the shell
 * adapter. It is kept as a thin include so existing `#include
 * "applets/nfc_applet.h"` users keep compiling.
 */

#ifndef NFC_APPLETS_NFC_APPLET_H_
#define NFC_APPLETS_NFC_APPLET_H_

#include "applets/nfc_applet_service.h"

#endif /* NFC_APPLETS_NFC_APPLET_H_ */
