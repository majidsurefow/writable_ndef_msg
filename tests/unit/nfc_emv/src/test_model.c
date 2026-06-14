#include "emv.h"
#include "router/service.h"
#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>

static emv_card_image_t s_image;
static emv_card_image_t s_copy; /* Static to avoid stack allocation */
static uint8_t s_out[512];

ZTEST(emv_model, test_default_load)
{
	emv_card_image_load_default(&s_image);
	zassert_equal(s_image.app_aid_len, EMV_SERVICE_APP_AID_LEN);
	zassert_equal(s_image.record_count, 1U);
}

ZTEST(emv_model, test_serialize_roundtrip)
{
	size_t len = 0U;

	(void)memset(&s_copy, 0, sizeof(s_copy));

	emv_card_image_load_default(&s_image);
	zassert_ok(emv_serialize(&s_image, s_out, sizeof(s_out), &len));
	zassert_ok(emv_deserialize(&s_copy, s_out, len));
	zassert_equal(s_copy.pan_len, s_image.pan_len);
	zassert_equal(s_copy.record_count, s_image.record_count);
}

ZTEST(emv_model, test_deserialize_bad_aid)
{
	size_t len = 0U;

	emv_card_image_load_default(&s_image);
	s_image.app_aid_len = EMV_APP_AID_MAX_BYTES + 1U;
	zassert_ok(emv_serialize(&s_image, s_out, sizeof(s_out), &len));
	zassert_equal(emv_deserialize(&s_image, s_out, len), -EBADMSG);
}

ZTEST(emv_model, test_mastercard_aid_roundtrip)
{
	size_t len = 0U;
	static const uint8_t mc_aid[] = {0xA0U, 0x00U, 0x00U, 0x00U, 0x04U, 0x10U, 0x10U};

	(void)memset(&s_copy, 0, sizeof(s_copy));
	emv_card_image_load_default(&s_image);
	(void)memcpy(s_image.app_aid, mc_aid, sizeof(mc_aid));
	s_image.app_aid_len = sizeof(mc_aid);
	zassert_ok(emv_rebuild_caches(&s_image));
	zassert_ok(emv_serialize(&s_image, s_out, sizeof(s_out), &len));
	zassert_ok(emv_deserialize(&s_copy, s_out, len));
	zassert_equal(s_copy.app_aid_len, sizeof(mc_aid));
	zassert_mem_equal(s_copy.app_aid, mc_aid, sizeof(mc_aid));
}

ZTEST(emv_model, test_persist_id)
{
	zassert_equal(emv_persist_id(), NFC_PERSIST_ID_EMV);
}

ZTEST_SUITE(emv_model, NULL, NULL, NULL, NULL, NULL);
