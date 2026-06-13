/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFCT (nrfxlib nfc_t4t_lib) listen-only nfc_transport backend.
 */

#include "hal/nfc_apdu_asm.h"
#include "hal/nfc_apdu_pool.h"
#include "hal/nfc_transport.h"
#include "run/nfc_stack_workq.h"

#include "../../stats.h"

#include <errno.h>
#include <string.h>

#include <nfc_t4t_lib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nfc_transport_nrfx, CONFIG_LOG_DEFAULT_LEVEL);

/* HAL caps 255 B; T4T lib allows 0xFFF0 — send_response enforces the HAL limit. */
BUILD_ASSERT(NFC_TRANSPORT_MAX_RESPONSE_LEN <= NFC_T4T_MAX_PAYLOAD_SIZE);

/* nrfxlib selects PICC vs NDEF by payload registration, not a runtime setter. */
enum {
	NFC_TRANSPORT_T4T_EMU_MODE = NFC_T4T_EMUMODE_PICC,
};

BUILD_ASSERT(NFC_TRANSPORT_T4T_EMU_MODE == NFC_T4T_EMUMODE_PICC);

#if IS_ENABLED(CONFIG_NFC_ROLE_READER)
BUILD_ASSERT(!IS_ENABLED(CONFIG_NFC_ROLE_READER),
	     "NFCT backend is listen-only; disable CONFIG_NFC_ROLE_READER or use PN7160");
#endif

#define NFC_SESSION_NONE   0
#define NFC_SESSION_LISTEN 1

/* nfc_apdu_asm_drop_t values — kept as uint8_t for irq_lock-free ISR state. */

static const uint8_t s_sw_wrong_length[] = { 0x67U, 0x00U };

static atomic_t s_state = ATOMIC_INIT(NFC_TRANSPORT_STATE_UNINITIALIZED);
static atomic_t s_session = ATOMIC_INIT(NFC_SESSION_NONE);

static nfc_transport_config_t s_config = {
	.fwi = NFC_TRANSPORT_DEFAULT_FWI,
};
static nfc_transport_stats_t s_stats;
static struct k_spinlock s_stats_lock;

static nfc_transport_ops_t s_ops;
static void *s_ops_user_ctx;
static bool s_ops_registered;

static nfc_uid_t s_pending_uid;
static bool s_pending_uid_valid;
static struct k_mutex s_uid_mutex;

static struct k_work s_field_on_work;
static struct k_work s_field_off_work;
static struct k_work s_apdu_work;

/* ISR → fifo → apdu_work_handler → on_apdu (callee unref). Shared pool in nfc_apdu_pool.c. */
static struct k_fifo s_apdu_fifo;

static struct net_buf *s_cur_buf;
static nfc_apdu_asm_drop_t s_asm_drop;

static const nfc_transport_caps_t s_caps = {
	.roles = NFC_ROLE_LISTEN,
	.technologies = NFC_TECH_ISO_DEP_A,
	.tier = NFC_TIER_SMART,
};

static bool uid_len_valid(uint8_t len)
{
	return len == NFC_UID_LEN_SINGLE || len == NFC_UID_LEN_DOUBLE ||
	       len == NFC_UID_LEN_TRIPLE;
}

static int nrf_to_errno(int rc)
{
	return (rc == 0) ? 0 : -EIO;
}

static void discard_cur_buf(void)
{
	unsigned int key = irq_lock();
	struct net_buf *buf = s_cur_buf;

	s_cur_buf = NULL;
	s_asm_drop = NFC_APDU_ASM_OK;
	irq_unlock(key);

	if (buf != NULL) {
		net_buf_unref(buf);
	}
}

static void nfc_transport_stats_on_fragment(bool dropped_nobuf, bool oversized, bool assembled)
{
	STATS_SCOPE(&s_stats_lock, {
		if (dropped_nobuf) {
			s_stats.frag_dropped_no_buffer++;
		}
		if (oversized) {
			s_stats.apdu_oversized_count++;
		}
		if (assembled) {
			s_stats.apdu_assembled_count++;
		}
		s_stats.fragment_rx_count++;
	});
}

static void nfc_hal_on_fragment(const uint8_t *data, size_t len, bool more)
{
	bool dropped_nobuf = false;
	bool oversized = false;
	bool assembled = false;
	bool send_wrong_length = false;
	struct net_buf *enqueue_buf = NULL;
	unsigned int key = irq_lock();

	if (s_cur_buf == NULL && s_asm_drop == NFC_APDU_ASM_OK) {
		s_cur_buf = net_buf_alloc(nfc_apdu_pool_get(), K_NO_WAIT);
		if (s_cur_buf == NULL) {
			s_asm_drop = NFC_APDU_ASM_DROP_NOBUF;
			dropped_nobuf = true;
		}
	}

	if (s_asm_drop == NFC_APDU_ASM_OK && s_cur_buf != NULL) {
		nfc_apdu_asm_drop_t next_drop = nfc_apdu_asm_on_frag(s_asm_drop, s_cur_buf->len,
								     len, s_cur_buf->size);

		if (next_drop == NFC_APDU_ASM_DROP_OVERSIZE) {
			net_buf_unref(s_cur_buf);
			s_cur_buf = NULL;
			s_asm_drop = NFC_APDU_ASM_DROP_OVERSIZE;
			oversized = true;
		} else {
			net_buf_add_mem(s_cur_buf, data, len);
		}
	}

	if (!more) {
		switch (s_asm_drop) {
		case NFC_APDU_ASM_OK:
			if (s_cur_buf != NULL) {
				/* Ownership: ISR → fifo (WQ owns after k_fifo_get). */
				enqueue_buf = s_cur_buf;
				s_cur_buf = NULL;
				assembled = true;
			}
			break;
		case NFC_APDU_ASM_DROP_OVERSIZE:
			send_wrong_length = true;
			oversized = true;
			break;
		case NFC_APDU_ASM_DROP_NOBUF:
		default:
			break;
		}
		s_asm_drop = NFC_APDU_ASM_OK;
	}

	irq_unlock(key);

	if (enqueue_buf != NULL) {
		k_fifo_put(&s_apdu_fifo, enqueue_buf);
		(void)k_work_submit_to_queue(nfc_stack_wq_get(), &s_apdu_work);
	} else if (send_wrong_length) {
		(void)nfc_t4t_response_pdu_send(s_sw_wrong_length, sizeof(s_sw_wrong_length));
	}

	nfc_transport_stats_on_fragment(dropped_nobuf, oversized, assembled);
}

static void field_on_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (s_ops_registered && s_ops.on_field_on != NULL) {
		s_ops.on_field_on(s_ops_user_ctx);
	}
}

static void field_off_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (s_ops_registered && s_ops.on_field_off != NULL) {
		s_ops.on_field_off(s_ops_user_ctx);
	}
}

static void apdu_work_handler(struct k_work *work)
{
	struct net_buf *buf;

	ARG_UNUSED(work);

	while ((buf = k_fifo_get(&s_apdu_fifo, K_NO_WAIT)) != NULL) {
		if (s_ops_registered && s_ops.on_apdu != NULL) {
			/* Callee owns ref — must net_buf_unref after router returns. */
			s_ops.on_apdu(buf, s_ops_user_ctx);
		} else {
			net_buf_unref(buf);
			STATS_INC(&s_stats_lock, s_stats, apdu_dropped_no_consumer);
		}
	}
}

static void nfc_t4t_event_cb(void *context, nfc_t4t_event_t event, const uint8_t *data,
			     size_t data_length, uint32_t flags)
{
	ARG_UNUSED(context);

	switch (event) {
	case NFC_T4T_EVENT_FIELD_ON:
		discard_cur_buf();
		STATS_INC(&s_stats_lock, s_stats, field_on_count);
		(void)k_work_submit_to_queue(nfc_stack_wq_get(), &s_field_on_work);
		break;
	case NFC_T4T_EVENT_FIELD_OFF:
		discard_cur_buf();
		STATS_INC(&s_stats_lock, s_stats, field_off_count);
		(void)k_work_submit_to_queue(nfc_stack_wq_get(), &s_field_off_work);
		break;
	case NFC_T4T_EVENT_DATA_IND:
		nfc_hal_on_fragment(data, data_length,
				    (flags & NFC_T4T_DI_FLAG_MORE) != 0U);
		break;
	case NFC_T4T_EVENT_DATA_TRANSMITTED:
	case NFC_T4T_EVENT_NDEF_READ:
	case NFC_T4T_EVENT_NDEF_UPDATED:
	case NFC_T4T_EVENT_NONE:
	default:
		break;
	}
}

static int apply_nfcid1(const nfc_uid_t *uid)
{
	int ret;

	if (uid == NULL) {
		return 0;
	}

	ret = nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, uid->bytes, uid->len);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	s_pending_uid = *uid;
	s_pending_uid_valid = true;
	return 0;
}

const nfc_transport_caps_t *nfc_transport_get_capabilities(void)
{
	return &s_caps;
}

const nfc_transport_config_t *nfc_transport_get_config(void)
{
	return &s_config;
}

int nfc_transport_get_stats(nfc_transport_stats_t *out)
{
	return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

nfc_transport_state_t nfc_transport_get_state(void)
{
	return (nfc_transport_state_t)atomic_get(&s_state);
}

int nfc_transport_init(void)
{
	nfc_transport_state_t st = nfc_transport_get_state();
	uint8_t fwi = s_config.fwi;
	int ret;

	if (st != NFC_TRANSPORT_STATE_UNINITIALIZED && st != NFC_TRANSPORT_STATE_STOPPED) {
		return -EALREADY;
	}

	STATS_RESET(s_stats);

	k_fifo_init(&s_apdu_fifo);
	k_work_init(&s_field_on_work, field_on_work_handler);
	k_work_init(&s_field_off_work, field_off_work_handler);
	k_work_init(&s_apdu_work, apdu_work_handler);
	k_mutex_init(&s_uid_mutex);

	ret = nfc_t4t_setup(nfc_t4t_event_cb, NULL);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	/*
	 * Raw ISO-DEP / PICC mode: do not call nfc_t4t_ndef_* before emulation_start.
	 * NFC_TRANSPORT_T4T_EMU_MODE documents the locked listen profile for aid_router.
	 */
	ARG_UNUSED(NFC_TRANSPORT_T4T_EMU_MODE);

	ret = nfc_t4t_parameter_set(NFC_T4T_PARAM_FWI, &fwi, sizeof(fwi));
	if (ret != 0) {
		(void)nfc_t4t_done();
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_INITIALIZED);
	return 0;
}

int nfc_transport_shutdown(void)
{
	if (atomic_get(&s_session) == NFC_SESSION_LISTEN) {
		(void)nfc_transport_stop();
	}

	(void)k_work_cancel_sync(&s_field_on_work, NULL);
	(void)k_work_cancel_sync(&s_field_off_work, NULL);
	(void)k_work_cancel_sync(&s_apdu_work, NULL);

	discard_cur_buf();

	while (true) {
		struct net_buf *buf = k_fifo_get(&s_apdu_fifo, K_NO_WAIT);

		if (buf == NULL) {
			break;
		}
		net_buf_unref(buf);
	}

	(void)nfc_t4t_done();

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_UNINITIALIZED);
	s_ops_registered = false;
	memset(&s_ops, 0, sizeof(s_ops));
	s_ops_user_ctx = NULL;
	s_pending_uid_valid = false;

	return 0;
}

int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops, void *user_ctx)
{
	nfc_transport_state_t st = nfc_transport_get_state();

	if (st == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (st == NFC_TRANSPORT_STATE_STARTED) {
		return -EBUSY;
	}

	if (ops == NULL) {
		memset(&s_ops, 0, sizeof(s_ops));
		s_ops_user_ctx = NULL;
		s_ops_registered = false;
		return 0;
	}

	s_ops = *ops;
	s_ops_user_ctx = user_ctx;
	s_ops_registered = true;
	return 0;
}

int nfc_transport_discover_start(nfc_tech_t tech_mask)
{
	ARG_UNUSED(tech_mask);
	return -ENOTSUP;
}

int nfc_transport_discover_stop(void)
{
	return -ENOTSUP;
}

int nfc_transport_discover_wait(nfc_transport_tag_info_t *info, k_timeout_t timeout)
{
	ARG_UNUSED(info);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
}

int nfc_transport_tag_transceive(const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_max,
				   size_t *rx_len, k_timeout_t timeout)
{
	ARG_UNUSED(tx);
	ARG_UNUSED(tx_len);
	ARG_UNUSED(rx);
	ARG_UNUSED(rx_max);
	ARG_UNUSED(rx_len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
}

int nfc_transport_start(const nfc_uid_t *uid)
{
	const nfc_uid_t *apply_uid = uid;
	int ret;

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_NONE) {
		return -EBUSY;
	}

	if (apply_uid != NULL) {
		if (!uid_len_valid(apply_uid->len)) {
			return -EINVAL;
		}
	} else if (s_pending_uid_valid) {
		apply_uid = &s_pending_uid;
	}

	if (apply_uid != NULL) {
		ret = apply_nfcid1(apply_uid);
		if (ret != 0) {
			return ret;
		}
	}

	(void)nfc_stack_wq_get();

	ret = nfc_t4t_emulation_start();
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	atomic_set(&s_session, NFC_SESSION_LISTEN);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_STARTED);
	return 0;
}

int nfc_transport_stop(void)
{
	int ret = 0;

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_LISTEN) {
		return 0;
	}

	(void)k_mutex_lock(&s_uid_mutex, K_FOREVER);

	ret = nfc_t4t_emulation_stop();
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
	}

	discard_cur_buf();
	(void)k_work_cancel_sync(&s_field_on_work, NULL);
	(void)k_work_cancel_sync(&s_field_off_work, NULL);
	(void)k_work_cancel_sync(&s_apdu_work, NULL);

	while (true) {
		struct net_buf *buf = k_fifo_get(&s_apdu_fifo, K_NO_WAIT);

		if (buf == NULL) {
			break;
		}
		net_buf_unref(buf);
	}

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_STOPPED);
	k_mutex_unlock(&s_uid_mutex);

	return nrf_to_errno(ret);
}

int nfc_transport_set_uid(const nfc_uid_t *uid)
{
	int ret;

	if (uid == NULL) {
		return -EINVAL;
	}

	if (!uid_len_valid(uid->len)) {
		return -EINVAL;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_LISTEN) {
		s_pending_uid = *uid;
		s_pending_uid_valid = true;
		STATS_INC(&s_stats_lock, s_stats, uid_rotation_count);
		return 0;
	}

	(void)k_mutex_lock(&s_uid_mutex, K_FOREVER);

	ret = nfc_t4t_emulation_stop();
	if (ret != 0) {
		k_mutex_unlock(&s_uid_mutex);
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	discard_cur_buf();

	ret = apply_nfcid1(uid);
	if (ret != 0) {
		(void)nfc_t4t_emulation_start();
		k_mutex_unlock(&s_uid_mutex);
		return ret;
	}

	ret = nfc_t4t_emulation_start();
	if (ret != 0) {
		atomic_set(&s_state, NFC_TRANSPORT_STATE_STOPPED);
		atomic_set(&s_session, NFC_SESSION_NONE);
		k_mutex_unlock(&s_uid_mutex);
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	STATS_INC(&s_stats_lock, s_stats, uid_rotation_count);
	k_mutex_unlock(&s_uid_mutex);
	return 0;
}

int nfc_transport_send_response(const uint8_t *buf, size_t len)
{
	int ret;

	if (buf == NULL || len == 0U || len > NFC_TRANSPORT_MAX_RESPONSE_LEN) {
		return -EINVAL;
	}

	if (atomic_get(&s_session) != NFC_SESSION_LISTEN) {
		return -ENODEV;
	}

	ret = nfc_t4t_response_pdu_send(buf, len);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, nrf_to_errno(ret));
		return nrf_to_errno(ret);
	}

	STATS_INC(&s_stats_lock, s_stats, response_tx_count);
	return 0;
}

int nfc_transport_submit_work(struct k_work *work)
{
	if (work == NULL) {
		return -EINVAL;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	(void)k_work_submit_to_queue(nfc_stack_wq_get(), work);
	return 0;
}
