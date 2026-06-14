# NFC Harmonization Findings

Rollup of findings from program phases P1–P7.  
Format: `severity · file:line · category(D.4) · description · fix status`

---

## Phase A — Applets + Shell Decoupling (LANDED)

**Status:** DONE 2026-06-14  
**Commits:** 32b9ad1 → ca54ea7  
**Baseline:** 97/97 twister tests on `qemu_cortex_m3`

### Locked artifacts
- L1 applet API: `nfc_applet_service.h` — no `struct shell *`
- L2 shell: `*_shell_cmds.c` only
- Commands: `scan start/stop`, `read`, `emulate`/`golden`, `loop`, `check` (DK)
- DROPPED: `reader clone`, product `verify`

---

## P1 — Kconfig Truth Audit

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (2/2 configs, 97/97 cases)

### KCONFIG_TRUTH_FINDINGS

#### 1. Profile → Symbol Table (verified via `.config` dumps)

| Profile | What it implies |
|---------|-----------------|
| `NFC_PROFILE_READER` | `NFC_READER`, `NFC_STORE`, `NFC_STORE_RAM`, `NFC_APPLETS`, **all 9 protocols** (NDEF, Ultralight, Classic, FeliCa, ISO15693-3, SLIX, DESFire, EMV, Aliro) |
| `NFC_PROFILE_CARD_EMULATION` | `NFC_LISTEN_STACK` (→ selects `NFC_FRAMING`, `NFC_ROUTER`, `NFC_PROTOCOL_NDEF`), `NFC_STORE`, `NFC_STORE_RAM`, **5 emulatable protocols** (NDEF, Ultralight, DESFire, EMV, Aliro) |
| `NFC_PROFILE_LAB` | nothing — HAL scaffold only (roles compile, no engine/store/applets/protocols) |

**Verified builds (qemu_cortex_m3):**

| Overlay(s) | Profile | Symbols confirmed |
|------------|---------|-------------------|
| `overlay-pn7160-stack.conf` | READER | 9 protocols, `NFC_READER=y`, `NFC_STORE=y`, `NFC_STORE_RAM=y`, `NFC_APPLETS=y`, `NFC_ROLE_LISTEN=n` (no listen stack) |
| `overlay-pn7160-stack.conf;overlay-pn7160-listen.conf` | READER + listen | all above + `NFC_LISTEN_STACK=y`, `NFC_FRAMING=y`, `NFC_ROUTER=y`, `NFC_ROLE_LISTEN=y` |
| `overlay-pn7160-hal.conf` | LAB | `NFC_ROLE_READER=y` but `NFC_READER` absent; no store/applets/protocols |

#### 2. Overlay → Profile Mapping

| Overlay file | Profile set | Additional explicit symbols |
|--------------|-------------|----------------------------|
| `overlay-pn7160-stack.conf` | `NFC_PROFILE_READER=y` | `NFC_ROLE_LISTEN=n`, board deps (I2C/GPIO/EMUL/PN7160) |
| `overlay-pn7160-listen.conf` | (layers on reader) | `NFC_ROLE_LISTEN=y`, `NFC_LISTEN_STACK=y` |
| `overlay-pn7160-hal.conf` | `NFC_PROFILE_LAB=y` | `NFC_ROLE_READER=y`, `# CONFIG_NFC_READER is not set` |
| `overlay-nfct-stack.conf` | `NFC_PROFILE_CARD_EMULATION=y` | `NFC_T4T_NRFXLIB=y`, `NFC_HAL_BACKEND_NRFX=y`, `# CONFIG_NFC_ROLE_READER is not set` |

**Imply chains match overlays:** one profile line enables the expected orchestrators, store, and protocols. Overlays reduced to profile + board deps + explicit role overrides.

#### 3. `nfc_reader` Test Divergence (document for P5 — DO NOT FIX HERE)

| Finding | File | Category | Severity | Fix phase |
|---------|------|----------|----------|-----------|
| All 9 protocol `.c` files compiled **unconditionally** | `tests/unit/nfc_reader/CMakeLists.txt:27-52` | D (Kconfig↔CMake) | divergence | P5 |
| `prj.conf` force-enables all 9 protocols regardless of scenario | `tests/unit/nfc_reader/prj.conf:6-14` | D | divergence | P5 |
| Test `Kconfig` **duplicates** the production symbol tree (155+ lines) instead of `rsource`-ing it | `tests/unit/nfc_reader/Kconfig:7-155` | D | divergence | P5 (Phase 3 per KCONFIG_ARCH §6) |
| `NFC_ROLE_LISTEN=y` stub present unconditionally | `tests/unit/nfc_reader/Kconfig:133-135` | D | minor | P5 |
| Capacity defaults differ (test: smaller for RAM; prod: larger) | `prj.conf` vs `src/nfc/protocols/*/Kconfig` | D | acceptable | n/a |

**Root cause:** the test suite was written before profile `imply` chains landed. It compiles sources directly to prove store roundtrip without linking the HAL. This works but **diverges from profile builds** — the test proves code that a real overlay might not compile. Example: a CE-only profile (`NFC_PROFILE_CARD_EMULATION`) does not imply Classic/FeliCa/SLIX/ISO15693-3, yet `nfc_reader` tests them anyway.

**P5 fix plan (from Master Plan B.5):**
1. Wrap each protocol block in CMakeLists.txt under `if(CONFIG_NFC_PROTOCOL_X)`.
2. Split `prj.conf` into profile-scoped fragments (`prj_reader.conf`, `prj_ce.conf`).
3. Add twister scenarios tied to profiles.
4. (Deferred to KCONFIG_ARCH Phase 3) Replace duplicated test Kconfig with `rsource "../../../src/nfc/Kconfig"`.

#### 4. Kconfig Inconsistencies Found

| Finding | Category | Severity | Notes |
|---------|----------|----------|-------|
| None critical | — | — | Imply chains are correct; `select` (CRC from NFC_STORE, NET_BUF from NFC_ROLE_LISTEN, FRAMING/ROUTER/NDEF from NFC_LISTEN_STACK) match documented architecture |

#### 5. Profile → Compiles → May-Test Mapping (per Master Plan Part B)

| Profile | Compiles (production) | Tests that may run |
|---------|----------------------|-------------------|
| `NFC_PROFILE_READER` | reader engine, store+RAM, applets, **all 9 poller protocols**; **no listen stack** | protocol Tier A+B (all 9), `nfc_reader` Tier E (store+store_ram), applet L1 headless (future P5/P8) |
| `NFC_PROFILE_CARD_EMULATION` | listen stack (framing+router), store+RAM, applets, **emulate subset** (5 protocols) | Tier A+C for emulate subset, virtual loopback (emulatable only), `nfc_reader` Tier E |
| `NFC_PROFILE_LAB` | HAL + role scaffold only | module/HAL tier only (`pn7160_tml`, `nfc_apdu_asm`); no protocol/store/applet tiers |

**Test truth gap:** `nfc_reader` currently runs all 9 protocols for both scenarios (store, store_ram). A reader-profile-only scenario should be added in P5; a CE-profile scenario should skip clone-only protocols.

### Verification summary

```
$ west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild
INFO - 2 of 2 executed test configurations passed (100.00%)
INFO - 97 of 97 executed test cases passed (100.00%)
```

**Exit gate: PASS.** No code changes required for P1 (read-only audit).

---

## P2 — HAL + APDU Layer CONTEXT.md

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:**
- `west twister -T modules/nfc_pn7160/tests -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (1/1 config, 9/9 cases)
- `west twister -T tests/unit/nfc_apdu_asm -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (1/1 config, 4/4 cases)

### Deliverables

| Directory | CONTEXT.md | Lines |
|-----------|------------|-------|
| `modules/nfc_pn7160/` | Created | ~60 |
| `src/nfc/hal/` | Created | ~55 |
| `src/nfc/framing/` | Created | ~45 |
| `src/nfc/router/` | Created | ~50 |

### Audit Findings (D.4 Checklist)

#### A. Buffer Bounds

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| TML frame length validated before read | `pn7160_tml_framing.c` | OK | `pn7160_tml_frame_len_validate()` rejects oversize → `-EINVAL` |
| APDU net_buf FIXED pool with capacity check | `nfc_apdu_pool.c`, `nfc_apdu_asm.c` | OK | `nfc_apdu_frag_fits()` + `on_frag()` check before `net_buf_add_mem`; overflow → `NFC_APDU_ASM_DROP_OVERSIZE` |
| PN7160 RX buffer static | `pn7160_priv.h:46` | OK | `rx_buf[CONFIG_PN7160_RX_BUF_SIZE]` (default 258) |
| APDU parse validates length before access | `apdu_assembler.c:51-116` | OK | Rejects < 4 bytes, length mismatch → `6700` SW |
| AID router table capped | `aid_router.c:30` | OK | `s_table[CONFIG_NFC_ROUTER_MAX_AIDS]`; register → `-ENOMEM` when full |

#### B. Error Propagation

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| errno returned consistently | all | OK | Functions return negative errno |
| Stats counters track errors | all | OK | `error_count`, `last_error_code` in each stats struct |
| Named drop counters | transport, framing | OK | `frag_dropped_no_buffer`, `apdu_oversized_count`, `apdu_dropped_no_consumer`, `apdu_parse_reject_count` |

#### C. Session/State

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| PN7160 `discovery_active` flag | `pn7160_priv.h:43` | OK | Tracks RF state |
| Transport state enum | `nfc_transport.h:47-52` | OK | UNINITIALIZED/INITIALIZED/STARTED/STOPPED |
| Framing state | `apdu_assembler.c:34` | OK | Drops APDUs when not INITIALIZED |
| Router state | `aid_router.c:28` | OK | Returns `-ENODEV` when not initialized |

#### D. Kconfig↔CMake Consistency

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| PN7160 module gating | `modules/nfc_pn7160/CMakeLists.txt` | OK | All sources under `if(CONFIG_PN7160)` |
| TML I2C/SPI conditional | CMakeLists.txt:15-20 | OK | `if(CONFIG_PN7160_TML_I2C/SPI)` |
| Shell gating | CMakeLists.txt:21-23 | OK | `if(CONFIG_PN7160_SHELL)` |
| HAL sources gated | `src/nfc/hal/CMakeLists.txt` | OK | Backend + pool/asm + shell all conditional |
| Framing/router gated | respective CMakeLists.txt | OK | `zephyr_library_sources_ifdef(CONFIG_NFC_FRAMING/ROUTER ...)` |

#### E. Shell Leakage

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| No shell in L0 engine files | all non-`*_shell_cmds.c` | OK | Verified: no `#include <zephyr/shell/shell.h>` outside shell adapters |
| PN7160 shell isolated | `pn7160_shell.c` under `PN7160_SHELL` | OK | Thin wrappers over public API |
| Transport shell isolated | `nfc_transport_shell_cmds.c` under `NFC_HAL_SHELL` | OK | Only reads via `get_*` functions |

#### F. Test Fidelity

| Finding | Tier | Notes |
|---------|------|-------|
| `pn7160_tml` Tier A | model | 9 cases: frame length validation (normal + DWL mode, boundary conditions) |
| `nfc_apdu_asm` Tier A | model | 4 cases: `frag_fits` boundaries, oversize drop decision, state preservation |
| Framing/router Tier C | via loopback | Exercised through emulatable protocol tests (NDEF, DESFire, EMV, Aliro) |

### No Code Changes Required

P2 is a read-only audit phase. All findings confirm correct implementation per CONVENTIONS and Master Plan D.4 checklist.

---

## P3 — Reader + Stack + Run + Store CONTEXT.md

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:**
- `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (2/2 configs, 97/97 cases)

### Deliverables

| Directory | CONTEXT.md | Lines |
|-----------|------------|-------|
| `src/nfc/reader/` | Created | ~55 |
| `src/nfc/nfc_stack/` | Created | ~50 |
| `src/nfc/run/` | Created | ~40 |
| `src/nfc/store/` | Created | ~60 |

### Audit Findings (D.4 Checklist)

#### A. Buffer Bounds

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| Tag name length validated | `nfc_reader_engine.c:273`, `nfc_store.c:120` | OK | `strlen(tag) >= sizeof(buffer)` → `-EINVAL` |
| Staging buffer size checked | `nfc_store.c:475` | OK | Entry overflow → `-ENOMEM` with stats |
| RAM slot blob size checked | `nfc_store_ram.c:82` | OK | `len > CONFIG_NFC_STORE_BLOB_SIZE` → `-ENOSPC` |

#### B. Error Propagation

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| errno returned consistently | all | OK | Functions return negative errno |
| Stats track errors | `nfc_store.c`, `nfc_stack.c` | OK | `error_count`, `last_error_code` |
| CRC mismatch → `-EBADMSG` | `nfc_store.c:217` | OK | `corrupt_blob_count` stat incremented |

#### C. Session/State

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| Reader scan/read/clone `-EBUSY` | `nfc_reader_engine.c:210,277,330` | OK | Atomics guard single-flight |
| Stack state machine | `nfc_stack.c` | OK | `UNINITIALIZED→INITIALIZED→STARTED→STOPPED→ERROR` |
| Store quiescent check | `nfc_store.c:101-108` | OK | `nfc_stack_get_state() == STARTED` → `-EBUSY` |
| Stack load/save guards | `nfc_stack.c:381,415` | OK | `STARTED` → `-EBUSY` |

#### D. Kconfig↔CMake Consistency

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| Reader sources gated | `reader/CMakeLists.txt` | OK | `if(CONFIG_NFC_READER)` |
| Shell gated separately | `reader/CMakeLists.txt:7` | OK | `if(CONFIG_NFC_READER_SHELL)` |
| Stack sources gated | `nfc_stack/CMakeLists.txt` | OK | `zephyr_library_sources_ifdef(CONFIG_NFC_LISTEN_STACK ...)` |
| Store RAM shell gated | `store/CMakeLists.txt:6` | OK | `zephyr_library_sources_ifdef(CONFIG_NFC_STORE_RAM_SHELL ...)` |
| Run workq gated | `run/CMakeLists.txt` | OK | `if(CONFIG_NFC_STACK)` |

#### E. Shell Leakage

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| `nfc_store.c` no shell include | `nfc_store.c` | OK | Phase A made default save inert (`-ENODEV`) |
| `nfc_store_ram.c` shell gated | `nfc_store_ram.c:183-264` | OK | `#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)` around shell code |
| `nfc_reader_engine.c` no shell | `nfc_reader_engine.c` | OK | No shell include |
| `nfc_stack.c` no shell | `nfc_stack.c` | OK | No shell include |
| Shell adapters isolated | `*_shell_cmds.c` | OK | All shell code in dedicated files under `*_SHELL` gates |

#### F. Test Fidelity

| Finding | Tier | Notes |
|---------|------|-------|
| `nfc_reader.store` Tier E | store roundtrip | 9-protocol serialize→save→load→deserialize |
| `nfc_reader.store_ram` Tier E | RAM backend | Slot management + overlay-store-ram.conf |
| `test_verify_diff.c` | E | Verify/check diff logic |
| `test_poller_registry.c` | E | Poller detect dispatch |

### Architecture Observations

1. **Work queue centralization:** All blocking NFC operations funnel through `nfc_stack_wq_get()` in `run/`. This is the single-flight enforcement point for poll vs listen mutual exclusion (at engine level, not kernel level).

2. **Buffer ownership path:** `reader_engine` → poller registry → protocol `_poller_read` → listener `_load` → `nfc_store_save` → `s_staging_buf`. Buffer is copied at each boundary; no pointer aliasing across work items.

3. **Quiescent check pattern:** Both `nfc_store_save/load` and `nfc_stack_load/save` check `nfc_stack_get_state() == STARTED` → `-EBUSY`. This prevents serialize/deserialize during active card emulation.

4. **Default save callback design:** `nfc_store_default_save()` returns `-ENODEV` (inert). Real backends (RAM) or shell adapters register their own callbacks. This is the Phase A shell leak fix.

### No Code Changes Required

P3 is a read-only audit phase. All findings confirm correct implementation per CONVENTIONS and Master Plan D.4 checklist.

---

## P4 — Protocol CONTEXT.md + Parity Matrix

**Status:** PENDING

---

## P4b — Applets CONTEXT.md + L1 Verification

**Status:** PENDING

---

## P5 — Test Gating Fix

**Status:** PENDING

---

## P6 — Full QEMU Green + CI + Doc Drift

**Status:** PENDING

---

## P7 — Architecture Assembly

**Status:** PENDING

---

## Summary

| Phase | Status | Key metric |
|-------|--------|------------|
| A | DONE | 97/97 baseline |
| P1 | DONE | 97/97, imply chains verified |
| P2 | DONE | pn7160_tml 9/9, nfc_apdu_asm 4/4; 4× CONTEXT.md |
| P3 | DONE | 97/97 (verified); 4× CONTEXT.md |
| P4 | PENDING | — |
| P4b | PENDING | — |
| P5 | PENDING | — |
| P6 | PENDING | — |
| P7 | PENDING | — |
