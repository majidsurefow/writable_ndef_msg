/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_work_spy.h"

#include "hal/nfc_transport.h"

#include <errno.h>

#include <zephyr/kernel.h>

static size_t s_submit_count;

void nfc_work_spy_reset(void)
{
	s_submit_count = 0U;
}

size_t nfc_work_spy_submit_count(void)
{
	return s_submit_count;
}

int nfc_transport_submit_work(struct k_work *work)
{
	if (work == NULL) {
		return -EINVAL;
	}

	s_submit_count++;
	if (work->handler != NULL) {
		work->handler(work);
	}

	return 0;
}
