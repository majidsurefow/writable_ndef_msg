# protocols/emv — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

EMV contactless protocol: data model (PPSE response, AID, GPO, READ RECORD records), serialize/deserialize, poller (reader), and ISO-DEP listener (card emulation). **Partially emulatable**—public transaction records can be emulated; cryptographic auth not replayed.

## Key files

| File | Owns |
|------|------|
| `emv.c` | Data model: `emv_card_image_t`, PPSE, AID, GPO, records, serialize/deserialize |
| `emv.h` | Public types, model API, capacity symbols |
| `emv_poller.c` | Reader poller: SELECT PPSE → parse FCI AID → SELECT AID → GPO → AFL READ RECORD loop |
| `emv_poller.h` | Poller API: `emv_poller_detect`, `emv_poller_read` |
| `emv_listener.c` | ISO-DEP listener: PPSE/app SELECT, GPO, READ RECORD per AFL |
| `emv_listener.h` | Listener API: `emv_listener_init`, `emv_listener_get` |

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
- **Error propagation:** Functions return negative errno; record not found → `-ENOENT` / SW `6A83`

## Roles

- **Poller:** `emv_poller.c` (Tier B) — PPSE FCI AID parse, AFL multi-record READ RECORD, PAN/track2 TLV extract
- **Listener:** `emv_listener.c` (Tier C) — ISO-DEP emulation with AFL-bound READ RECORD

## Wire/spec

- EMV Contactless Specifications for Payment Systems, Book B/C
- **Emulatable:** Yes (partial; public records only, no cryptographic auth)
- APDU catalog: [`docs/nfc/archive/waves/wave5-emv.md`](../../../docs/nfc/archive/waves/wave5-emv.md) §8

## Capacity symbols

| CONFIG_NFC_EMV_* | Default | Bound it protects |
|------------------|---------|-------------------|
| `MAX_RECORDS` | 2 | Record array size |
| `RECORD_SIZE` | 64 | Single record buffer |

## Fixtures ↔ goldens

- **Flipper:** No Flipper EMV fixture (no `emv/` protocol in upstream)
- **Synthetic:** `tests/fixtures/emv/Emv.card.bin`, `Emv_mc.card.bin` via `scripts/nfc/protocol_to_card_bin.py --emv` / `--emv-mc`
- **Store envelope:** `tests/fixtures/store/Emv_card.inc`, `Emv_mc_card.inc`
- **Poller mocks:** `tests/fixtures/emv/Emv_mock.h` — PPSE/GPO/READ RECORD RX scripts (single + multi-record AFL)
- **Loopback:** `test_virtual_loopback_emv` with `emv_compare` (AID, PAN, track2, AFL, record bytes)

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_emv.model` | A (model) | `NFC_PROTOCOL_EMV` | Serialize/deserialize, Mastercard AID roundtrip |
| `sample.nfc.unit.nfc_emv.poller` | B (poller) | `+NFC_EMV_TEST_TIER_POLLER` | PPSE AID select, AFL multi-record READ RECORD |
| `sample.nfc.unit.nfc_emv.listener` | C (listener) | `+NFC_EMV_TEST_TIER_LISTENER` | PPSE/app SELECT, GPO, READ RECORD, wrong SFI/range |
| `nfc_reader_emv_store` | E (store) | `NFC_PROTOCOL_EMV` | Golden `.card.bin` load/save |
| `nfc_reader_loopback.test_virtual_loopback_emv` | E+ | `CONFIG_NFC_TEST_LOOPBACK` | Deep compare poller↔listener |

**Tier counts:** 3 configs, 14 cases (model 5 + poller 3 + listener 6).
**Twister dir:** `tests/unit/nfc_emv`

## Live HIL

- **Tag:** EMV contactless card
- **Golden:** Reader-captured `.card` (optional; synthetic goldens cover CI)
- **Overlay:** `overlay-pn7160-stack.conf` (reader); `+overlay-pn7160-listen.conf` (emulate)
- **Shell sequence:**
  ```
  nfc read tag1 → nfc stack stop → nfc emulate tag1
  nfc_transport stats  # apdu_assembled > 0
  ```
- **PASS criteria:** External reader receives valid PPSE response; SELECT AID → GPO → READ RECORD sequence completes

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. EMV via `nfc read`, `nfc emulate`, `nfc loop`.
