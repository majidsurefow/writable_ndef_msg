#include "emv_poller.h"
#include "nfc_session_mock.h"
#include "Emv_mock.h"
#include <string.h>
#include <zephyr/ztest.h>

static emv_card_image_t s_data;
static nfc_reader_session_t s_session;

static void setup(void *f)
{
	ARG_UNUSED(f);
	nfc_session_mock_reset();
	s_session.active = true;
	emv_card_image_reset(&s_data);
}

ZTEST(emv_poller, test_detect_ok)
{
	setup(NULL);
	nfc_session_mock_load(emv_Emv_read_steps, 1U);
	zassert_ok(emv_poller_detect(&s_session));
}

ZTEST(emv_poller, test_read_partial)
{
	setup(NULL);
	nfc_session_mock_load(emv_Emv_read_steps, EMV_EMV_READ_STEP_COUNT);
	zassert_ok(emv_poller_read(&s_session, &s_data));
	zassert_equal(s_data.record_count, 1U);
	zassert_equal(s_data.app_aid_len, EMV_SERVICE_APP_AID_LEN);
	zassert_mem_equal(s_data.app_aid, emv_service_app_aid, EMV_SERVICE_APP_AID_LEN);
	zassert_equal(s_data.afl[0], 0x08U);
	zassert_equal(s_data.afl[1], 0x01U);
	zassert_equal(s_data.afl[2], 0x01U);
	zassert_equal(s_data.pan_len, 16U);
	zassert_equal(s_data.track2_len, 16U);
}

ZTEST(emv_poller, test_read_multi_record)
{
	setup(NULL);
	nfc_session_mock_load(emv_Emv_read_multi_steps, EMV_EMV_READ_MULTI_STEP_COUNT);
	zassert_ok(emv_poller_read(&s_session, &s_data));
	zassert_equal(s_data.record_count, 2U);
	zassert_equal(s_data.afl[2], 0x02U);
	zassert_equal(s_data.pan_len, 16U);
	zassert_equal(s_data.track2_len, 3U);
	zassert_mem_equal(s_data.track2, emv_record2_ok + 4U, 3U);
}

ZTEST_SUITE(emv_poller, NULL, NULL, setup, NULL, NULL);
