# P3 — ISO15693 Parent Suite + Addressed SLIX Mocks — Agent Dispatch Brief

**Priority:** #3  
**Goal:** Standalone `nfc_iso15693_3` Tier A/B suite, harden parent poller to Flipper wire shape, upgrade SLIX mocks/poller to addressed manufacturer frames.  
**Do not dispatch P1 (Ultralight).**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper ISO15693?** | **YES** — `flipperzero/lib/nfc/protocols/iso15693_3/` |
| **Flipper SLIX?** | **YES** — separate child protocol; parent→child poller chain |
| **Combined or separate?** | **Separate protocols, combined at runtime** — SLIX poller wraps `iso15693_3_poller_send_frame` |
| **Local SLIX `.nfc`?** | **YES** — 3 files: `Slix_cap_default.nfc`, `Slix_cap_missed.nfc`, `Slix_cap_accept_all_pass.nfc` |
| **Standalone ISO15693 `.nfc`?** | **NO** — parent section embedded in SLIX dumps only |
| **Our parent suite?** | **MISSING** — `tests/unit/nfc_iso15693_3/` does not exist |
| **Mock fidelity?** | **Abbreviated** — RX-only, wrong flags on non-inventory cmds, no addressed SLIX TX |

---

## Exact files and functions to edit

### Phase A — ISO15693 parent suite (new)

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Create** | `tests/unit/nfc_iso15693_3/` | `CMakeLists.txt`, `testcase.yaml`, `prj_model.conf`, `prj_poller.conf`, `Kconfig` |
| **Create** | `tests/unit/nfc_iso15693_3/src/test_model.c` | UID byte-reverse, `block_security[]` |
| **Create** | `tests/unit/nfc_iso15693_3/src/test_poller.c` | Inventory, sysinfo, read, security batch |
| **Create** | `tests/fixtures/iso15693_3/` | Pre-CRC TX/RX from `Slix_cap_default.nfc` parent section |
| **Extend** | `scripts/nfc/flipper_nfc_to_fixture.py` | `iso15693_framed_steps()` — INVENTORY, GET_SYS_INFO, READ, GET_BLOCKS_SECURITY |
| **Harden** | `src/nfc/protocols/iso15693_3/iso15693_3_poller.c` | All functions below |
| **Add** | `src/nfc/protocols/iso15693_3/iso15693_3_poller_i.c` (or inline) | CRC, parsers from Flipper `iso15693_3_i.c` |
| **Update header** | `src/nfc/protocols/iso15693_3/iso15693_3_poller.h` | Add `ISO15693_CMD_FLAGS` (`0x02`), `GET_BLOCKS_SECURITY` cmd |
| **Register CI** | `docs/nfc/CI_TESTING.md`, `.github/workflows/twister.yaml` | Add `nfc_iso15693_3` to unit matrix |

**Parent poller functions to change:**

- `iso15693_3_poller_get_system_info()` — `iso15693_3_poller.c` L63–87: use `0x02` flags, parse `info_flags`
- `iso15693_3_poller_read_block()` — L89–108: use `0x02` not `0x26`
- `iso15693_3_poller_detect()` — L110–132: relax or split `uid[0]==0xE0` for generic 15693
- **Add** `iso15693_3_poller_get_blocks_security()` — batch `0x2C`, populate `block_security[]`
- **Add** optional-step degradation (NotSupported/Timeout → continue)

### Phase B — SLIX addressed mocks + poller

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Refactor** | `src/nfc/protocols/slix/slix_poller.c` | `slix_poller_read()` L70–118; add `slix_poller_prepare_request()` |
| **Add** | `src/nfc/protocols/slix/slix_poller_i.c` | Addressed frame builder (flags `0x22`, mfg `0x04`, UID append reversed) |
| **Extend generator** | `scripts/nfc/flipper_nfc_to_fixture.py` | `slix_read_steps()` — addressed TX for `0xAB`, `0xBD` |
| **Update tests** | `tests/unit/nfc_slix/src/test_poller.c` | TX shape asserts, full parent field compare |
| **Remove** | `slix_poller.c` L75, L113–116 | Synthetic `0xB0` CAP probe — load CAP from model (Flipper-aligned) |

### Flipper reference (read-only)

- `flipperzero/lib/nfc/protocols/iso15693_3/iso15693_3_poller_i.c`
- `flipperzero/lib/nfc/protocols/slix/slix_poller_i.c` — `slix_poller_prepare_request()`
- `flipperzero/applications/debug/unit_tests/tests/nfc/nfc_test.c` — `slix_set_password_*`

---

## Wire format bytes / parity targets

### Flag bytes (Flipper)

| Command | Flags | UID in frame |
|---------|-------|--------------|
| INVENTORY `0x01` | `0x26` | No |
| GET SYSTEM INFO `0x2B` | `0x02` | No |
| READ BLOCK `0x20` | `0x02` | No |
| GET BLOCKS SECURITY `0x2C` | `0x02` | No (32-block batches) |
| SLIX mfg (`0xAB`, `0xBD`, `0xB3`, …) | `0x22` (`0x02 \| ADDRESSED`) | Yes (reversed on-air) + mfg code `0x04` after command |
| GET RANDOM `0xB2` | `0x02` | No (`skip_uid=true`) |

### Our bugs vs targets

| Command | Flipper TX prefix | Our TX today |
|---------|-------------------|--------------|
| INVENTORY | `26 01` | `26 01` ✓ (`iso15693_3_poller.c` L44) |
| GET_SYS_INFO | `02 2B` | `26 2B` ✗ (`iso15693_3_poller.c` L66) |
| READ BLOCK | `02 20 [block]` | `26 20 [block]` ✗ (`iso15693_3_poller.c` L92) |
| SLIX NXP sysinfo | `22 AB 04 [uid×8]` | `26 AB` ✗ (`slix_poller.c` L73) |
| SLIX signature | `22 BD 04 [uid×8]` | `26 BD` ✗ (`slix_poller.c` L74) |

### Golden UID (from `Slix_cap_default.nfc`)

Extract from fixture generator output in `tests/fixtures/slix/Slix_cap_default_mock.h` — verify UID byte-reverse in inventory response matches Flipper `iso15693_3_uid_from_inventory()`.

### CRC

- ISO13239 CRC append/check/trim on all frames (Flipper `iso15693_3_poller_i.c`) — **missing in our parent poller**

### CAP semantics

- Flipper: CAP is `.nfc`/listener emulation field — **not** RF-read by poller SM
- Ours: invented `0xB0` probe (`slix_poller.c` L75, L113–116) — **remove**, set `capabilities` from deserialized model

---

## Current bugs (file:line)

| Bug | Location | Impact |
|-----|----------|--------|
| GET_SYS_INFO/READ use inventory flags `0x26` | `iso15693_3_poller.c` L66, L92 | Wrong on-air flags |
| No ISO13239 CRC | `iso15693_3_poller.c` (entire file) | Wire mismatch |
| No GET_BLOCKS_SECURITY | `iso15693_3_poller.c` | `block_security[]` never populated by poller |
| Simplified sysinfo parse (no `info_flags`) | `iso15693_3_poller.c` L76–79 | AFI, IC ref not validated |
| Detect requires `uid[0]==0xE0` | `iso15693_3_poller.c` L127–129 | SLIX-biased; blocks generic 15693 |
| SLIX uses non-addressed frames | `slix_poller.c` L73–75 | Wrong SLIX mfg wire |
| Synthetic CAP probe `0xB0` | `slix_poller.c` L75, L113–116 | Diverges from Flipper |
| No `tests/unit/nfc_iso15693_3/` | (missing) | Parent never independently tested |
| Mocks RX-only, stub sysinfo `[0x00,0x0F]` | `scripts/nfc/flipper_nfc_to_fixture.py` | Poller tests don't catch wire bugs |
| No TX asserts in SLIX poller tests | `tests/unit/nfc_slix/src/test_poller.c` L33–38 | Count-only on detect |

---

## DO NOT touch

- `src/nfc/protocols/ultralight/**` — P1
- ISO15693/SLIX listeners — clone-only; use Flipper `*_listener_i.c` as reference only
- Password set/unlock Tier B+ until addressed mocks land (optional follow-up)
- Flipper submodule files
- FeliCa / Classic / NDEF poller code paths

---

## Staged commits (exact message templates)

### Commit 1 — ISO15693 fixtures + generator

```
tests/iso15693_3: add fixture dir and framed step generator

Extract parent section from Slix_cap_default.nfc; emit pre-CRC TX/RX
for INVENTORY, GET_SYS_INFO, READ, GET_BLOCKS_SECURITY.
```

### Commit 2 — Parent poller hardening

```
nfc/iso15693_3: separate inv vs cmd flags and add security batch read

Use 0x26 for inventory, 0x02 for sysinfo/read/security; port optional-step
degradation; populate block_security[] from GET_BLOCKS_SECURITY.
```

### Commit 3 — Parent unit suite

```
tests/iso15693_3: add Tier A model and Tier B poller ztest suite

Register sample.nfc.unit.nfc_iso15693_3.{model,poller} in CI matrix.
```

### Commit 4 — SLIX addressed poller

```
nfc/slix: use addressed manufacturer frames and drop synthetic CAP probe

Add slix_poller_prepare_request(); type-gate sysinfo/signature reads;
load capabilities from model per Flipper semantics.
```

### Commit 5 — SLIX mocks + TX asserts

```
tests/slix: regenerate addressed mocks and assert SLIX TX shape

Extend slix_read_steps() with 0x22-prefixed mfg commands; update test_poller.c.
```

### Commit 6 — Docs + registry

```
docs/nfc: register nfc_iso15693_3 in CI_TESTING and flipper README

Mark SLIX Tier A/B parity row; note ISO15693 parent suite landed.
```

---

## Twister verify commands

```bash
REPO=/Users/majidfaroud/writable_ndef_msg

# New parent suite (after Commit 3)
west twister -T "$REPO/tests/unit/nfc_iso15693_3" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# SLIX regression (after Commit 5)
west twister -T "$REPO/tests/unit/nfc_slix" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Combined P3 gate
west twister -T "$REPO/tests/unit/nfc_iso15693_3" -T "$REPO/tests/unit/nfc_slix" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Store roundtrip
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.store -v
```

**Pass criteria:** New `nfc_iso15693_3` configs green; existing 18 SLIX cases remain green; store SLIX roundtrip passes.

---

## Caller sweep checklist

- [ ] `src/nfc/protocols/slix/slix_poller.c` — only consumer of `iso15693_3_poller_read()`
- [ ] `src/nfc/reader/nfc_reader_poller_registry.c` — `iso15693_3` + `slix` detect/read registration
- [ ] `tests/unit/nfc_reader/src/test_poller_registry.c` — registry smoke
- [ ] `tests/unit/nfc_reader/src/test_store_slix_roundtrip.c` — 5 store cases
- [ ] `tests/unit/nfc_slix/src/test_model.c` — 3 CAP variant blobs unchanged layout
- [ ] `scripts/nfc/flipper_nfc_to_fixture.py` — `iso15693_read_steps()`, `slix_read_steps()`, `build_slix_model()`
- [ ] `src/nfc/protocols/iso15693_3/iso15693_3.c` — serialize/deserialize `block_security[]`
- [ ] `src/nfc/protocols/slix/slix.c` — embedded `Iso15693_3Data` parent section

---

## Deferred scope

- SLIX password unlock loopback (`0xB2`/`0xB3`) — Tier B+ optional
- Privacy mode / lock EAS/PPL RF commands
- Flipper HAL `furi_hal_nfc_iso15693.c` port
- Standalone bare ISO15693 `.nfc` capture (derive from SLIX parent section is sufficient for v1)
- native_sim / HIL ISO15693
