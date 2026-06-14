/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "store/nfc_store.h"

#include "../../stats.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_NFC_ROLE_LISTEN)
#include "nfc_stack/nfc_stack.h"
#endif

LOG_MODULE_REGISTER(nfc_store, CONFIG_LOG_DEFAULT_LEVEL);

static nfc_store_config_t s_config = NFC_STORE_CONFIG_DEFAULT;
static nfc_store_stats_t s_stats;
static struct k_spinlock s_stats_lock;
static nfc_store_state_t s_state = NFC_STORE_STATE_UNINITIALIZED;

static uint8_t s_staging_buf[CONFIG_NFC_STORE_BLOB_SIZE];
static uint8_t s_serialize_buf[CONFIG_NFC_STORE_BLOB_SIZE];

static nfc_store_save_fn s_save_cb;
static void *s_save_user_ctx;
static nfc_store_load_fn s_load_cb;
static void *s_load_user_ctx;
static nfc_store_commit_fn s_commit_cb;
static void *s_commit_user_ctx;

static uint8_t s_resolved_max_tag_len;

/* Inert default: the store core never renders. A real backend (RAM) or the
 * shell adapter (@@NFCDUMP@@ hex dump) registers a save cb via
 * nfc_store_register_save_cb(). Layer-0 must contain no shell symbols.
 */
static int nfc_store_default_save(const char *tag, const uint8_t *blob, size_t len,
				  void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(blob);
	ARG_UNUSED(len);
	ARG_UNUSED(user_ctx);

	return -ENODEV;
}

static int nfc_store_default_load(const char *tag, uint8_t *out, size_t max, size_t *out_len,
				  void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(out);
	ARG_UNUSED(max);
	ARG_UNUSED(out_len);
	ARG_UNUSED(user_ctx);

	return -ENOENT;
}

static int nfc_store_default_commit(const nfc_service_t *svc, const char *tag, void *user_ctx)
{
	ARG_UNUSED(svc);
	ARG_UNUSED(tag);
	ARG_UNUSED(user_ctx);

	return 0;
}

static uint8_t nfc_store_flags_for_persist_id(uint8_t persist_id, bool reader_capture)
{
	static const uint8_t k_persist_hand_flags[NFC_PERSIST_ID_MAX + 1U] = {
		[NFC_PERSIST_ID_NDEF] = NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
		[NFC_PERSIST_ID_ULTRALIGHT] = NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
		[NFC_PERSIST_ID_EMV] = NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
		[NFC_PERSIST_ID_DESFIRE] = NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
		[NFC_PERSIST_ID_ALIRO] = NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
	};
	uint8_t flags = 0U;

	if (persist_id <= NFC_PERSIST_ID_MAX) {
		flags = k_persist_hand_flags[persist_id];
	}

	if (reader_capture) {
		flags |= NFC_STORE_ENTRY_FLAG_READER_CAPTURED;
	} else {
		flags |= NFC_STORE_ENTRY_FLAG_HAND_AUTHORED;
	}

	return flags;
}

static int nfc_store_check_quiescent(void)
{
#if IS_ENABLED(CONFIG_NFC_ROLE_LISTEN)
	if (nfc_stack_get_state() == NFC_STACK_STATE_STARTED) {
		return -EBUSY;
	}
#endif
	return 0;
}

static int nfc_store_validate_tag(const char *tag)
{
	size_t len;

	if (tag == NULL) {
		return -EINVAL;
	}

	len = strlen(tag);
	if (len == 0U || len >= (size_t)s_resolved_max_tag_len) {
		return -EINVAL;
	}

	return 0;
}

static int nfc_store_write_header(uint8_t *buf, size_t buf_max, size_t payload_len, size_t *pos)
{
	if (buf_max < NFC_STORE_BLOB_HDR_SIZE) {
		return -ENOSPC;
	}

	buf[0] = NFC_STORE_BLOB_MAGIC_0;
	buf[1] = NFC_STORE_BLOB_MAGIC_1;
	buf[2] = NFC_STORE_BLOB_VERSION;
	buf[3] = 0U;
	buf[4] = (uint8_t)(payload_len & 0xFFU);
	buf[5] = (uint8_t)((payload_len >> 8U) & 0xFFU);
	*pos = NFC_STORE_BLOB_HDR_SIZE;
	return 0;
}

static int nfc_store_append_crc(uint8_t *buf, size_t buf_max, size_t payload_end, size_t *total)
{
	uint16_t crc;

	if ((payload_end + NFC_STORE_BLOB_CRC_SIZE) > buf_max) {
		return -ENOSPC;
	}

	crc = crc16_ccitt(0xFFFFU, buf, payload_end);
	buf[payload_end] = (uint8_t)(crc & 0xFFU);
	buf[payload_end + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);
	*total = payload_end + NFC_STORE_BLOB_CRC_SIZE;
	return 0;
}

static const nfc_service_t *nfc_store_find_service(const nfc_service_t *const *svcs, size_t n,
						   uint8_t persist_id)
{
	for (size_t i = 0U; i < n; i++) {
		if ((svcs[i] != NULL) && (svcs[i]->persist_id == persist_id)) {
			return svcs[i];
		}
	}

	return NULL;
}

static int nfc_store_read_envelope(const char *tag, size_t *blob_len, uint16_t *payload_len,
				   uint8_t *version)
{
	uint16_t crc_stored;
	uint16_t crc_calc;
	int ret;

	if (s_load_cb == NULL) {
		return -ENODEV;
	}

	ret = s_load_cb(tag, s_staging_buf, sizeof(s_staging_buf), blob_len, s_load_user_ctx);
	if (ret == -ENOENT) {
		return -ENOENT;
	}
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -EIO;
	}

	if (*blob_len < NFC_STORE_ENVELOPE_OVERHEAD) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	if ((s_staging_buf[0] != NFC_STORE_BLOB_MAGIC_0) ||
	    (s_staging_buf[1] != NFC_STORE_BLOB_MAGIC_1)) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	if ((s_staging_buf[2] != NFC_STORE_BLOB_VERSION) &&
	    (s_staging_buf[2] != NFC_STORE_BLOB_VERSION_V1)) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	*version = s_staging_buf[2];
	*payload_len = (uint16_t)s_staging_buf[4] | ((uint16_t)s_staging_buf[5] << 8U);
	if ((size_t)(NFC_STORE_BLOB_HDR_SIZE + *payload_len + NFC_STORE_BLOB_CRC_SIZE) != *blob_len) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	crc_stored = (uint16_t)s_staging_buf[NFC_STORE_BLOB_HDR_SIZE + *payload_len] |
		     ((uint16_t)s_staging_buf[NFC_STORE_BLOB_HDR_SIZE + *payload_len + 1U] << 8U);
	crc_calc = crc16_ccitt(0xFFFFU, s_staging_buf, NFC_STORE_BLOB_HDR_SIZE + *payload_len);
	if (crc_calc != crc_stored) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	return 0;
}

static int nfc_store_validate_blob_buf(const uint8_t *blob, size_t len, uint16_t *payload_len,
				       uint8_t *version)
{
	uint16_t crc_stored;
	uint16_t crc_calc;

	if (blob == NULL) {
		return -EINVAL;
	}

	if (len < NFC_STORE_ENVELOPE_OVERHEAD) {
		return -EBADMSG;
	}

	if ((blob[0] != NFC_STORE_BLOB_MAGIC_0) || (blob[1] != NFC_STORE_BLOB_MAGIC_1)) {
		return -EBADMSG;
	}

	if ((blob[2] != NFC_STORE_BLOB_VERSION) && (blob[2] != NFC_STORE_BLOB_VERSION_V1)) {
		return -EBADMSG;
	}

	*version = blob[2];
	*payload_len = (uint16_t)blob[4] | ((uint16_t)blob[5] << 8U);
	if ((size_t)(NFC_STORE_BLOB_HDR_SIZE + *payload_len + NFC_STORE_BLOB_CRC_SIZE) != len) {
		return -EBADMSG;
	}

	crc_stored = (uint16_t)blob[NFC_STORE_BLOB_HDR_SIZE + *payload_len] |
		     ((uint16_t)blob[NFC_STORE_BLOB_HDR_SIZE + *payload_len + 1U] << 8U);
	crc_calc = crc16_ccitt(0xFFFFU, blob, NFC_STORE_BLOB_HDR_SIZE + *payload_len);
	if (crc_calc != crc_stored) {
		return -EBADMSG;
	}

	return 0;
}

int nfc_store_validate_blob(const uint8_t *blob, size_t len)
{
	uint16_t payload_len;
	uint8_t version;

	return nfc_store_validate_blob_buf(blob, len, &payload_len, &version);
}

int nfc_store_import_blob(const char *tag, const uint8_t *blob, size_t len)
{
	uint16_t payload_len;
	uint8_t version;
	int ret;

	if ((tag == NULL) || (blob == NULL)) {
		return -EINVAL;
	}

	ret = nfc_store_validate_blob_buf(blob, len, &payload_len, &version);
	if (ret != 0) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return ret;
	}

	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	ret = nfc_store_validate_tag(tag);
	if (ret != 0) {
		return ret;
	}

	if (s_save_cb == NULL) {
		return -ENODEV;
	}

	ret = s_save_cb(tag, blob, len, s_save_user_ctx);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -EIO;
	}

	return 0;
}

int nfc_store_init(const nfc_store_config_t *cfg)
{
	if (s_state == NFC_STORE_STATE_INITIALIZED) {
		return -EALREADY;
	}

	STATS_RESET(s_stats);
	s_config = (cfg != NULL) ? *cfg : NFC_STORE_CONFIG_DEFAULT;
	s_resolved_max_tag_len = (s_config.max_tag_len == 0U) ?
					 (uint8_t)CONFIG_NFC_STORE_MAX_TAG_LEN :
					 s_config.max_tag_len;
	s_save_cb = nfc_store_default_save;
	s_save_user_ctx = NULL;
	s_load_cb = nfc_store_default_load;
	s_load_user_ctx = NULL;
	s_commit_cb = nfc_store_default_commit;
	s_commit_user_ctx = NULL;
	s_state = NFC_STORE_STATE_INITIALIZED;
	return 0;
}

int nfc_store_shutdown(void)
{
	s_save_cb = nfc_store_default_save;
	s_save_user_ctx = NULL;
	s_load_cb = nfc_store_default_load;
	s_load_user_ctx = NULL;
	s_commit_cb = nfc_store_default_commit;
	s_commit_user_ctx = NULL;
	s_state = NFC_STORE_STATE_UNINITIALIZED;
	return 0;
}

int nfc_store_register_save_cb(nfc_store_save_fn fn, void *user_ctx)
{
	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	s_save_cb = (fn != NULL) ? fn : nfc_store_default_save;
	s_save_user_ctx = user_ctx;
	return 0;
}

int nfc_store_register_load_cb(nfc_store_load_fn fn, void *user_ctx)
{
	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	s_load_cb = (fn != NULL) ? fn : nfc_store_default_load;
	s_load_user_ctx = user_ctx;
	return 0;
}

int nfc_store_register_commit_cb(nfc_store_commit_fn fn, void *user_ctx)
{
	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	s_commit_cb = (fn != NULL) ? fn : nfc_store_default_commit;
	s_commit_user_ctx = user_ctx;
	return 0;
}

int nfc_store_on_dirty(const nfc_service_t *svc, const char *tag)
{
	size_t body_len = 0U;
	int ret;

	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	if ((svc == NULL) || (tag == NULL)) {
		return -EINVAL;
	}

	if (svc->persist_id == 0U) {
		return -EINVAL;
	}

	if (svc->serialize == NULL) {
		return -EINVAL;
	}

	ret = svc->serialize(s_staging_buf, sizeof(s_staging_buf), &body_len, svc->user_ctx);
	if (ret == -ENOTSUP) {
		return 0;
	}
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -EIO;
	}

	if (s_commit_cb == NULL) {
		return -ENODEV;
	}

	ret = s_commit_cb(svc, tag, s_commit_user_ctx);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	STATS_INC(&s_stats_lock, s_stats, live_commit_count);
	return 0;
}

int nfc_store_save(const char *tag, const nfc_service_t *const *svcs, size_t n)
{
	size_t pos = 0U;
	size_t payload_len = 0U;
	size_t total = 0U;
	int ret;

	ret = nfc_store_check_quiescent();
	if (ret != 0) {
		return ret;
	}

	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	ret = nfc_store_validate_tag(tag);
	if (ret != 0) {
		return ret;
	}

	if ((n > 0U) && (svcs == NULL)) {
		return -EINVAL;
	}

	ret = nfc_store_write_header(s_staging_buf, sizeof(s_staging_buf), 0U, &pos);
	if (ret != 0) {
		return ret;
	}

	pos = NFC_STORE_BLOB_HDR_SIZE;
	payload_len = 0U;

	for (size_t i = 0U; i < n; i++) {
		const nfc_service_t *svc = svcs[i];
		size_t body_len = 0U;
		uint8_t flags;
		size_t entry_len;

		if ((svc == NULL) || (svc->persist_id == 0U) || (svc->serialize == NULL)) {
			STATS_INC(&s_stats_lock, s_stats, serialize_skip_count);
			continue;
		}

		ret = svc->serialize(s_serialize_buf, sizeof(s_serialize_buf), &body_len,
				     svc->user_ctx);
		if (ret == -ENOTSUP) {
			STATS_INC(&s_stats_lock, s_stats, serialize_skip_count);
			continue;
		}
		if (ret != 0) {
			STATS_ERROR(&s_stats_lock, s_stats, ret);
			return -EIO;
		}

		entry_len = (size_t)NFC_STORE_ENTRY_OVERHEAD + body_len;
		if ((pos + entry_len + NFC_STORE_BLOB_CRC_SIZE) > sizeof(s_staging_buf)) {
			STATS_ERROR(&s_stats_lock, s_stats, -ENOMEM);
			return -ENOMEM;
		}

		flags = nfc_store_flags_for_persist_id(svc->persist_id, true);
		s_staging_buf[pos++] = svc->persist_id;
		s_staging_buf[pos++] = flags;
		s_staging_buf[pos++] = (uint8_t)(body_len & 0xFFU);
		s_staging_buf[pos++] = (uint8_t)((body_len >> 8U) & 0xFFU);
		(void)memcpy(&s_staging_buf[pos], s_serialize_buf, body_len);
		pos += body_len;
		payload_len += entry_len;
	}

	s_staging_buf[4] = (uint8_t)(payload_len & 0xFFU);
	s_staging_buf[5] = (uint8_t)((payload_len >> 8U) & 0xFFU);

	ret = nfc_store_append_crc(s_staging_buf, sizeof(s_staging_buf),
				   NFC_STORE_BLOB_HDR_SIZE + payload_len, &total);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -ENOMEM;
	}

	if (s_save_cb == NULL) {
		return -ENODEV;
	}

	ret = s_save_cb(tag, s_staging_buf, total, s_save_user_ctx);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -EIO;
	}

	STATS_INC(&s_stats_lock, s_stats, save_count);
	return 0;
}

int nfc_store_peek_entry_meta(const char *tag, uint8_t *persist_id, uint8_t *flags)
{
	size_t blob_len = 0U;
	uint16_t payload_len;
	uint8_t version;
	uint8_t entry_overhead;
	size_t pos;
	int ret;

	if ((persist_id == NULL) || (flags == NULL)) {
		return -EINVAL;
	}

	ret = nfc_store_check_quiescent();
	if (ret != 0) {
		return ret;
	}

	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	ret = nfc_store_validate_tag(tag);
	if (ret != 0) {
		return ret;
	}

	ret = nfc_store_read_envelope(tag, &blob_len, &payload_len, &version);
	if (ret != 0) {
		return ret;
	}

	if (payload_len == 0U) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	entry_overhead = (version == NFC_STORE_BLOB_VERSION_V1) ? NFC_STORE_ENTRY_OVERHEAD_V1 :
								  NFC_STORE_ENTRY_OVERHEAD;
	pos = NFC_STORE_BLOB_HDR_SIZE;
	if ((pos + entry_overhead) > (NFC_STORE_BLOB_HDR_SIZE + payload_len)) {
		STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
		return -EBADMSG;
	}

	*persist_id = s_staging_buf[pos++];
	if (version == NFC_STORE_BLOB_VERSION_V1) {
		*flags = 0U;
	} else {
		*flags = s_staging_buf[pos++];
	}

	return 0;
}

int nfc_store_load(const char *tag, const nfc_service_t *const *svcs, size_t n)
{
	size_t blob_len = 0U;
	size_t pos;
	uint16_t payload_len;
	uint8_t version;
	int ret;
	int last_err = 0;

	ret = nfc_store_check_quiescent();
	if (ret != 0) {
		return ret;
	}

	if (s_state != NFC_STORE_STATE_INITIALIZED) {
		return -ENODEV;
	}

	ret = nfc_store_validate_tag(tag);
	if (ret != 0) {
		return ret;
	}

	if ((n > 0U) && (svcs == NULL)) {
		return -EINVAL;
	}

	ret = nfc_store_read_envelope(tag, &blob_len, &payload_len, &version);
	if (ret != 0) {
		return ret;
	}

	pos = NFC_STORE_BLOB_HDR_SIZE;
	while (pos < (NFC_STORE_BLOB_HDR_SIZE + payload_len)) {
		uint8_t persist_id;
		uint8_t entry_flags;
		uint16_t entry_len;
		uint8_t entry_overhead;
		const nfc_service_t *svc;

		ARG_UNUSED(entry_flags);

		if (version == NFC_STORE_BLOB_VERSION_V1) {
			entry_overhead = NFC_STORE_ENTRY_OVERHEAD_V1;
		} else {
			entry_overhead = NFC_STORE_ENTRY_OVERHEAD;
		}

		if ((pos + entry_overhead) > (NFC_STORE_BLOB_HDR_SIZE + payload_len)) {
			STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
			return -EBADMSG;
		}

		persist_id = s_staging_buf[pos++];
		if (version != NFC_STORE_BLOB_VERSION_V1) {
			entry_flags = s_staging_buf[pos++];
		}
		entry_len = (uint16_t)s_staging_buf[pos] |
			      ((uint16_t)s_staging_buf[pos + 1U] << 8U);
		pos += 2U;

		if ((pos + entry_len) > (NFC_STORE_BLOB_HDR_SIZE + payload_len)) {
			STATS_INC(&s_stats_lock, s_stats, corrupt_blob_count);
			return -EBADMSG;
		}

		svc = nfc_store_find_service(svcs, n, persist_id);
		if (svc == NULL || svc->deserialize == NULL) {
			STATS_INC(&s_stats_lock, s_stats, deserialize_skip_count);
			pos += entry_len;
			continue;
		}

		ret = svc->deserialize(&s_staging_buf[pos], entry_len, svc->user_ctx);
		pos += entry_len;
		if (ret != 0) {
			STATS_INC(&s_stats_lock, s_stats, partial_load_count);
			STATS_ERROR(&s_stats_lock, s_stats, ret);
			last_err = ret;
		}
	}

	if (last_err != 0) {
		return -EBADMSG;
	}

	STATS_INC(&s_stats_lock, s_stats, load_count);
	return 0;
}

const nfc_store_config_t *nfc_store_get_config(void)
{
	return &s_config;
}

int nfc_store_get_stats(nfc_store_stats_t *out)
{
	return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

nfc_store_state_t nfc_store_get_state(void)
{
	return s_state;
}
