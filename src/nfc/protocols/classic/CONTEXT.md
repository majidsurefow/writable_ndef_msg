# protocols/classic — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

MIFARE Classic protocol: block-based data model (UID, sectors/blocks, access bits), Crypto1 authentication, poller (reader), and virtual listener for QEMU loopback. **Clone-only** on NFCT product — Crypto1 not emulated on hardware CE.

## Key files

| File | Owns |
|------|------|
| `classic.c` | Data model: `classic_data_t`, serialize/deserialize, `classic_compare` |
| `classic.h` | Public types, model API, capacity symbols |
| `classic_i.c` | Internal helpers: sector→block mapping, access bits parsing |
| `classic_i.h` | Internal API |
| `classic_poller.c` | Reader poller: anticollision, Crypto1 auth, block read |
| `classic_poller.h` | Poller API: `classic_poller_detect`, `classic_poller_read` |
| `classic_listener.c` | Virtual tag listener (Crypto1 auth + encrypted READ) |
| `classic_listener.h` | Listener init/shutdown, `classic_listener_get()` |
| `crypto1.c` | Crypto1 cipher implementation (LFSR, filter) |
| `crypto1.h` | Crypto1 API |
| `iso14443_crc.c` | ISO 14443-3A CRC-A implementation |
| `iso14443_crc.h` | CRC-A API |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_CLASSIC` | n | Gate model + poller + listener + crypto |
| `NFC_CLASSIC_BLOCK_COUNT` | 256 | Max blocks (Classic 4K = 256) |
| `NFC_CLASSIC_TEST_DETERMINISTIC_NR` | n | Fixed NR bytes for unit test fixtures |

## Deps

- **Upstream (calls):** None (standalone)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_store`

## Invariants & safety

- **Buffers:** Block index validated against tag type and `CLASSIC_BLOCKS_IN_MODEL`
- **Crypto:** Crypto1 LFSR state isolated per session
- **Error propagation:** Auth failure → `-EACCES`; block out-of-range → `-EINVAL`

## Roles

- **Poller:** `classic_poller.c` (Tier B) — reader NFC-A, Crypto1 auth
- **Listener:** `classic_listener.c` (Tier C) — virtual loopback tag; NFCT product emulate `-ENOTSUP`

## Wire/spec

- NXP MIFARE Classic EV1 datasheet; ISO 14443-3A
- **Emulatable:** Virtual listener only (QEMU); NFCT clone-only

## Capacity symbols

| CONFIG_NFC_CLASSIC_* | Default | Bound it protects |
|----------------------|---------|-------------------|
| `BLOCK_COUNT` | 256 | Kconfig cap on `classic_data_t.blocks[] (CLASSIC_BLOCKS_IN_MODEL)` |

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/MfClassic_1K_4b.nfc` — generator golden
- **Generated:** `flipper_nfc_to_fixture.py` → `tests/fixtures/classic/` + `tests/fixtures/store/MfClassic_1K_4b.card.bin`
- **Loopback:** E+ `test_virtual_loopback_classic` with `classic_compare` deep compare

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** No (not in emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_classic.model` | A (model) | `NFC_PROTOCOL_CLASSIC` | Serialize/deserialize, sector mapping, access bits |
| `sample.nfc.unit.nfc_classic.poller` | B (poller) | `+NFC_CLASSIC_TEST_TIER_POLLER` | 98-TX full assert, Crypto1 auth, block read |
| `sample.nfc.unit.nfc_classic.listener` | C (listener) | `+NFC_CLASSIC_TEST_TIER_LISTENER` | Crypto1 auth NT/NR|AR, virtual RF poller read |
| `nfc_reader_classic_store.*` | E (store) | `CONFIG_NFC_PROTOCOL_CLASSIC` | `.card.bin` envelope load/save |
| `nfc_reader_loopback.test_virtual_loopback_classic` | E+ | same | Full poller→listener→compare |

**Tier counts:** 3 configs (model + poller + listener), 22+ cases.
**Twister dir:** `tests/unit/nfc_classic`, `tests/unit/nfc_reader`

### Cookbook §14.3 poller cases

`test_detect_auth_probe`, `test_detect_enotsup_short_rx`, `test_detect_eio`, `test_detect_timeout`, `test_detect_inactive_session`, `test_crypto_decrypt_block0_golden`, `test_crypto_decrypt_block4_golden`, `test_read_1k_golden`, `test_read_tx_sequence` (98 TX), `test_read_truncated`, `test_read_eio`, `test_read_timeout`, `test_read_no_session_end`, `test_read_inactive_session`

## Live HIL

- **Tag:** MIFARE Classic 1K/4K
- **Overlay:** `overlay-pn7160-stack.conf`
- **Shell sequence:**
  ```
  nfc scan start → nfc scan stop → nfc read tag1
  nfc store dump tag1  # inspect block data
  ```
- **PASS criteria:** UID extracted, blocks read with auth; `nfc emulate` refused (clone-only warning)

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. Classic read via `nfc read`; emulate refused by policy applet.
