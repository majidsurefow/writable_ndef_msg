# protocols/felica — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

FeliCa protocol: block-based data model (IDm, PMm, system code, blocks) and poller (reader). **Clone-only**—no listener; FeliCa encryption keys are proprietary and not emulatable.

## Key files

| File | Owns |
|------|------|
| `felica.c` | Data model: `felica_t` struct, IDm/PMm/system code, block storage, serialize/deserialize |
| `felica.h` | Public types, model API, capacity symbols |
| `felica_poller.c` | Reader poller: NFC-F polling, block read |
| `felica_poller.h` | Poller API: `felica_poller_read` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_FELICA` | n | Gate model + poller |
| `FELICA_BLOCKS_MAX` | 64 | Max blocks in model |

## Deps

- **Upstream (calls):** None (standalone)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_store`

## Invariants & safety

- **Buffers:** Block index validated against `FELICA_BLOCKS_MAX`
- **Error propagation:** Functions return negative errno; auth failure → `-EACCES`

## Roles

- **Poller:** `felica_poller.c` (Tier B) — reader NFC-F
- **Listener:** **None** (clone-only)

## Wire/spec

- Sony FeliCa Lite-S datasheet; JIS X 6319-4
- **Emulatable:** No (clone-only; tech_mask gap + encryption)

## Capacity symbols

| CONFIG_* | Default | Bound it protects |
|----------|---------|-------------------|
| `FELICA_BLOCKS_MAX` | 64 | Block array size in model |

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/Felica.nfc`
- **Generated:** `flipper_nfc_to_fixture.py` → `tests/fixtures/felica/`
- **Loopback:** **SKIP** — clone-only; no native listener

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** No (not in emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_felica.model` | A (model) | `NFC_PROTOCOL_FELICA` | Serialize/deserialize, IDm/PMm parse |
| `sample.nfc.unit.nfc_felica.poller` | B (poller) | `+NFC_FELICA_TEST_TIER_POLLER` | NFC-F polling, block read |

**Tier counts:** 2 configs, 13 cases (model + poller).
**Twister dir:** `tests/unit/nfc_felica`

## Live HIL

- **Tag:** FeliCa Lite-S
- **Overlay:** `overlay-pn7160-stack.conf`
- **Shell sequence:**
  ```
  nfc scan start → nfc scan stop → nfc read tag1
  ```
- **PASS criteria:** IDm extracted; `nfc emulate` refused (clone-only)
- **Known gap:** `tech_mask` NFC-F discovery deferred in PN7160 driver

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. FeliCa read via `nfc read`; emulate refused by policy applet.
