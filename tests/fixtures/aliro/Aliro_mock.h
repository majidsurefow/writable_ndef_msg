#ifndef TESTS_FIXTURES_ALIRO_ALIRO_MOCK_H_
#define TESTS_FIXTURES_ALIRO_ALIRO_MOCK_H_

#include "nfc_session_mock.h"

static const uint8_t aliro_select_ok[] = {
	0x00U, 0x00U,
	0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U,
	0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U,
};

static const uint8_t aliro_auth0_req[] = {
#include "AUTH0_req.inc"
};

static const uint8_t aliro_auth0_rsp[] = {
#include "AUTH0_rsp.inc"
};

static const uint8_t aliro_auth1_req[] = {
#include "AUTH1_req.inc"
};

static const uint8_t aliro_auth1_rsp[] = {
#include "AUTH1_rsp.inc"
};

static const nfc_session_mock_step_t aliro_Aliro_read_steps[] = {
	{.rx = aliro_select_ok, .rx_len = sizeof(aliro_select_ok), .err = 0},
};

static const nfc_session_mock_step_t aliro_Aliro_auth_chain_steps[] = {
	{.rx = aliro_select_ok, .rx_len = sizeof(aliro_select_ok), .err = 0},
	{.rx = aliro_auth0_rsp, .rx_len = sizeof(aliro_auth0_rsp), .err = 0},
	{.rx = aliro_auth1_rsp, .rx_len = sizeof(aliro_auth1_rsp), .err = 0},
};

#define ALIRO_ALIRO_READ_STEP_COUNT ARRAY_SIZE(aliro_Aliro_read_steps)
#define ALIRO_ALIRO_AUTH_CHAIN_STEP_COUNT ARRAY_SIZE(aliro_Aliro_auth_chain_steps)

#endif
