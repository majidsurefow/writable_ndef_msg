#include "aliro_listener.h"
#include "nfc_response_spy.h"
#include "nfc_test_apdu.h"
#include "nfc_work_spy.h"
#include <string.h>
#include <zephyr/ztest.h>

static void reset(void *f)
{
	ARG_UNUSED(f);
	nfc_response_spy_reset();
	nfc_work_spy_reset();
	(void)aliro_listener_shutdown();
	zassert_ok(aliro_listener_init(NULL));
}

static void send_apdu(const uint8_t *apdu, size_t len)
{
	uint8_t storage[128];
	nfc_apdu_t parsed;

	zassert_ok(nfc_test_apdu_from_bytes(apdu, len, storage, sizeof(storage), &parsed));
	aliro_listener_get()->on_apdu(&parsed, NULL);
}

ZTEST(aliro_listener, test_select_expedited)
{
	reset(NULL);
	aliro_listener_get()->on_select(aliro_expedited_aid, ALIRO_AID_LEN, NULL);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_AWAIT_AUTH0);
}

ZTEST(aliro_listener, test_auth0_submits_work)
{
	static const uint8_t auth0[] = {0x80U, 0x80U, 0x00U, 0x00U, 0x10U,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	reset(NULL);
	aliro_listener_get()->on_select(aliro_expedited_aid, ALIRO_AID_LEN, NULL);
	send_apdu(auth0, sizeof(auth0));
	zassert_equal(nfc_work_spy_submit_count(), 1U);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_AWAIT_AUTH1);
}

ZTEST(aliro_listener, test_field_off_resets)
{
	reset(NULL);
	aliro_listener_get()->on_select(aliro_expedited_aid, ALIRO_AID_LEN, NULL);
	aliro_listener_get()->on_field_off(NULL);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_IDLE);
}

ZTEST(aliro_listener, test_unknown_cla_rejects)
{
	static const uint8_t bad[] = {0x00U, 0x80U, 0x00U, 0x00U, 0x00U};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	reset(NULL);
	send_apdu(bad, sizeof(bad));
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal((uint16_t)((buf[len - 2U] << 8U) | buf[len - 1U]), NFC_SW_CLA_NOT_SUPPORTED);
}

ZTEST_SUITE(aliro_listener, NULL, NULL, reset, NULL, NULL);
