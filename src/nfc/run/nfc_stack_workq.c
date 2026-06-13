/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "run/nfc_stack_workq.h"

#include <zephyr/kernel.h>

static K_THREAD_STACK_DEFINE(nfc_stack_wq_stack, CONFIG_NFC_STACK_WORKQ_STACK_SIZE);
static struct k_work_q nfc_stack_wq;
static bool s_wq_started;

struct k_work_q *nfc_stack_wq_get(void)
{
	if (!s_wq_started) {
		k_work_queue_init(&nfc_stack_wq);
		k_work_queue_start(&nfc_stack_wq, nfc_stack_wq_stack,
				   K_THREAD_STACK_SIZEOF(nfc_stack_wq_stack),
				   CONFIG_NFC_STACK_WORKQ_PRIORITY, NULL);
		s_wq_started = true;
	}

	return &nfc_stack_wq;
}
