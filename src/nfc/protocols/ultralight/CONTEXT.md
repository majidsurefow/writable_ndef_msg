# protocols/ultralight — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status:** SHIPPED

## Purpose

**MIFARE Ultralight / NTAG Type-2 poll path** — page-based data model, `ultralight_poller.c` (READ/FAST_READ, GET_VERSION, READ_SIG/CNT/TEARING, PWD_AUTH, Ultralight C 3DES `0x1A`/`0xAF`), serialize/deserialize. **Native Type-2 RF listener** (`ultralight_listener_t2.c`) for virtual loopback; T4 NDEF adapter for product CE.

**T2 NDEF:** CC on page 3, NDEF TLV from page 4+. Flipper `supported_cards/ndef.c` **`NDEF_PROTO_UL`** is **content-parse reference only** (TLV/message bytes after pages are in the model) — not a wire poller golden.

**Emulate handoff:** `ultralight_listener.c` (T4 adapter) extracts NDEF TLV from stored pages → `ndef_service_set_content()` → **`ndef_listener.c`** responds to ISO-DEP SELECT/READ BINARY. Full routing: [`NDEF_T2_T4_ROUTING.md`](../../../../docs/nfc/NDEF_T2_T4_ROUTING.md); T4 wire spec: [`../ndef/CONTEXT.md`](../ndef/CONTEXT.md).

## Key files

| File | Owns |
|------|------|
| `ultralight.c` | Data model: `ultralight_t` struct, page storage, serialize/deserialize, NTAG subtype detection |
| `ultralight.h` | Public types, model API, capacity symbols, NTAG subtype enum |
| `ultralight_poller.c` | Reader poller: NTAG READ/FAST_READ page commands |
| `ultralight_poller_i.c` | Wire helpers, Ultralight C 3DES auth (`0x1A`/`0xAF`), PWD_AUTH |
| `ultralight_poller.h` | Poller API: `ultralight_poller_read` |
| `ultralight_3des.c` / `ultralight_des.c` | 3DES-2KEY CBC (Flipper parity, no mbedtls dep in unit tests) |
| `ultralight_listener_t2.c` | Native Type-2 listener for virtual loopback |
| `ultralight_listener.c` | T4 NDEF adapter: pages → NDEF TLV → handoff to `ndef_listener` |
| `ultralight_listener.h` | Listener API: `ultralight_listener_load` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_ULTRALIGHT` | n | Gate model + poller + listener |
| `NFC_ULTRALIGHT_MAX_PAGES` | 256 | Max pages (NTAG216 = 231); bounds page index |

## Deps

- **Upstream (calls):** `NFC_PROTOCOL_NDEF` (select dependency for T4 listener adapter)
- **Downstream (called by):** `nfc_reader` (poller registry), `ndef_listener` (T4T adapter), `nfc_store`

## Invariants & safety

- **Buffers:** Page index validated against `NFC_ULTRALIGHT_MAX_PAGES`; oversize → `-EINVAL`
- **Concurrency:** Poller single-flight via reader engine
- **Error propagation:** Functions return negative errno

## Roles

- **Poller:** `ultralight_poller.c` + `ultralight_poller_i.c` (Tier B) — reader NFC-A, READ/FAST_READ; GET_VERSION; READ_SIG/CNT/TEARING; NTAG21x PWD_AUTH (`0x1B`); Ultralight C 3DES auth before protected pages; partial read stops at AUTH0 when auth fails (Flipper parity: `Ntag213_locked` → `pages_read=4`)
- **Listener:** `ultralight_listener_t2.c` — native T2 for E+ loopback; `ultralight_listener.c` — T4 adapter for product CE

## Wire/spec

- NXP MIFARE Ultralight EV1 / NTAG213/215/216 datasheets
- **Emulatable:** Yes (native T2 listener + T4 NDEF adapter)

## Capacity symbols

| CONFIG_NFC_ULTRALIGHT_* | Default | Bound it protects |
|-------------------------|---------|-------------------|
| `MAX_PAGES` | 256 | Page array size in model |

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/Ultralight_11.nfc`, `Ultralight_21.nfc`, `Ultralight_C.nfc`, `Ntag213_locked.nfc`, `Ntag215.nfc`, `Ntag216.nfc`
- **Generated:** `flipper_nfc_to_fixture.py --card-bin` → `tests/fixtures/ultralight/` (mock headers, TX goldens, `.card.bin`)
- **Loopback:** Tier E+ via native T2 listener + `ultralight_compare` deep compare; cookbook [§5.2](../../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md#52-mf_ultralight-type-2-ndef-poll-path)

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (T4T adapter + native T2 loopback tag)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_ultralight.model` | A (model) | `NFC_PROTOCOL_ULTRALIGHT` | Serialize/deserialize, page bounds, NTAG subtype |
| `sample.nfc.unit.nfc_ultralight.poller` | B (poller) | `+NFC_ULTRALIGHT_TEST_TIER_POLLER` | Poller page read, TX goldens all 6 fixtures, Ultralight C auth |
| `nfc_reader` ultralight store + loopback | E+ | `NFC_PROTOCOL_ULTRALIGHT` | `.card.bin` roundtrip, poller↔T2 listener deep compare |

**Twister dir:** `tests/unit/nfc_ultralight`, `tests/unit/nfc_reader`

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
