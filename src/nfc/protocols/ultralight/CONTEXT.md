# protocols/ultralight — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

MIFARE Ultralight / NTAG protocol: page-based data model (UID, pages, OTP, counter, signature), serialize/deserialize, poller (reader page reads), and T4T NDEF adapter listener. Does NOT implement native Type-2 emulation—card emulation wraps Ultralight data in NDEF-T4T responses.

## Key files

| File | Owns |
|------|------|
| `ultralight.c` | Data model: `ultralight_t` struct, page storage, serialize/deserialize, NTAG subtype detection |
| `ultralight.h` | Public types, model API, capacity symbols, NTAG subtype enum |
| `ultralight_poller.c` | Reader poller: NTAG READ/FAST_READ page commands |
| `ultralight_poller.h` | Poller API: `ultralight_poller_read` |
| `ultralight_listener.c` | T4T NDEF adapter: wraps page data as NDEF for emulation |
| `ultralight_listener.h` | Listener API: `ultralight_listener_load` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_ULTRALIGHT` | n | Gate model + poller + listener |
| `NFC_ULTRALIGHT_MAX_PAGES` | 256 | Max pages (NTAG216 = 231); bounds page index |

## Deps

- **Upstream (calls):** `NFC_PROTOCOL_NDEF` (select dependency for listener)
- **Downstream (called by):** `nfc_reader` (poller registry), `ndef_listener` (T4T adapter), `nfc_store`

## Invariants & safety

- **Buffers:** Page index validated against `NFC_ULTRALIGHT_MAX_PAGES`; oversize → `-EINVAL`
- **Concurrency:** Poller single-flight via reader engine
- **Error propagation:** Functions return negative errno

## Roles

- **Poller:** `ultralight_poller.c` (Tier B) — reader NFC-A, READ; NTAG21x PWD_AUTH (`0x1B`) before protected pages; partial read stops at AUTH0 when auth fails (Flipper parity: `Ntag213_locked` → `pages_read=4`)
- **Listener:** `ultralight_listener.c` (Tier C) — T4T NDEF adapter (not native T2)

## Deferred

- **Ultralight C 3DES** (`0x1A`/`0xAF`): not implemented; AUTH probe steps deferred
- **Native T2 RF harness** (`nfc_t2t_lib`): E+ loopback remains NDEF T4 adapter

## Wire/spec

- NXP MIFARE Ultralight EV1 / NTAG213/215/216 datasheets
- **Emulatable:** Yes (via NDEF T4T adapter; no native T2)

## Capacity symbols

| CONFIG_NFC_ULTRALIGHT_* | Default | Bound it protects |
|-------------------------|---------|-------------------|
| `MAX_PAGES` | 256 | Page array size in model |

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/Ultralight_11.nfc`, `Ultralight_21.nfc`, `Ultralight_C.nfc`, `Ntag213_locked.nfc`, `Ntag215.nfc`, `Ntag216.nfc`
- **Generated:** `flipper_nfc_to_fixture.py --all --card-bin` → `tests/fixtures/ultralight/`
- **Loopback:** Via ndef listener (T4T adapter)

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset via T4T)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_ultralight.model` | A (model) | `NFC_PROTOCOL_ULTRALIGHT` | Serialize/deserialize, page bounds, NTAG subtype |
| `sample.nfc.unit.nfc_ultralight.poller` | B (poller) | `+NFC_ULTRALIGHT_TEST_TIER_POLLER` | Poller page read, FAST_READ batching |

**Tier counts:** 2 configs, 32 cases (model + poller; listener via NDEF).
**Twister dir:** `tests/unit/nfc_ultralight`

## Live HIL

- **Tag:** NTAG213/215/216, Ultralight EV1
- **Overlay:** `overlay-pn7160-stack.conf`
- **Shell sequence:**
  ```
  nfc scan start → nfc scan stop → nfc read tag1
  nfc store dump tag1  # inspect page data
  ```
- **PASS criteria:** UID extracted, pages read (compare to Flipper `.nfc` if available)

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. Ultralight read via `nfc read`; emulate via T4T adapter (`nfc emulate`).
