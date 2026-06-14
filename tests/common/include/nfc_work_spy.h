/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_COMMON_NFC_WORK_SPY_H_
#define TESTS_COMMON_NFC_WORK_SPY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void nfc_work_spy_reset(void);
size_t nfc_work_spy_submit_count(void);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_WORK_SPY_H_ */
