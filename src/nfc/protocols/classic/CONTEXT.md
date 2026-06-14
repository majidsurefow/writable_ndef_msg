# protocols/classic — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

MIFARE Classic protocol: block-based data model (UID, sectors/blocks, access bits), Crypto1 authentication, and poller (reader). **Clone-only**—no listener; NFCT cannot emulate the proprietary Crypto1 cipher.

## Key files

| File | Owns |
|------|------|
| `classic.c` | Data model: `classic_t` struct, sector/block storage, serialize/deserialize |
| `classic.h` | Public types, model API, capacity symbols |
| `classic_i.c` | Internal helpers: sector→block mapping, access bits parsing |
| `classic_i.h` | Internal API |
| `classic_poller.c` | Reader poller: anticollision, Crypto1 auth, block read |
| `classic_poller.h` | Poller API: `classic_poller_read` |
| `crypto1.c` | Crypto1 cipher implementation (LFSR, filter) |
| `crypto1.h` | Crypto1 API |
| `iso14443_crc.c` | ISO 14443-3A CRC-A implementation |
| `iso14443_crc.h` | CRC-A API |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_CLASSIC` | n | Gate model + poller + crypto |
| `NFC_CLASSIC_BLOCK_COUNT` | 256 | Max blocks (Classic 4K = 256) |
| `NFC_CLASSIC_TEST_DETERMINISTIC_NR` | n | Fixed NR bytes for unit test fixtures |

## Deps

- **Upstream (calls):** None (standalone)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_store`

## Invariants & safety

- **Buffers:** Block index validated against `NFC_CLASSIC_BLOCK_COUNT`
- **Crypto:** Crypto1 LFSR state isolated per session
- **Error propagation:** Auth failure → `-EACCES`; block out-of-range → `-EINVAL`

## Roles

- **Poller:** `classic_poller.c` (Tier B) — reader NFC-A, Crypto1 auth
- **Listener:** **None** (clone-only)

## Wire/spec

- NXP MIFARE Classic EV1 datasheet; ISO 14443-3A
- **Emulatable:** No (clone-only; Crypto1 not emulatable via NFCT)

## Capacity symbols

| CONFIG_NFC_CLASSIC_* | Default | Bound it protects |
|----------------------|---------|-------------------|
| `BLOCK_COUNT` | 256 | Block array size in model |

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/MfClassic_1K_4b.nfc` — generator golden
- **Generated:** `flipper_nfc_to_fixture.py` → `tests/fixtures/classic/`
- **Loopback:** **SKIP** — clone-only; no native listener

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** No (not in emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_classic.model` | A (model) | `NFC_PROTOCOL_CLASSIC` | Serialize/deserialize, sector mapping, access bits |
| `sample.nfc.unit.nfc_classic.poller` | B (poller) | `+NFC_CLASSIC_TEST_TIER_POLLER` | Anticollision, Crypto1 auth, block read (deterministic NR) |

**Tier counts:** 2 configs, 17 cases (model + poller).
**Twister dir:** `tests/unit/nfc_classic`

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
