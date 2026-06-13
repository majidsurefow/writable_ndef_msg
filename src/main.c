/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @defgroup nfc_writable_ndef_msg_example_main main.c
 * @{
 * @ingroup nfc_writable_ndef_msg_example
 * @brief NFC Type 4 tag emulation (custom UID, minimal in-RAM NDEF file).
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <stdbool.h>

#if IS_ENABLED(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

#if IS_ENABLED(CONFIG_NFC_STACK)
#else
#include "nfc_emulation.h"
#endif

static int board_init(void)
{
#if IS_ENABLED(CONFIG_DK_LIBRARY)
	return dk_leds_init();
#else
	return 0;
#endif
}

#if IS_ENABLED(CONFIG_NFC_STACK)

#include "nfc_stack/nfc_stack.h"

int main(void)
{
	int ret;

	printk("NFC stack (shell: nfc stack init|start|stop|load");
#if IS_ENABLED(CONFIG_NFC_READER_SHELL)
	printk(" | nfc reader scan|clone");
#endif
	printk(")\n");

	if (board_init() < 0) {
		return -EIO;
	}

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
	ret = nfc_stack_init(NULL);
	if ((ret != 0) && (ret != -EALREADY)) {
		printk("nfc_stack_init failed: %d\n", ret);
		return -EIO;
	}
#endif

	while (true) {
		k_sleep(K_FOREVER);
	}
}

#else /* legacy NFCT sample */

#define NFC_FIELD_LED	DK_LED1
#define NFC_READ_LED	DK_LED4
#define NFC_UID_COUNT 3
static const uint8_t nfc_uids[NFC_UID_COUNT][7] = {
	{ 0x04, 0x1F, 0xF5, 0x5C, 0xBD, 0x2A, 0x81 },
	{ 0x04, 0x91, 0xF7, 0x01, 0x8A, 0x04, 0x03 },
	{ 0x04, 0x2D, 0x6F, 0xFA, 0x11, 0x6F, 0x80 },
};

static size_t nfc_uid_slot;
static struct k_work nfc_field_off_work;

static void nfc_field_off_work_handler(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	nfc_uid_slot = (nfc_uid_slot + 1U) % NFC_UID_COUNT;

	nfc_uid_t uid = {
		.bytes = nfc_uids[nfc_uid_slot],
		.len = sizeof(nfc_uids[nfc_uid_slot]),
	};

	err = nfc_apply_uid_and_restart(&uid);
	if (err < 0) {
		printk("nfc_apply_uid_and_restart failed: %d\n", err);
	}
}

#define NDEF_BUF_SIZE 64
static uint8_t ndef_msg_buf[NDEF_BUF_SIZE] = {
	0x00, 0x03,
	0xD0, 0x00, 0x00,
};

static void nfc_callback(void *context, nfc_t4t_event_t event, const uint8_t *data,
			 size_t data_length, uint32_t flags)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);
	ARG_UNUSED(flags);

	switch (event) {
	case NFC_T4T_EVENT_FIELD_ON:
		dk_set_led_on(NFC_FIELD_LED);
		break;
	case NFC_T4T_EVENT_FIELD_OFF:
		dk_set_leds(DK_NO_LEDS_MSK);
		k_work_submit(&nfc_field_off_work);
		break;
	case NFC_T4T_EVENT_NDEF_READ:
		dk_set_led_on(NFC_READ_LED);
		break;
	default:
		break;
	}
}

int main(void)
{
	printk("NFC Type 4 emulation sample\n");

	if (board_init() < 0) {
		goto fail;
	}

	k_work_init(&nfc_field_off_work, nfc_field_off_work_handler);
	nfc_uid_slot = 0;

	nfc_uid_t uid = {
		.bytes = nfc_uids[nfc_uid_slot],
		.len = sizeof(nfc_uids[nfc_uid_slot]),
	};
	nfc_t4_start_args_t t4 = {
		.callback = nfc_callback,
		.callback_context = NULL,
		.ndef_file_buf = ndef_msg_buf,
		.ndef_file_buf_size = sizeof(ndef_msg_buf),
	};

	if (nfc_on(&uid, NFC_TAG_TYPE_4, &t4) < 0) {
		printk("Cannot turn NFC on\n");
		goto fail;
	}

	while (true) {
		k_cpu_atomic_idle(irq_lock());
	}

fail:
#if CONFIG_REBOOT
	sys_reboot(SYS_REBOOT_COLD);
#endif
	return -EIO;
}

#endif /* CONFIG_NFC_STACK */
/** @} */
