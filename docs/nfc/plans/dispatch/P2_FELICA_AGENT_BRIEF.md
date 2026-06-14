# P2 — FeliCa TX Asserts + Framed Mocks — Agent Dispatch Brief

**Priority:** #2  
**Goal:** Port Flipper wire framing (CRC + Lite read SM), regenerate framed mocks, add Tier B TX asserts for golden `Felica.nfc`.  
**Do not dispatch P1 (Ultralight) — user has P1 running.**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper FeliCa poller?** | **YES** — full JIS X 6319-4 state machine in `flipperzero/lib/nfc/protocols/felica/` |
| **Local `.nfc` golden?** | **YES** — `tests/fixtures/nfc/flipper/Felica.nfc` (identical to Flipper unit-test copy) |
| **Can `.nfc` alone supply TX goldens?** | **NO** — block contents only; TX must be derived from Flipper C (`felica_poller_i.c` + Lite block-index table) or Python mirror |
| **Why "29 steps wrong wire"?** | Enabling `test_read_tx_sequence` against stub poller + RX-only mocks fails on **every** transceive (wrong bytes, wrong block indices, no CRC) |
| **Current tests pass?** | **Misleading** — stub poller ↔ RX-only mock are self-consistent; no Flipper wire parity |

---

## Exact files and functions to edit

### Product — transceive + CRC (prerequisite)

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Add** | `src/nfc/helpers/felica_crc.c` | `felica_crc_append()`, `felica_crc_check()`, `felica_crc_trim()` — port from `flipperzero/lib/nfc/helpers/felica_crc.c` |
| **Add** | `src/nfc/helpers/felica_crc.h` | Declarations |
| **Add** | `src/nfc/protocols/felica/felica_poller_i.c` | `felica_poller_frame_exchange()`, `felica_poller_polling()`, `felica_poller_read_blocks()`, `felica_poller_prepare_tx_buffer[_raw]()` |
| **Add** | `src/nfc/protocols/felica/felica_poller_i.h` | Internal structs: `FelicaCommandHeader`, block list elements |
| **Replace stub** | `src/nfc/protocols/felica/felica_poller.c` | `felica_poller_poll()` L37–58, `felica_poller_read_block()` L61–79, `felica_poller_read()` L107–143 |
| **Add workflow detect** | `src/nfc/protocols/felica/felica.c` | `felica_get_workflow_type()` — PMm byte[1] `0xF1` → Lite |
| **Wire CMake/Kconfig** | `src/nfc/protocols/felica/CMakeLists.txt`, parent CMake | Add new sources |

### Test infra

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Raise cap** | `tests/common/include/nfc_session_mock.h` L20 | `NFC_SESSION_MOCK_MAX_TX` **16 → ≥32** (29 transceives + headroom) |
| **Extend generator** | `scripts/nfc/flipper_nfc_to_fixture.py` | Add `felica_framed_read_steps()` — poll + 28 Lite reads with TX/RX pairs |
| **Regenerate** | `tests/fixtures/felica/Felica_mock.h`, optional `Felica_framed.inc` | Paired TX constants + framed RX |
| **Add TX tests** | `tests/unit/nfc_felica/src/test_poller.c` | `test_read_tx_sequence`, `test_detect_poll_tx_framed`, CRC-fail cases |
| **Docs** | `tests/fixtures/nfc/flipper/README.md` | Tier B framed mocks row |

### Flipper reference (read-only)

- `flipperzero/lib/nfc/protocols/felica/felica_poller.c` — `felica_poller_state_handler_read_lite_blocks()` ~L438–484
- `flipperzero/lib/nfc/protocols/felica/felica_poller_i.c` — `felica_poller_frame_exchange()` L18–49, `felica_poller_polling()` ~L52–94

---

## Wire format bytes / parity targets

### Golden card constants (`Felica.nfc`)

```
IDm:  29 9F FA 53 AB 75 87 6E
PMm:  00 F1 00 00 00 01 43 00   → FelicaLite (PMm[1]=0xF1)
Blocks total: 28 (dump index 0..27, NOT wire block numbers)
Steps: 29 (1 poll + 28 block reads)
```

### Poll TX (Flipper target — step 0)

```
[len=0x06][0x00][SYSCODE=FF FF][req=0x00][slot=0x00][CRC16×2]
Example prefix: 06 00 FF FF 00 00 + CRC
```

**Our stub today** (`felica_poller.c` L39, L85): `00 FF FF 00 00` (5 B, no len, no CRC)

### Poll RX (after CRC trim)

```
[len][0x01][IDm×8][PMm×8][request_data×2 optional]
Mock today: raw IDm+PMm 16 B only (no len/resp/CRC)
```

### Read TX (Lite path, per block — steps 1..28)

```
[len][0x06 READ_WITHOUT_ENCRYPTION][IDm×8][service_num=1][service_code=0B 00][block_count=1][block_list×2][CRC16×2]
Service: FELICA_SERVICE_RO_ACCESS = 0x000B
```

**Our stub today** (`felica_poller.c` L64): `{0x06, block_num}` (2 B)

### Lite wire block index sequence (NOT sequential 0..27)

```
0..14, jump 15→0x80, read 0x80..0x88, jump 0x89→0x90, read 0x90..0x92, jump 0x93→0xA0, read 0xA0
```

**Our stub today** (`felica_poller.c` L127–137): sequential `block = 0..FELICA_BLOCKS_PROBE-1`

### Read RX (Flipper parse target)

```
[len][0x07][IDm×8][SF1][SF2][block_count][data×16×N]
Mock today: raw 16 B data (SF stripped from `.nfc`)
```

### CRC

- Poly `0x1021`, init `0`, 2-byte trailing on every frame, byte-swapped on wire (`flipperzero/lib/nfc/helpers/felica_crc.c`)

---

## Current bugs (file:line)

| Bug | Location | Impact |
|-----|----------|--------|
| No length byte, no CRC on TX/RX | `src/nfc/protocols/felica/felica_poller.c` L39, L64, L85 | Not Flipper-wire-compatible |
| Sequential block probe vs Lite jumps | `felica_poller.c` L127–137 | Wrong block numbers on air |
| SF1/SF2 hardcoded zero | `felica_poller.c` L133–134 | Ignores response header semantics |
| No `felica_crc` in `src/` | (missing) | Cookbook §5.7 gap |
| RX-only mocks, no TX goldens | `tests/fixtures/felica/Felica_mock.h` | Cannot assert wire |
| TX log cap 16 < 29 steps | `tests/common/include/nfc_session_mock.h` L20 | Blocks full TX assert |
| Tests assert TX count only, no bytes | `tests/unit/nfc_felica/src/test_poller.c` L36 | No `assert_tx_equals` |
| No `test_read_tx_sequence` | `tests/unit/nfc_felica/src/test_poller.c` | Cookbook §14.3 gap |

---

## DO NOT touch

- `src/nfc/protocols/ultralight/**` — **P1 in flight**
- FeliCa listener / Tier C — clone-only policy; no `felica_listener.c`
- `NFC_TECH_TYPE3_FELICA` HAL / PN7160 NFC-F discovery — F4/F5 backlog
- Standard FeliCa SM (system list, auth, external DES-MAC) — defer unless golden card changes
- Tier E+ virtual loopback for FeliCa — explicitly **SKIP** per harmonization
- Flipper submodule / upstream files — read-only reference
- `tests/unit/nfc_reader/src/test_virtual_loopback.c` FeliCa (none exists)

---

## Staged commits (exact message templates)

### Commit 1 — CRC helper

```
nfc/felica: add felica_crc helper ported from Flipper

Port CRC-16 (poly 0x1021) append/check/trim for JIS X 6319-4 framing.
Behavioral reference: flipperzero/lib/nfc/helpers/felica_crc.c.
```

### Commit 2 — Poller internal layer

```
nfc/felica: add felica_poller_i transceive and frame builders

Add frame_exchange, polling, and read_blocks builders with CRC.
Wire through nfc_reader_session_transceive().
```

### Commit 3 — Lite read state machine

```
nfc/felica: replace stub poller with Lite ReadLiteBlocks path

Detect FelicaLite from PMm, replay non-sequential block indices for
golden Felica.nfc (29 transceives).
```

### Commit 4 — Mock infra + generator

```
tests/felica: raise mock TX cap and add framed fixture generator

Extend flipper_nfc_to_fixture.py with felica_framed_read_steps();
regenerate Felica_mock.h; NFC_SESSION_MOCK_MAX_TX >= 32.
```

### Commit 5 — Tier B TX asserts

```
tests/felica: add test_read_tx_sequence and poll TX golden asserts

Assert all 29 TX frames against Python-generated constants (cookbook §14.3).
```

### Commit 6 — Docs

```
docs/nfc: mark FeliCa Tier B framed mocks done in flipper README

Note pre-CRC TX emission in cookbook §5.7 (mirror §5.8 ISO15693 pattern).
```

---

## Twister verify commands

Run from NCS root with `REPO=/Users/majidfaroud/writable_ndef_msg`:

```bash
# After each product commit (build smoke)
west twister -T "$REPO/tests/unit/nfc_felica" -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Full P2 gate (all FeliCa configs)
west twister -T "$REPO/tests/unit/nfc_felica" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Regression — reader store roundtrip (Tier E, no loopback)
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.store -v

# CI parity (optional full unit matrix slice)
west twister -T "$REPO/tests/unit/nfc_felica" -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v
```

**Pass criteria:** `sample.nfc.unit.nfc_felica.model` + `sample.nfc.unit.nfc_felica.poller` green; new `test_read_tx_sequence` passes; no regressions in `nfc_reader` store suite.

---

## Caller sweep checklist

After poller API/signature changes, grep and verify callers:

- [ ] `src/nfc/reader/nfc_reader_poller_registry.c` — `felica_poller_detect`, `felica_poller_read` registration
- [ ] `tests/unit/nfc_felica/src/test_poller.c` — all ztests
- [ ] `tests/unit/nfc_felica/src/test_model.c` — serialize roundtrip unchanged
- [ ] `tests/unit/nfc_reader/src/test_store_felica_roundtrip.c` — store envelope
- [ ] `scripts/nfc/flipper_nfc_to_fixture.py` — `felica_read_steps()` / `build_felica_model()`
- [ ] `src/nfc/protocols/felica/felica.c` — model serialize/deserialize field layout
- [ ] Any applet using `NFC_PERSIST_ID_FELICA` / clone path
- [ ] CMake: `tests/unit/nfc_felica/CMakeLists.txt` includes new fixture headers if added

---

## Deferred scope

- FeliCa Standard workflow (RequestSystemCode, ListService, auth paths)
- DES-MAC internal/external authentication
- Native FeliCa listener + Tier C loopback
- HAL NFC-F mode / `furi_hal_nfc_felica.c` port
- HIL FeliCa on PN7160 (tech_mask gap)
- Full 29-TX assert without raising `NFC_SESSION_MOCK_MAX_TX` (anti-pattern — cap must rise)
