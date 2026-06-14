# protocols/desfire — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

MIFARE DESFire protocol: application/file-based data model (PICC info, applications, files), serialize/deserialize, keyless poller (reader partial clone), and ISO-DEP listener (card emulation). **Partially emulatable** — public file contents with free-read access (`0x0E`) are captured and emulated; encrypted/MAC comm and AES auth are not replayed.

## Key files

| File | Owns |
|------|------|
| `desfire.c` | Data model: `desfire_data_t`, PICC info, app/file trees, serialize/deserialize |
| `desfire_poller.c` | Keyless poller SM: PICC → apps → files (standard/backup/value/record) |
| `desfire_poller_i.c` | APDU transceive, `0x91AF` chunking, READ_DATA/GET_VALUE/READ_RECORDS |
| `desfire_listener.c` | ISO-DEP listener: PICC/app/file commands, VALUE/RECORD file types |

## Auth / capture policy

| Path | Behavior |
|------|----------|
| **Keyless golden** | `MfDesfire_EV1_sample.nfc`: access `0E 0E`, no keys in model; poller reads app/file payloads without AUTH |
| **Auth required** | Master/app key settings or file access ≠ free → poller skips with log; listener returns `91 AE` |
| **Crypto auth** | Listener stub: AUTH step1 only when `master_key` non-zero; no full AES session replay |

Flipper poller never authenticates; our poller matches that keyless path for the sample fixture.

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/MfDesfire_EV1_sample.nfc` → `tests/fixtures/desfire/` + `tests/fixtures/store/MfDesfire_EV1_sample.card.bin`
- **Generated:** `flipper_nfc_to_fixture.py --input …/MfDesfire_EV1_sample.nfc --card-bin`
- **Loopback:** E+ `test_virtual_loopback_desfire` with `desfire_compare` (UID, PICC, app/file payloads)

## Tests that prove it

| Test | Tier | Proves |
|------|------|--------|
| `sample.nfc.unit.nfc_desfire.model` | A | Serialize/deserialize, app/file bounds |
| `sample.nfc.unit.nfc_desfire.poller` | B | 13-TX keyless read (PICC + app + file) |
| `sample.nfc.unit.nfc_desfire.listener` | C | SELECT, READ_DATA, GET_VALUE, READ_RECORDS |
| `sample.nfc.unit.nfc_reader.store` | E | DESFire store roundtrip |
| `sample.nfc.unit.nfc_reader.store` | E+ | `test_virtual_loopback_desfire` deep compare |

**Twister dir:** `tests/unit/nfc_desfire`, `tests/unit/nfc_reader`

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (public files; no encrypted auth replay)

## Live HIL

- **Tag:** MIFARE DESFire EV1/EV2
- **Golden:** Reader-captured or Flipper `.nfc` → `.card.bin`
- **PASS criteria:** External reader SELECT app/file; emulated responses match stored public payloads
