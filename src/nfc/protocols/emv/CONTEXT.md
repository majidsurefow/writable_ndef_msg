# protocols/emv — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

EMV contactless protocol: data model (PPSE response, AID, GPO, READ RECORD records), serialize/deserialize, poller (reader), and ISO-DEP listener (card emulation). **Partially emulatable**—public transaction records can be emulated; cryptographic auth not replayed.

## Key files

| File | Owns |
|------|------|
| `emv.c` | Data model: `emv_t` struct, PPSE, AID, GPO, records, serialize/deserialize |
| `emv.h` | Public types, model API, capacity symbols |
| `emv_poller.c` | Reader poller: SELECT PPSE → SELECT AID → GPO → READ RECORD |
| `emv_poller.h` | Poller API: `emv_poller_read` |
| `emv_listener.c` | ISO-DEP listener: responds to EMV APDU sequence |
| `emv_listener.h` | Listener API: `emv_listener_load`, `emv_listener_on_apdu` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_EMV` | n | Gate model + poller + listener |
| `NFC_EMV_MAX_RECORDS` | 2 | Max READ RECORD blobs |
| `NFC_EMV_RECORD_SIZE` | 64 | Max tag-70 record size |

## Deps

- **Upstream (calls):** None (standalone)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_router` (AID dispatch), `nfc_store`

## Invariants & safety

- **Buffers:** Record count validated against `NFC_EMV_MAX_RECORDS`
- **Error propagation:** Functions return negative errno; record not found → `-ENOENT`

## Roles

- **Poller:** `emv_poller.c` (Tier B) — reader ISO-DEP
- **Listener:** `emv_listener.c` (Tier C) — ISO-DEP emulation

## Wire/spec

- EMV Contactless Specifications for Payment Systems, Book B/C
- **Emulatable:** Yes (partial; public records only, no cryptographic auth)

## Capacity symbols

| CONFIG_NFC_EMV_* | Default | Bound it protects |
|------------------|---------|-------------------|
| `MAX_RECORDS` | 2 | Record array size |
| `RECORD_SIZE` | 64 | Single record buffer |

## Fixtures ↔ goldens

- **Flipper:** No Flipper EMV fixture (reader-captured `.card`)
- **Generated:** Reader-captured card via `nfc read`
- **Loopback:** `tests/common/nfc_virtual_loopback` (poller↔listener)

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_emv.model` | A (model) | `NFC_PROTOCOL_EMV` | Serialize/deserialize, PPSE/AID parse |
| `sample.nfc.unit.nfc_emv.poller` | B (poller) | `+NFC_EMV_TEST_TIER_POLLER` | Poller PPSE → GPO → READ RECORD |
| `sample.nfc.unit.nfc_emv.listener` | C (listener) | `+NFC_EMV_TEST_TIER_LISTENER` | Listener APDU responses |

**Tier counts:** 3 configs, 17 cases (model + poller + listener).
**Twister dir:** `tests/unit/nfc_emv`

## Live HIL

- **Tag:** EMV contactless card
- **Golden:** Reader-captured `.card`
- **Overlay:** `overlay-pn7160-stack.conf` (reader); `+overlay-pn7160-listen.conf` (emulate)
- **Shell sequence:**
  ```
  nfc read tag1 → nfc stack stop → nfc emulate tag1
  nfc_transport stats  # apdu_assembled > 0
  ```
- **PASS criteria:** External reader receives valid PPSE response; SELECT AID → GPO sequence completes

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. EMV via `nfc read`, `nfc emulate`, `nfc loop`.
