# protocols/slix — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

NXP SLIX (ISO 15693-3 extension) protocol: data model extensions (capabilities, password protection) and poller (reader). **Clone-only**—inherits ISO15693-3 limitation. Child protocol of ISO15693-3.

## Key files

| File | Owns |
|------|------|
| `slix.c` | Data model: `slix_t` struct (extends iso15693_3_t), SLIX capabilities, serialize/deserialize |
| `slix.h` | Public types, model API, capability flags |
| `slix_poller.c` | Reader poller: SLIX-specific commands atop ISO15693-3 |
| `slix_poller.h` | Poller API: `slix_poller_read` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_SLIX` | n | Gate model + poller; selects `NFC_PROTOCOL_ISO15693_3` |

## Deps

- **Upstream (calls):** `NFC_PROTOCOL_ISO15693_3` (parent, selected)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_store`

## Invariants & safety

- **Buffers:** Inherits ISO15693-3 bounds
- **Error propagation:** Functions return negative errno; password failure → `-EACCES`

## Roles

- **Poller:** `slix_poller.c` (Tier B) — reader NFC-V SLIX
- **Listener:** **None** (clone-only)

## Wire/spec

- NXP ICODE SLIX/SLIX2 datasheet; ISO 15693-3 base
- **Emulatable:** No (clone-only)

## Capacity symbols

Inherits from `ISO15693_BLOCKS_MAX`, `ISO15693_BLOCK_SIZE_MAX`.

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/Slix_cap_default.nfc`, `Slix_cap_missed.nfc`, `Slix_cap_accept_all_pass.nfc`
- **Generated:** `flipper_nfc_to_fixture.py` → `tests/fixtures/slix/`
- **Loopback:** **SKIP** — clone-only; no native listener

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** No (not in emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_slix.model` | A (model) | `NFC_PROTOCOL_SLIX` | Serialize/deserialize, capability parsing |
| `sample.nfc.unit.nfc_slix.poller` | B (poller) | `+NFC_SLIX_TEST_TIER_POLLER` | SLIX inventory, block read |

**Tier counts:** 2 configs, 18 cases (model + poller).
**Twister dir:** `tests/unit/nfc_slix`

## Live HIL

- **Tag:** NXP ICODE SLIX/SLIX2
- **Overlay:** `overlay-pn7160-stack.conf`
- **Shell sequence:**
  ```
  nfc scan start → nfc scan stop → nfc read tag1
  nfc store dump tag1  # inspect SLIX capabilities
  ```
- **PASS criteria:** UID + capabilities extracted; `nfc emulate` refused (clone-only)

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. SLIX read via `nfc read`; emulate refused by policy applet.
