# protocols/iso15693_3 — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

ISO 15693-3 (NFC-V / Vicinity) protocol: block-based data model (UID, DSFID, AFI, blocks) and poller (reader). **Clone-only**—no listener; NFC-V operates at different coupling than NFCT supports. Parent protocol for SLIX extensions.

## Key files

| File | Owns |
|------|------|
| `iso15693_3.c` | Data model: `iso15693_3_t` struct, UID, block storage, serialize/deserialize |
| `iso15693_3.h` | Public types, model API, capacity symbols |
| `iso15693_3_poller.c` | Reader poller: NFC-V inventory + block read |
| `iso15693_3_poller.h` | Poller API: `iso15693_3_poller_read` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_ISO15693_3` | n | Gate model + poller |
| `ISO15693_BLOCKS_MAX` | 256 | Max blocks in model |
| `ISO15693_BLOCK_SIZE_MAX` | 32 | Max block size (bytes) |

## Deps

- **Upstream (calls):** None (standalone)
- **Downstream (called by):** `nfc_reader` (poller registry), `NFC_PROTOCOL_SLIX` (child), `nfc_store`

## Invariants & safety

- **Buffers:** Block index validated against `ISO15693_BLOCKS_MAX`; block size against `ISO15693_BLOCK_SIZE_MAX`
- **Error propagation:** Functions return negative errno

## Roles

- **Poller:** `iso15693_3_poller.c` (Tier B) — reader NFC-V
- **Listener:** **None** (clone-only)

## Wire/spec

- ISO/IEC 15693-3 Vicinity Integrated Circuit Cards
- **Emulatable:** No (clone-only; NFC-V not supported on NFCT)

## Capacity symbols

| CONFIG_ISO15693_* | Default | Bound it protects |
|-------------------|---------|-------------------|
| `BLOCKS_MAX` | 256 | Block array size in model |
| `BLOCK_SIZE_MAX` | 32 | Single block buffer |

## Fixtures ↔ goldens

- **Flipper:** No direct ISO15693-3 fixture (SLIX variants used)
- **Generated:** Via SLIX fixtures
- **Loopback:** **SKIP** — clone-only; no native listener

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** No (not in emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| — | — | — | No dedicated test dir; tested via `nfc_reader` + SLIX |

**Tier counts:** (via `nfc_slix` + `nfc_reader` suites)
**Twister dir:** No dedicated dir; parent protocol for SLIX

## Live HIL

- **Tag:** ISO 15693-3 compliant tag (e.g., ICODE SLIX)
- **Overlay:** `overlay-pn7160-stack.conf`
- **Shell sequence:**
  ```
  nfc scan start → nfc scan stop → nfc read tag1
  ```
- **PASS criteria:** UID extracted, blocks read; `nfc emulate` refused (clone-only)

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. ISO15693-3 read via `nfc read`; emulate refused by policy applet.
