/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "store/nfc_store_ram.h"

#if IS_ENABLED(CONFIG_NFC_STORE_RAM)

#include "store/nfc_store.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

typedef struct {
	char tag[CONFIG_NFC_STORE_MAX_TAG_LEN];
	uint8_t blob[CONFIG_NFC_STORE_BLOB_SIZE];
	size_t len;
	bool occupied;
} nfc_store_ram_slot_t;

static nfc_store_ram_slot_t s_slots[CONFIG_NFC_STORE_RAM_SLOT_COUNT];
static struct k_spinlock s_lock;
static bool s_ready;

static int nfc_store_ram_find_tag(const char *tag)
{
	for (size_t i = 0U; i < ARRAY_SIZE(s_slots); i++) {
		if (s_slots[i].occupied && (strcmp(s_slots[i].tag, tag) == 0)) {
			return (int)i;
		}
	}

	return -1;
}

static int nfc_store_ram_find_free(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(s_slots); i++) {
		if (!s_slots[i].occupied) {
			return (int)i;
		}
	}

	return -1;
}

void nfc_store_ram_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&s_lock);

	for (size_t i = 0U; i < ARRAY_SIZE(s_slots); i++) {
		s_slots[i].occupied = false;
		s_slots[i].len = 0U;
		s_slots[i].tag[0] = '\0';
	}

	k_spin_unlock(&s_lock, key);
}

int nfc_store_ram_save_cb(const char *tag, const uint8_t *blob, size_t len, void *user_ctx)
{
	k_spinlock_key_t key;
	int slot_idx;
	size_t tag_len;

	ARG_UNUSED(user_ctx);

	if ((tag == NULL) || (blob == NULL)) {
		return -EINVAL;
	}

	tag_len = strlen(tag);
	if (tag_len == 0U || tag_len >= (size_t)CONFIG_NFC_STORE_MAX_TAG_LEN) {
		return -EINVAL;
	}

	if (len > (size_t)CONFIG_NFC_STORE_BLOB_SIZE) {
		return -ENOSPC;
	}

	key = k_spin_lock(&s_lock);
	slot_idx = nfc_store_ram_find_tag(tag);
	if (slot_idx < 0) {
		slot_idx = nfc_store_ram_find_free();
	}
	if (slot_idx < 0) {
		k_spin_unlock(&s_lock, key);
		return -ENOSPC;
	}

	(void)strncpy(s_slots[slot_idx].tag, tag, sizeof(s_slots[slot_idx].tag) - 1U);
	s_slots[slot_idx].tag[sizeof(s_slots[slot_idx].tag) - 1U] = '\0';
	(void)memcpy(s_slots[slot_idx].blob, blob, len);
	s_slots[slot_idx].len = len;
	s_slots[slot_idx].occupied = true;

	k_spin_unlock(&s_lock, key);
	return 0;
}

int nfc_store_ram_load_cb(const char *tag, uint8_t *out, size_t max, size_t *out_len,
			  void *user_ctx)
{
	k_spinlock_key_t key;
	int slot_idx;
	size_t len;

	ARG_UNUSED(user_ctx);

	if ((tag == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	key = k_spin_lock(&s_lock);
	slot_idx = nfc_store_ram_find_tag(tag);
	if (slot_idx < 0) {
		k_spin_unlock(&s_lock, key);
		return -ENOENT;
	}

	len = s_slots[slot_idx].len;
	if (len > max) {
		k_spin_unlock(&s_lock, key);
		return -ENOSPC;
	}

	(void)memcpy(out, s_slots[slot_idx].blob, len);
	*out_len = len;

	k_spin_unlock(&s_lock, key);
	return 0;
}

int nfc_store_ram_init(void)
{
	int ret;

	if (s_ready) {
		return 0;
	}

	ret = nfc_store_init(NULL);
	if ((ret != 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = nfc_store_register_save_cb(nfc_store_ram_save_cb, NULL);
	if (ret != 0) {
		return ret;
	}

	ret = nfc_store_register_load_cb(nfc_store_ram_load_cb, NULL);
	if (ret != 0) {
		return ret;
	}

	s_ready = true;
	return 0;
}

#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)

static int cmd_nfc_store_ram_list(const struct shell *sh, size_t argc, char **argv)
{
	char tags[CONFIG_NFC_STORE_RAM_SLOT_COUNT][CONFIG_NFC_STORE_MAX_TAG_LEN];
	size_t lens[CONFIG_NFC_STORE_RAM_SLOT_COUNT];
	size_t count = 0U;
	k_spinlock_key_t key;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	key = k_spin_lock(&s_lock);
	for (size_t i = 0U; i < ARRAY_SIZE(s_slots); i++) {
		if (!s_slots[i].occupied) {
			continue;
		}

		(void)strncpy(tags[count], s_slots[i].tag, sizeof(tags[count]) - 1U);
		tags[count][sizeof(tags[count]) - 1U] = '\0';
		lens[count] = s_slots[i].len;
		count++;
	}
	k_spin_unlock(&s_lock, key);

	if (count == 0U) {
		shell_print(sh, "(empty)");
		return 0;
	}

	for (size_t i = 0U; i < count; i++) {
		uint8_t persist_id = 0U;
		uint8_t flags = 0U;
		int meta_ret = nfc_store_peek_entry_meta(tags[i], &persist_id, &flags);

		if (meta_ret == 0) {
			shell_print(sh, "[%zu] %s len=%zu persist_id=0x%02x flags=0x%02x", i,
				    tags[i], lens[i], persist_id, flags);
		} else {
			shell_print(sh, "[%zu] %s len=%zu (meta %d)", i, tags[i], lens[i],
				    meta_ret);
		}
	}

	return 0;
}

static int cmd_nfc_store_ram_dump(const struct shell *sh, size_t argc, char **argv)
{
	k_spinlock_key_t key;
	int slot_idx;
	size_t len;
	const uint8_t *blob;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc store dump <slot>");
		return -EINVAL;
	}

	key = k_spin_lock(&s_lock);
	slot_idx = nfc_store_ram_find_tag(argv[1]);
	if (slot_idx < 0) {
		k_spin_unlock(&s_lock, key);
		shell_error(sh, "Slot \"%s\" not found", argv[1]);
		return -ENOENT;
	}

	len = s_slots[slot_idx].len;
	blob = s_slots[slot_idx].blob;
	k_spin_unlock(&s_lock, key);

	shell_print(sh, "@@NFCDUMP@@ %s", argv[1]);
	for (size_t i = 0U; i < len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%02x", blob[i]);
	}
	shell_print(sh, "");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	nfc_store_ram_cmds,
	SHELL_CMD(list, NULL, "List occupied RAM store slots", cmd_nfc_store_ram_list),
	SHELL_CMD_ARG(dump, NULL, "Hex dump RAM slot blob: <slot>", cmd_nfc_store_ram_dump, 2, 0),
	SHELL_SUBCMD_SET_END);

#endif /* CONFIG_NFC_STORE_RAM_SHELL */

#else /* !CONFIG_NFC_STORE_RAM */

#include <errno.h>

int nfc_store_ram_save_cb(const char *tag, const uint8_t *blob, size_t len, void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(blob);
	ARG_UNUSED(len);
	ARG_UNUSED(user_ctx);

	return -ENOTSUP;
}

int nfc_store_ram_load_cb(const char *tag, uint8_t *out, size_t max, size_t *out_len,
			  void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(out);
	ARG_UNUSED(max);
	ARG_UNUSED(out_len);
	ARG_UNUSED(user_ctx);

	return -ENOTSUP;
}

int nfc_store_ram_init(void)
{
	return -ENOTSUP;
}

void nfc_store_ram_reset(void)
{
}

#endif /* CONFIG_NFC_STORE_RAM */
