# protocols/slix — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

NXP SLIX (ISO 15693-3 extension) protocol: data model extensions (capabilities, passwords, signature) and poller (reader). **Clone-only** on NFCT; **virtual listener** for Tier C / E+ loopback with password unlock (`0xB2`/`0xB3`).

## Key files

| File | Owns |
|------|------|
| `slix.c` | Data model, serialize/deserialize, `slix_compare` |
| `slix_poller.c` | Reader poller atop ISO15693-3 parent |
| `slix_poller_i.c` | Addressed mfg frames, `get_random` / `set_password` |
| `slix_listener.c` | Virtual SLIX tag (parent + mfg commands) |

## Fixtures ↔ goldens

- **Flipper:** `Slix_cap_default.nfc`, `Slix_cap_missed.nfc`, `Slix_cap_accept_all_pass.nfc`
- **Generated:** `tests/fixtures/slix/` + `tests/fixtures/store/*.card.bin`
- **Loopback:** E+ `test_virtual_loopback_slix` with `slix_compare` deep compare

## Tests that prove it

| Test | Tier | Proves |
|------|------|--------|
| `sample.nfc.unit.nfc_slix.model` | A | 3 CAP variants serialize roundtrip |
| `sample.nfc.unit.nfc_slix.poller` | B | 87-TX read (parent + AB/BD) |
| `sample.nfc.unit.nfc_slix.listener` | C | Parent + password unlock loopback |
| `sample.nfc.unit.nfc_reader.store` | E | 5× SLIX store roundtrip |
| `sample.nfc.unit.nfc_reader.store` | E+ | `test_virtual_loopback_slix` |

**Twister dir:** `tests/unit/nfc_slix`

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Virtual listener only
