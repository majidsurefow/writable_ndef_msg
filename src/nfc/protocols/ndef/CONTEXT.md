# protocols/ndef — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

NDEF Type-4 Tag protocol: data model (NDEF message + CC file), serialize/deserialize, poller (reader), and listener (card emulation via T4T APDU). Does NOT own the T2T NDEF adapter path—that routes through `ultralight_listener → ndef_listener`.

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
- **Emulatable:** Yes (full listener)

## Capacity symbols

| CONFIG_NFC_NDEF_* | Default | Bound it protects |
|-------------------|---------|-------------------|
| `MAX_SIZE` | 500 | NDEF file size in model + serialize buffer |

## Fixtures ↔ goldens

- **Flipper:** No direct Flipper NDEF fixture (NDEF is carried by Ultralight/NTAG tags)
- **Generated:** `scripts/nfc/ndef_to_fixture.py --variant all` produces store goldens
- **Loopback:** `tests/common/nfc_virtual_loopback` (poller↔listener)

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
