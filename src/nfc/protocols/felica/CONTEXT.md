# protocols/felica — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

FeliCa protocol: block-based data model (IDm, PMm, system code, blocks), poller (reader), and virtual listener for QEMU loopback. **Clone-only on product NFCT** — FeliCa encryption keys are proprietary; product emulate returns `-ENOTSUP`.

## Key files

| File | Owns |
|------|------|
| `felica.c` | Data model: `felica_data_t`, serialize/deserialize, `felica_compare` |
| `felica.h` | Public types, model API, capacity symbols |
| `felica_poller.c` | Reader poller: NFC-F polling, Lite block read SM |
| `felica_poller.h` | Poller API: `felica_poller_detect`, `felica_poller_read` |
| `felica_poller_i.c` | Wire framing: poll, read_blocks, CRC transceive |
| `felica_listener.c` | Virtual listener: poll + READ_WITHOUT_ENCRYPTION for loopback |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_FELICA` | n | Gate model + poller + virtual listener |
| `FELICA_BLOCKS_MAX` | 64 | Max blocks in model |

## Deps

- **Upstream (calls):** `felica_crc` helper
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_store`

## Invariants & safety

- **Buffers:** Block index validated against `FELICA_BLOCKS_MAX`
- **Error propagation:** Functions return negative errno; auth failure → `-EACCES`
- **Workflow:** PMm byte[1] `0xF1` → Lite read path; Standard SM not required for golden `Felica.nfc`

## Roles

- **Poller:** `felica_poller.c` (Tier B) — reader NFC-F, Lite `ReadLiteBlocks` (29 transceives)
- **Listener:** `felica_listener.c` (Tier C) — virtual tag for unit/E+ loopback only

## Wire/spec

- Sony FeliCa Lite-S datasheet; JIS X 6319-4
- **Product emulate:** No (`nfc_applet_check_emulate` → `-ENOTSUP`)
- **Virtual emulate:** Yes (QEMU loopback via `felica_listener` + `nfc_virtual_rf`)

## Capacity symbols

| CONFIG_* | Default | Bound it protects |
|----------|---------|-------------------|
| `FELICA_BLOCKS_MAX` | 64 | Block array size in model |

## Fixtures ↔ goldens

- **Flipper:** `tests/fixtures/nfc/flipper/Felica.nfc`
- **Generated:** `tests/fixtures/felica/` — `Felica_mock.h`, `Felica_framed.inc`, `Felica.card.bin`
- **Store envelope:** `tests/fixtures/store/Felica_card.inc`
- **Loopback:** E+ `test_virtual_loopback_felica` with `felica_compare` deep compare

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** No (not in product emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_felica.model` | A (model) | `NFC_PROTOCOL_FELICA` | Serialize/deserialize, IDm/PMm parse |
| `sample.nfc.unit.nfc_felica.poller` | B (poller) | `+NFC_FELICA_TEST_TIER_POLLER` | 29-TX poll + Lite read, CRC fail |
| `sample.nfc.unit.nfc_felica.listener` | C (listener) | `+NFC_FELICA_TEST_TIER_LISTENER` | Poll/read framed responses |
| `nfc_reader_felica_store` | E (store) | `CONFIG_NFC_PROTOCOL_FELICA` | Golden `.card.bin` load/save |
| `test_virtual_loopback_felica` | E+ (loopback) | same | Poller ↔ listener deep compare |

**Twister dir:** `tests/unit/nfc_felica` + `tests/unit/nfc_reader`

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
