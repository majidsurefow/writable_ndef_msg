#include "emv_listener.h"
#include "nfc_response_spy.h"
#include "nfc_test_apdu.h"
#include <string.h>
#include <zephyr/ztest.h>

static void reset(void *f)
{
	ARG_UNUSED(f);
	nfc_response_spy_reset();
	(void)emv_listener_shutdown();
	zassert_ok(emv_listener_init(NULL));
}

static void send_apdu(const uint8_t *apdu, size_t len)
{
	uint8_t storage[64];
	nfc_apdu_t parsed;
	const nfc_service_t *svc = emv_listener_get();

	zassert_ok(nfc_test_apdu_from_bytes(apdu, len, storage, sizeof(storage), &parsed));
	svc->on_apdu(&parsed, NULL);
}

static void assert_sw(uint16_t sw)
{
	const uint8_t *buf = NULL;
	size_t blen = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &blen));
	zassert_true(blen >= 2U);
	zassert_equal((uint16_t)((buf[blen - 2U] << 8U) | buf[blen - 1U]), sw);
}

ZTEST(emv_listener, test_ppse_select)
{
	const nfc_service_t *svc = emv_listener_get();

	reset(NULL);
	svc->on_select(emv_ppse_aid, EMV_PPSE_AID_LEN, NULL);
	assert_sw(NFC_SW_OK);
	zassert_equal(emv_listener_session_state(), EMV_SESSION_PPSE_SELECTED);
}

ZTEST(emv_listener, test_gpo_after_app_select)
{
	const nfc_service_t *svc = emv_listener_get();
	static const uint8_t gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U};

	reset(NULL);
	svc->on_select(emv_service_app_aid, EMV_SERVICE_APP_AID_LEN, NULL);
	send_apdu(gpo, sizeof(gpo));
	assert_sw(NFC_SW_OK);
	zassert_equal(emv_listener_session_state(), EMV_SESSION_GPO_DONE);
}

ZTEST(emv_listener, test_read_record)
{
	const nfc_service_t *svc = emv_listener_get();
	static const uint8_t gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U};
	static const uint8_t read[] = {0x00U, 0xB2U, 0x01U, 0x0CU, 0x00U};

	reset(NULL);
	svc->on_select(emv_service_app_aid, EMV_SERVICE_APP_AID_LEN, NULL);
	send_apdu(gpo, sizeof(gpo));
	send_apdu(read, sizeof(read));
	assert_sw(NFC_SW_OK);
}

ZTEST(emv_listener, test_read_record_wrong_sfi)
{
	const nfc_service_t *svc = emv_listener_get();
	static const uint8_t gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U};
	static const uint8_t read[] = {0x00U, 0xB2U, 0x01U, 0x14U, 0x00U};

	reset(NULL);
	svc->on_select(emv_service_app_aid, EMV_SERVICE_APP_AID_LEN, NULL);
	send_apdu(gpo, sizeof(gpo));
	send_apdu(read, sizeof(read));
	assert_sw(0x6A83U);
}

ZTEST(emv_listener, test_read_record_out_of_range)
{
	const nfc_service_t *svc = emv_listener_get();
	static const uint8_t gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U};
	static const uint8_t read[] = {0x00U, 0xB2U, 0x02U, 0x0CU, 0x00U};

	reset(NULL);
	svc->on_select(emv_service_app_aid, EMV_SERVICE_APP_AID_LEN, NULL);
	send_apdu(gpo, sizeof(gpo));
	send_apdu(read, sizeof(read));
	assert_sw(0x6A83U);
}

ZTEST(emv_listener, test_gpo_before_app_select)
{
	static const uint8_t gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U};

	reset(NULL);
	send_apdu(gpo, sizeof(gpo));
	assert_sw(NFC_SW_CONDITIONS_NOT_SAT);
}

ZTEST_SUITE(emv_listener, NULL, NULL, reset, NULL, NULL);
