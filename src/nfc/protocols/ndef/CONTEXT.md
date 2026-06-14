# protocols/ndef — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status:** SHIPPED

## Purpose

**NDEF Type-4 Tag (T4T) only** — ISO-DEP poller/listener on the NDEF v2 AID (`D2 76 00 00 85 01 01`). Owns the data model (CC file + NDEF message), `ndef_poller.c` (SELECT/READ BINARY), and `ndef_listener.c` (T4T APDU emulation).

**Type-2 NDEF is out of scope here.** T2 poll/read (page READ, CC page 3, TLV page 4+) lives in [`../ultralight/CONTEXT.md`](../ultralight/CONTEXT.md). T2 emulate routes through ultralight → **T4 adapter** → `ndef_listener` — see [`NDEF_T2_T4_ROUTING.md`](../../../../docs/nfc/NDEF_T2_T4_ROUTING.md) and cookbook [§5.2](../../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md#52-mf_ultralight-type-2-ndef-poll-path).

## Key files

| File | Owns |
|------|------|
| `ndef.c` | Data model: `ndef_t` struct, `ndef_serialize`/`ndef_deserialize`, NDEF message + CC file handling |
| `ndef.h` | Public types, model API, capacity symbols |
| `ndef_poller.c` | Reader poller: SELECT CC → READ BINARY CC → SELECT NDEF → READ BINARY NDEF |
| `ndef_poller.h` | Poller API: `ndef_poller_read` |
| `ndef_listener.c` | T4T APDU listener: responds to SELECT/READ BINARY for NDEF emulation |
| `ndef_listener.h` | Listener API: `ndef_listener_load`, `ndef_listener_on_apdu` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_NDEF` | n | Gate model + serialize/deserialize |
| `NFC_NDEF_MAX_SIZE` | 500 | Max NDEF message payload (bytes); bounds-checked in deserialize |

## Deps

- **Upstream (calls):** None (pure model + APDU consumer)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_router` (AID dispatch), `nfc_store` (serialize)

## Invariants & safety

- **Buffers:** `NFC_NDEF_MAX_SIZE` bounds the NLEN field; oversize → `-ENOSPC`
- **CRC:** CC file integrity checked in listener response generation
- **Error propagation:** Functions return negative errno; no silent drops

## Roles

- **Poller:** `ndef_poller.c` (Tier B) — reader NFC-A/ISO-DEP
- **Listener:** `ndef_listener.c` (Tier C) — T4T APDU emulation

## Wire/spec

- NFC Forum Type-4 Tag Operation Specification v2.0
- **Emulatable:** Yes (full T4T listener)
- **Canonical poller TX (6 steps):** SELECT NDEF AID → SELECT CC → READ CC (15 B) → SELECT NDEF file → READ NLEN → READ body — asserted in `tests/unit/nfc_ndef/src/test_poller.c` `test_read_tx_sequence`; cookbook [§5.1](../../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md#51-ndef-type-4-iso-dep--t4t)

## Capacity symbols

| CONFIG_NFC_NDEF_* | Default | Bound it protects |
|-------------------|---------|-------------------|
| `MAX_SIZE` | 500 | NDEF file size in model + serialize buffer |

## Fixtures ↔ goldens

- **Flipper:** No T4 NDEF poller upstream (`protocols/ndef/` does not exist in Flipper). Flipper `supported_cards/ndef.c` is **content-parse reference only** (UL/MFC/SLIX TLV after read) — not wire goldens for this module.
- **Generated:** `scripts/nfc/ndef_to_fixture.py --variant all` → `tests/fixtures/ndef/` + store `.card.bin` (NXP `RW_NDEF_T4T` table, cookbook §5.1)
- **Loopback:** `tests/common/nfc_virtual_loopback` (T4 poller↔listener); Tier E+ in `tests/unit/nfc_reader/src/test_virtual_loopback.c`

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_ndef.model` | A (model) | `NFC_PROTOCOL_NDEF` | Serialize/deserialize, CC parse, NLEN bounds |
| `sample.nfc.unit.nfc_ndef.poller` | B (poller) | `+NFC_NDEF_TEST_TIER_POLLER` | Poller state machine, SELECT/READ sequence |
| `sample.nfc.unit.nfc_ndef.listener` | C (listener) | `+NFC_NDEF_TEST_TIER_LISTENER` | Listener APDU responses, emulation fidelity |

**Tier counts:** 3 configs, 87 cases (model + poller + listener).
**Twister dir:** `tests/unit/nfc_ndef`

## Live HIL

- **Tag:** Any NDEF Type-4 tag (e.g., ST25TA, NTAG I2C)
- **Golden:** `nfc emulate golden ndef_url`
- **Overlay:** `overlay-pn7160-stack.conf` (reader); `+overlay-pn7160-listen.conf` (emulate)
- **Shell sequence:**
  ```
  nfc scan start → nfc scan stop → nfc read tag1 → nfc emulate tag1
  nfc_transport stats  # apdu_assembled > 0, apdu_drop_cons == 0
  ```
- **PASS criteria:** External reader reads back NDEF matching stored `.card`; SELECT returns `90 00`

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` + `src/nfc/reader/nfc_reader_shell_cmds.c` under `NFC_APPLETS_SHELL` / `NFC_READER_SHELL`. NDEF is exercised via `nfc read`, `nfc emulate`, `nfc loop`.
