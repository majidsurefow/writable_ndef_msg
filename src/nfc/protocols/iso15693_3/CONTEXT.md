# protocols/iso15693_3 — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

ISO 15693-3 (NFC-V / Vicinity) protocol: block-based data model (UID, DSFID, AFI, blocks) and poller (reader). **Clone-only** on NFCT product path; **virtual listener** for Tier C / E+ QEMU loopback. Parent protocol for SLIX extensions.

## Key files

| File | Owns |
|------|------|
| `iso15693_3.c` | Data model, serialize/deserialize, `iso15693_3_compare` |
| `iso15693_3_poller.c` | Reader poller: inventory, sysinfo, read, security batch |
| `iso15693_3_poller_i.c` | CRC, response parsers |
| `iso15693_3_listener.c` | Virtual listener for loopback / PN7160 listen path |

## Roles

- **Poller:** `iso15693_3_poller.c` (Tier B)
- **Listener:** `iso15693_3_listener.c` (Tier C virtual tag; product emulate `-ENOTSUP` on NFCT)

## Fixtures ↔ goldens

- **Flipper:** Parent section from `Slix_cap_*.nfc` → `tests/fixtures/iso15693_3/`
- **Generated:** 85-step TX/RX goldens, `.card.bin` via `flipper_nfc_to_fixture.py --card-bin`
- **Loopback:** E+ via SLIX child listener wrapping parent commands

## Tests that prove it

| Test | Tier | Proves |
|------|------|--------|
| `sample.nfc.unit.nfc_iso15693_3.model` | A | Serialize/deserialize roundtrip |
| `sample.nfc.unit.nfc_iso15693_3.poller` | B | 85-TX parent read, CRC wire shape |
| `sample.nfc.unit.nfc_iso15693_3.listener` | C | Inventory/sysinfo/read virtual responses |

**Twister dir:** `tests/unit/nfc_iso15693_3`

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Virtual listener only (PN7160 stack-ready)
