# protocols/ultralight — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

**MIFARE Ultralight / NTAG Type-2 poll path** — page-based data model, `ultralight_poller.c` (READ/FAST_READ), serialize/deserialize. **No native Type-2 RF listener in v1.**

**T2 NDEF:** CC on page 3, NDEF TLV from page 4+. Flipper `supported_cards/ndef.c` **`NDEF_PROTO_UL`** is **content-parse reference only** (TLV/message bytes after pages are in the model) — not a wire poller golden.

**Emulate handoff:** `ultralight_listener.c` (T4 adapter) extracts NDEF TLV from stored pages → `ndef_service_set_content()` → **`ndef_listener.c`** responds to ISO-DEP SELECT/READ BINARY. Full routing: [`NDEF_T2_T4_ROUTING.md`](../../../../docs/nfc/NDEF_T2_T4_ROUTING.md); T4 wire spec: [`../ndef/CONTEXT.md`](../ndef/CONTEXT.md).

## Key files

| File | Owns |
|------|------|
| `ultralight.c` | Data model: `ultralight_t` struct, page storage, serialize/deserialize, NTAG subtype detection |
| `ultralight.h` | Public types, model API, capacity symbols, NTAG subtype enum |
| `ultralight_poller.c` | Reader poller: NTAG READ/FAST_READ page commands |
| `ultralight_poller.h` | Poller API: `ultralight_poller_read` |
| `ultralight_listener.c` | T4 NDEF adapter: pages → NDEF TLV → handoff to `ndef_listener` |
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
- **Listener:** `ultralight_listener.c` — T4 adapter only; Tier C pending F1 (not in `nfc_ultralight/` — emulate via `nfc_ndef` listener + adapter)

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
- **Loopback:** Tier E+ via T4 adapter → `ndef_listener` (not `nfc_ndef` poller on raw pages); cookbook [§5.2](../../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md#52-mf_ultralight-type-2-ndef-poll-path)

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
