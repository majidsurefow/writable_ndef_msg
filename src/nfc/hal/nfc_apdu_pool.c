/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/nfc_apdu_pool.h"

#include <zephyr/net_buf.h>

NET_BUF_POOL_FIXED_DEFINE(nfc_apdu_pool, CONFIG_NFC_APDU_POOL_COUNT,
			  CONFIG_NFC_APDU_BUF_SIZE, 0, NULL);

struct net_buf_pool *nfc_apdu_pool_get(void)
{
	return &nfc_apdu_pool;
}
