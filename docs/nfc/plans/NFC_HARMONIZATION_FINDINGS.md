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

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** All 8 protocol test suites PASS (see below)

### Deliverables

| Directory | CONTEXT.md | Lines |
|-----------|------------|-------|
| `src/nfc/protocols/ndef/` | Created | ~95 |
| `src/nfc/protocols/ultralight/` | Created | ~85 |
| `src/nfc/protocols/classic/` | Created | ~85 |
| `src/nfc/protocols/felica/` | Created | ~70 |
| `src/nfc/protocols/iso15693_3/` | Created | ~65 |
| `src/nfc/protocols/slix/` | Created | ~70 |
| `src/nfc/protocols/desfire/` | Created | ~90 |
| `src/nfc/protocols/emv/` | Created | ~85 |
| `src/nfc/protocols/aliro/` | Created | ~100 |

### P4 — Protocol Parity Matrix

| Protocol | Class | Flipper ref | QEMU proves | HIL must prove | Emulatable? |
|----------|-------|-------------|-------------|----------------|-------------|
| **NDEF** | emulatable | (via Ultralight tags) | model + poller + listener decode, store roundtrip | RF poll, real SELECT/READ BINARY, field on/off | Yes |
| **Ultralight** | emulatable | `Ultralight_11/21/C.nfc`, `Ntag213/215/216.nfc` | model + poller decode | page read over RF, UID/SAK from NCI | Yes (T4 adapter) |
| **Classic** | clone-only | `MfClassic_1K_4b.nfc` | model + poller, Crypto1, CRC | anticollision + auth + block read over RF | No |
| **FeliCa** | clone-only | `Felica.nfc` | model + poller decode | NFC-F polling + block read (tech_mask gap) | No |
| **ISO15693-3** | clone-only | (via SLIX) | model + poller decode | NFC-V inventory + block read | No |
| **SLIX** | clone-only | `Slix_cap_default/missed/accept_all_pass.nfc` | model + poller, CAP variants | NFC-V SLIX read over RF | No |
| **DESFire** | emulatable | (reader-captured) | model + poller + listener APDU | ISO-DEP transceive, app/file SELECT | Yes (partial) |
| **EMV** | emulatable | (reader-captured) | model + poller + listener decode | PPSE/AID SELECT + READ RECORD | Yes (partial) |
| **Aliro** | emulatable | (proprietary) | model + poller + listener; AID router | ISO-DEP AUTH transcript exchange | Yes (partial) |

### Flipper Fixture Mapping

| Flipper `.nfc` file | Protocol | Derived fixtures | Store `.card.bin` |
|--------------------|----------|------------------|-------------------|
| `Ultralight_11.nfc` | Ultralight | `tests/fixtures/ultralight/` | yes |
| `Ultralight_21.nfc` | Ultralight | yes | yes |
| `Ultralight_C.nfc` | Ultralight | yes | yes |
| `Ntag213_locked.nfc` | Ultralight | yes | yes |
| `Ntag215.nfc` | Ultralight | yes | yes |
| `Ntag216.nfc` | Ultralight | yes | yes |
| `Felica.nfc` | FeliCa | `tests/fixtures/felica/` | yes |
| `MfClassic_1K_4b.nfc` | Classic | `tests/fixtures/classic/` | yes |
| `Slix_cap_default.nfc` | SLIX | `tests/fixtures/slix/` | yes |
| `Slix_cap_missed.nfc` | SLIX | yes | yes |
| `Slix_cap_accept_all_pass.nfc` | SLIX | yes | yes |
| `nfc_nfca_signal_short.nfc` | HAL | `tests/fixtures/hal/` | n/a |
| `nfc_nfca_signal_long.nfc` | HAL | NFC-A signal capture | n/a |

### Loopback Eligibility

| Protocol | Loopback | Reason |
|----------|----------|--------|
| NDEF | Yes | `tests/common/nfc_virtual_loopback` |
| Ultralight | Via NDEF | T4T NDEF adapter, not native T2 |
| Classic | **SKIP** | Clone-only; no listener |
| FeliCa | **SKIP** | Clone-only; no listener |
| ISO15693-3 | **SKIP** | Clone-only; no listener |
| SLIX | **SKIP** | Clone-only; no listener |
| DESFire | Yes | `tests/common/nfc_virtual_loopback` |
| EMV | Yes | `tests/common/nfc_virtual_loopback` |
| Aliro | Yes | `tests/common/nfc_virtual_loopback` |

### Aliro vs NCS Door-Lock Parity

| Aspect | NCS `aliro/` tree | This protocol module |
|--------|-------------------|----------------------|
| Language | C++ | C (MISRA-compliant) |
| Crypto | C++ PSA wrappers | Direct PSA Crypto API |
| Key storage | kpersistent_manager | PSA persistent keys |
| Work queue | `aliro_work/` C++ | `nfc_stack_wq` (HAL-owned, C) |
| Build gate | `DOOR_LOCK_ALIRO_*` | `NFC_PROTOCOL_ALIRO` |
| License | GPL-derived reference | Clean-room reimplementation |

**Intent parity:** Both implement Aliro 0.9.4 NFC expedited AUTH (AUTH0 → AUTH1 → optional EXCHANGE). This module is **not a GPL port**—behavioral reference reimplementation.

### Exit Gate Test Results

| Protocol | Configs | Cases | Result |
|----------|---------|-------|--------|
| NDEF | 3/3 | 87/87 | **PASS** |
| Ultralight | 2/2 | 32/32 | **PASS** |
| Classic | 2/2 | 17/17 | **PASS** |
| FeliCa | 2/2 | 13/13 | **PASS** |
| SLIX | 2/2 | 18/18 | **PASS** |
| DESFire | 3/3 | 30/30 | **PASS** |
| EMV | 3/3 | 17/17 | **PASS** |
| Aliro | 3/3 | 19/19 | **PASS** |
| ISO15693-3 | — | — | SKIP (via nfc_reader) |

**Total:** 20 configs, 233 cases across 8 protocol suites.

### Audit Findings (D.4 Checklist)

#### A. Buffer Bounds

| Finding | Protocol | Severity | Notes |
|---------|----------|----------|-------|
| All capacity symbols have Kconfig defaults | all | OK | `NFC_*_MAX_*` symbols protect model arrays |
| Deserialize validates sizes before copy | all | OK | Oversize → `-ENOSPC` or `-EINVAL` |

#### B. Error Propagation

| Finding | Protocol | Severity | Notes |
|---------|----------|----------|-------|
| Functions return negative errno | all | OK | Consistent pattern |
| No silent drops in serialize/deserialize | all | OK | All paths return status |

#### C. Session/State

| Finding | Protocol | Severity | Notes |
|---------|----------|----------|-------|
| Poller single-flight via reader engine | all | OK | `-EBUSY` from engine level |
| Listener session via nfc_stack state | emulatable | OK | `STARTED` → `-EBUSY` guard |

#### D. Kconfig↔CMake Consistency

| Finding | Protocol | Severity | Notes |
|---------|----------|----------|-------|
| All sources gated by `CONFIG_NFC_PROTOCOL_*` | all | OK | CMakeLists.txt uses `zephyr_library_sources_ifdef` |
| Listener sources gated by `NFC_ROLE_LISTEN OR NFC_STORE` | ndef, ultralight | OK | Conditional in CMakeLists.txt |
| SLIX selects ISO15693_3 | slix | OK | Kconfig `select` chain |

#### E. Shell Leakage

| Finding | Protocol | Severity | Notes |
|---------|----------|----------|-------|
| No shell includes in protocol files | all | OK | Protocol layer is pure L0 |

#### F. Test Fidelity

| Finding | Protocol | Severity | Notes |
|---------|----------|----------|-------|
| Model tier (A) exercises serialize/deserialize | all | OK | All 9 protocols have model tests |
| Poller tier (B) exercises read sequence | all with tests | OK | Session mock fixtures |
| Listener tier (C) exercises APDU response | emulatable | OK | NDEF, DESFire, EMV, Aliro |
| Clone-only protocols have no listener tier | classic, felica, slix, iso15693_3 | OK | Correct per design |

### No Code Changes Required

P4 is a read-only audit phase. All findings confirm correct implementation per CONVENTIONS and Master Plan D.4 checklist

---

## P4b — Applets CONTEXT.md + L1 Verification

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:**
- `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (2/2 configs, 97/97 cases)
- Shell-off build (`CONFIG_SHELL=n`) → **PASS** (links clean)

### Deliverables

| Directory | CONTEXT.md | Lines |
|-----------|------------|-------|
| `src/nfc/applets/` | Created | ~95 |

### L1 API Verification

Verified `nfc_applet_service.h` matches locked L1 API (Phase A):

| Applet | L1 Functions | No `struct shell`? |
|--------|--------------|-------------------|
| Scan | `discover_start/stop/active`, `scan_get_result` | ✓ |
| Read | `read_start/busy/wait` | ✓ |
| Emulate | `emulate(slot, profile)` | ✓ |
| Loop | `loop_run(slot, timeout, log_fn, ctx, &result)` | ✓ |
| Check | `verify_start/busy/wait/last_result`, `verify_compare` | ✓ |
| Meta | `get_card_meta(slot, &out)` | ✓ |

### Callback Model Documented

| Applet | Callback | Purpose |
|--------|----------|---------|
| Scan | `nfc_applet_tag_cb_t` | Per-tag discovery notification |
| Loop | `nfc_applet_log_fn` | Per-stage progress (NULL = silent) |

### Grep Audit — Shell Leakage

```bash
$ grep -r "struct shell" src/nfc/applets/*.c | grep -v "_shell_cmds.c"
```

**Result:** Only documentation comments found ("Layer-1: NO struct shell"). No actual shell usage in L1.

```bash
$ grep -r "shell.h" src/nfc/applets/*.c | grep -v "_shell_cmds.c"
PASS: No shell.h include in L1 applet files

$ grep -r "shell_print\|shell_fprintf" src/nfc/applets/*.c | grep -v "_shell_cmds.c"
PASS: No shell_print in L1 applet files
```

**Conclusion:** L1 applet layer is clean — no shell symbols below L2.

### Shell-off Build Verification

```bash
$ west build -b qemu_cortex_m3 tests/unit/nfc_reader -d build_shell_off -- -DCONFIG_SHELL=n
```

**Result:** Build succeeded. `.config` confirms `# CONFIG_SHELL is not set`. Proves L1 code links without shell dependencies.

### Audit Findings (D.4 Checklist)

#### A. Buffer Bounds

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| Slot name length validated | `nfc_applet_verify.c:143` | OK | `len >= sizeof(s_verify_slot)` → `-EINVAL` |
| Result structs bounded | `nfc_applet_service.h` | OK | Fixed-size `nfc_applet_scan_result_t`, `nfc_applet_loop_result_t` |

#### B. Error Propagation

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| Functions return negative errno | all L1 files | OK | Consistent pattern |
| `-EBUSY` for concurrent access | scan, read, verify | OK | Atomic guards |
| `-EALREADY` for stop when not running | `discover_stop()` | OK | Expected pattern |

#### C. Session/State

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| Scan single-flight via atomic | `nfc_applet_scan.c:56` | OK | `s_discover_run` atomic |
| Verify single-flight via atomic | `nfc_applet_verify.c:29` | OK | `verify_busy` atomic |
| Loop orchestrates stop | `nfc_applet_loop.c:67` | OK | Stack stop between emulate→verify |

#### D. Kconfig↔CMake Consistency

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| L1 sources under `NFC_APPLETS` | `CMakeLists.txt:1-12` | OK | All L1 files gated |
| Emulate/loop require `NFC_LISTEN_STACK` | `CMakeLists.txt:14-19` | OK | Additional gate |
| Shell adapter under `NFC_APPLETS_SHELL` | `CMakeLists.txt:21-24` | OK | L2 isolated |

#### E. Shell Leakage

| Finding | File | Severity | Notes |
|---------|------|----------|-------|
| No shell in L1 | all non-`*_shell_cmds.c` | OK | Grep audit confirmed |
| Only `nfc_applet_shell_cmds.c` has shell | L2 adapter | OK | Correct isolation |

#### F. Test Fidelity

| Finding | Tier | Notes |
|---------|------|-------|
| `nfc_reader` Tier E | store/verify | 97 cases including verify-compare |
| Headless applet tier | deferred | P5: `loop_run` with `log==NULL` |

### No Code Changes Required

P4b is a read-only audit phase. All findings confirm correct implementation per Phase A deconvolution

---

## P5 — Test Gating Fix

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (3 configs, 155/156 cases)

### Deliverables

| Item | File | Status |
|------|------|--------|
| CMakeLists.txt guards | `tests/unit/nfc_reader/CMakeLists.txt` | Protocol sources wrapped in `if(CONFIG_NFC_PROTOCOL_X)` |
| Headless applet tests | `src/test_applet_headless.c` | 7 new L1 API tests (scan_get_result, get_card_meta, verify_compare) |
| Shell-off scenario | `testcase.yaml` | `nfc_reader.shell_off` with `CONFIG_SHELL=n` |
| Session mock | `src/nfc_reader_session_mock.c` | Shared configurable mock for reader session |

### CMakeLists.txt Changes (Master Plan B.5)

1. **Protocol source gating** — Each protocol block wrapped in `if(CONFIG_NFC_PROTOCOL_X)`:
   - NDEF, Ultralight, Classic, FeliCa, ISO15693-3, SLIX, DESFire, EMV, Aliro
   - Per-protocol test source files also gated

2. **Applet source gating** — `nfc_applet_service.c` and `nfc_applet_scan.c` under `if(CONFIG_NFC_APPLETS)`

3. **Memory optimization** — `NFC_APPLETS=n` in `overlay-store-ram.conf` to keep store_ram under 64KB RAM limit

### Headless Applet Tests (Master Plan E.5)

| Test | L1 Function | Proves |
|------|-------------|--------|
| `test_scan_get_result_no_session` | `nfc_applet_scan_get_result()` | Returns `-ENOENT` when no session |
| `test_scan_get_result_null_out` | `nfc_applet_scan_get_result()` | Returns `-EINVAL` for NULL output |
| `test_scan_get_result_populated` | `nfc_applet_scan_get_result()` | Populates result struct from session |
| `test_get_card_meta_missing_slot` | `nfc_applet_get_card_meta()` | Returns `-ENOENT` for missing slot |
| `test_get_card_meta_from_golden` | `nfc_applet_get_card_meta()` | Returns correct persist_id/flags/name |
| `test_verify_compare_headless` | `nfc_applet_verify_compare()` | Identical models compare equal |
| `test_verify_compare_headless_mismatch` | `nfc_applet_verify_compare()` | Different models return `-EBADMSG` |

### Shell-off Scenario

```yaml
nfc_reader.shell_off:
  tags: ci_unit
  platform_allow: qemu_cortex_m3
  extra_configs:
    - CONFIG_SHELL=n
    - CONFIG_NFC_APPLETS=y
```

Proves L1 applet code compiles and runs without shell. Exercises all store roundtrip and headless tests with `CONFIG_SHELL=n`.

### Findings

| Finding | Category | Severity | Notes |
|---------|----------|----------|-------|
| `test_virtual_loopback_desfire` fails with SHELL=n | F (Test fidelity) | **FIXED (P5b)** | Root cause: stack overflow corrupting ztest jmp_buf. Fixed by using static storage for large desfire_data_t. |
| store_ram at 99.98% RAM | D (Kconfig↔CMake) | addressed | Fixed by gating applet sources under NFC_APPLETS and disabling in store_ram overlay |
| prj.conf enables all protocols | D | acceptable | Current approach: prj.conf enables reader profile (all 9 protocols); CMake gating ensures only enabled protocols compile |

### Test Metrics

| Scenario | Configs | Cases | Result |
|----------|---------|-------|--------|
| store | 1 | 52 | PASS |
| store_ram | 1 | 52 | PASS |
| shell_off | 1 | 52 | **PASS** (was 51+1 skip, fixed in P5b) |
| **Total** | **3** | **156** | **156 PASS** (100%) |

**Baseline comparison:** P1 was 97/97 tests in 2 configs. P5 adds headless tests (+7) and shell_off scenario (+55), bringing total to 156 tests in 3 configs. All 156 pass after P5b fix.

---

## P5b — Shell Independence Audit (POST-P6)

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **156/156 PASS**

### Root Cause Analysis

The `test_virtual_loopback_desfire` failure with `CONFIG_SHELL=n` was caused by **stack overflow corrupting ztest's internal state**.

**Symptoms:**
- Test function completed successfully (all printk debug output showed success)
- ztest framework then marked the test as FAILED after the function returned
- Only DESFire loopback failed; EMV, Aliro, Ultralight, NDEF all passed
- Failure occurred only when `CONFIG_SHELL=n` changed memory layout

**Root Cause:**
- `desfire_data_t` structures are large (~450 bytes each due to `keys[14][16]` and `file_data[2][64]`)
- Test allocated two `desfire_data_t` on stack (~900 bytes total)
- With `CONFIG_SHELL=n`, the smaller binary/memory layout caused the stack to overflow into adjacent memory
- The overflow corrupted ztest's `jmp_buf` used for setjmp/longjmp in assertion handling
- Test function completed normally, but corrupted jmp_buf caused ztest to report failure on return

**Fix:**
- Moved `desfire_data_t` from stack allocation to static storage
- Used existing `s_desfire_clone` static variable for read output
- Added single `s_desfire_loopback` static for expected data
- Net RAM increase: ~450 bytes (one desfire_data_t) instead of ~900 bytes on stack

**Why Other Protocols Passed:**
- `emv_card_image_t`: smaller structure (~200 bytes)
- `aliro_data_t`: smaller structure (~200 bytes)
- `ndef_data_t`: smaller structure (~550 bytes but uses existing static `s_read`)
- `ultralight_data_t`: uses transformed ndef comparison with existing statics

### Audit Results

**Shell-dependent test skips (FIXED):**
```bash
$ grep -r "ztest_test_skip" tests/ | grep -i shell
# Previously: tests/unit/nfc_reader/src/test_virtual_loopback.c:550
# Now: REMOVED — test passes without skip
```

**Other `ztest_test_skip` calls:**
```bash
$ grep -r "ztest_test_skip" tests/
tests/unit/nfc_reader/src/test_store_roundtrip.c:307
# This skip is for CONFIG_NFC_STORE_RAM_SLOT_COUNT < 2 — legitimate, not shell-related
```

### Test Results

| Scenario | Configs | Cases | Result |
|----------|---------|-------|--------|
| store | 1 | 52 | PASS |
| store_ram | 1 | 52 | PASS |
| shell_off | 1 | 52 | **PASS** (was 51 + 1 skip) |
| **Total** | **3** | **156** | **156 PASS** (100%) |

### Exit Criteria

- [x] DESFire loopback passes with `CONFIG_SHELL=n`
- [x] No `ztest_test_skip()` calls gated on shell presence in NFC tests
- [x] Full unit test suite green with SHELL=n (156/156)
- [ ] CI includes SHELL=n test matrix (existing shell_off scenario covers this)

---

## P6 — Full QEMU Green + CI + Doc Drift

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** `west twister -T tests/unit -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (24 configs, 392/392 cases)

### Full Twister Results

```
INFO    - 24 test scenarios (24 configurations) selected
INFO    - 24 of 24 executed test configurations passed (100.00%)
INFO    - 392 of 392 executed test cases passed (100.00%)
```

| Suite | Configs | Cases |
|-------|---------|-------|
| nfc_ndef | 3 | 87 |
| nfc_ultralight | 2 | 32 |
| nfc_classic | 2 | 17 |
| nfc_felica | 2 | 13 |
| nfc_slix | 2 | 18 |
| nfc_desfire | 3 | 30 |
| nfc_emv | 3 | 17 |
| nfc_aliro | 3 | 19 |
| nfc_reader | 3 | 155 |
| nfc_apdu_asm | 1 | 4 |
| pn7160_tml | — | — |
| **Total** | **24** | **392** |

### CI Changes

Added `tests/unit/nfc_apdu_asm` to `.github/workflows/twister.yaml` `UNIT_DIRS` array (line 160). CI matrix now has 11 unit test directories.

### Doc Drift Fixes

| Doc | Issue | Fix Applied |
|-----|-------|-------------|
| `CI_TESTING.md` | Said "Local only" for NFC stack tests | Updated to "Active in CI"; added CI unit matrix section listing all 11 dirs |
| `NFC_HAL_GUIDE.md` | Gate 3 listen status said "drops only" | Updated to "implemented"; added overlay matrix table |
| `NFC_APPLETS_AND_TESTING.md` | Outdated ztest counts (said "13 ztests") | Added actual counts (392 total); updated Tier E owner to 59 ztests |
| `tests/unit/nfc_reader/README.md` | Said "13 ztests" | Updated to "59 ztests across 12 source files"; added full test inventory |
| `tests/fixtures/nfc/flipper/README.md` | `nfc_nfca_signal_long.nfc` row malformed (2 cols) | Fixed to 5 columns matching table format |
| `plans/2026-06-14-nfc-sequential-execution.md` | Stale shell names (`nfc verify`, `nfc reader clone`) | Updated to locked names (`nfc check`, `nfc scan start/stop`, dropped `reader clone`) |

### Verification

All doc fixes are consistent with:
- Phase A locked applet names (`scan start/stop`, `read`, `emulate`, `loop`, `check`)
- DROPPED commands: `nfc reader clone`, `nfc verify` (product), blocking `nfc scan`
- Actual test counts from twister output

---

## P7 — Architecture Assembly

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** All CONTEXT.md cross-links resolve; architecture doc contains all 9 sections

### Deliverables

| Item | File | Status |
|------|------|--------|
| Architecture doc | `docs/nfc/NFC_STACK_ARCHITECTURE.md` | Created (9 sections) |
| Findings rollup | This file | Updated |

### Architecture Document Sections

1. **Block diagram L0→L3** — ASCII art showing modules, HAL, protocols, applets, shell
2. **Mermaid data flow** — reader path, listen path, loop orchestration
3. **HAL profiles table** — PN7160 reader/listen, NFCT CE, LAB scaffold
4. **Protocol registry** — 9 protocols with emulatable status, Kconfig, QEMU/HIL coverage
5. **Store envelope** — V2 blob format, CRC, quiescent check
6. **Applet L1/L2/L3 split** — headless API, shell mapping (cross-link to NFC_SHELL_APPLETS.md)
7. **Test pyramid** — Tiers A/B/C/D/E with Kconfig gates, 24 configs/392 cases
8. **Overlay × profile matrix** — what each overlay enables
9. **HIL pointer** — "bench ready, run when hardware available" → link to NFC_HIL_PROTOCOL_GUIDE.md

### Cross-Link Verification

| Link | Source | Target | Status |
|------|--------|--------|--------|
| `NFC_SHELL_APPLETS.md` | Architecture §6 | `docs/nfc/NFC_SHELL_APPLETS.md` | ✓ resolves |
| `NFC_HIL_PROTOCOL_GUIDE.md` | Architecture §9 | `docs/nfc/NFC_HIL_PROTOCOL_GUIDE.md` | ✓ resolves |
| `CONTEXT.md` files (18) | Architecture §cross-ref | `src/nfc/**/CONTEXT.md`, `modules/nfc_pn7160/CONTEXT.md` | ✓ all resolve |
| `plans/NFC_APPLET_API_LAYERING.md` | applets CONTEXT.md | `docs/nfc/plans/NFC_APPLET_API_LAYERING.md` | ✓ resolves |

---

## P8 — Memory & Runtime Audit

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** Memory profile documented; 4 preset configs created

### Memory Baseline (nrf54l15dk/nrf54l15/cpuapp)

| Profile | Overlay(s) | RAM | FLASH |
|---------|-----------|-----|-------|
| PN7160 HAL only | `overlay-pn7160-hal.conf` | 16 KB | 86 KB |
| PN7160 reader | `overlay-pn7160-stack.conf` | **55 KB** | **119 KB** |
| PN7160 reader + CE | + `overlay-pn7160-listen.conf` | 57 KB | 131 KB |
| NFCT CE only | `overlay-nfct-stack.conf` | 54 KB | 102 KB |

### Key Findings

**Work queues:** 2 dedicated queues
- `nfc_stack_wq`: 2 KB stack (HAL poll, reader scan, listen recv)
- `pn7160_work_q`: 1 KB stack (driver events)

**Largest RAM consumers:**
| Component | Size | % | Notes |
|-----------|------|---|-------|
| Store slots | 16.5 KB | 30% | 4 slots × ~4 KB each |
| Store buffers | 8 KB | 15% | staging + serialize |
| DESFire model | 10 KB | 18% | Tune via MAX_APPS/FILES/SIZE |
| Thread stacks | 9.5 KB | 17% | Fixed overhead |
| Shell | 3.7 KB | 7% | Disable for headless |

**Static buffers:** APDU pool (4 × 512 B) only allocated when `NFC_ROLE_LISTEN=y`.

### Optimization Levers

| Kconfig | Default | Tuned | Savings |
|---------|---------|-------|---------|
| `NFC_STORE_RAM_SLOT_COUNT` | 4 | 1 | ~12 KB |
| `NFC_DESFIRE_MAX_APPS` | 4 | 1 | ~7 KB |
| `NFC_STORE_BLOB_SIZE` | 4096 | 2048 | 4 KB |
| `SHELL` | y | n | ~4 KB RAM, ~15 KB FLASH |

### Deliverables

| Item | File | Status |
|------|------|--------|
| Memory profile doc | `docs/nfc/NFC_MEMORY_PROFILE.md` | Created |
| PN7160 reader preset | `confs/pn7160-reader.conf` | Created |
| PN7160 CE preset | `confs/pn7160-ce.conf` | Created |
| PN7160 reader+CE preset | `confs/pn7160-reader-ce.conf` | Created |
| NFCT CE preset | `confs/nfct-ce.conf` | Created |

### Assessment

The stack is lean relative to feature set. 55 KB RAM for a full reader with 9 protocols, 4-slot store, and shell is reasonable. The "work queue or two" expectation is accurate (2 queues, ~3 KB stacks), but protocol data models (especially DESFire at 10 KB) and store slots (16.5 KB) dominate RAM. Production builds should tune these Kconfig options based on actual card complexity.

---

## Summary

| Phase | Status | Key metric |
|-------|--------|------------|
| A | DONE | 97/97 baseline |
| P1 | DONE | 97/97, imply chains verified |
| P2 | DONE | pn7160_tml 9/9, nfc_apdu_asm 4/4; 4× CONTEXT.md |
| P3 | DONE | 97/97 (verified); 4× CONTEXT.md |
| P4 | DONE | 20 configs, 233 cases; 9× protocol CONTEXT.md; parity matrix |
| P4b | DONE | 97/97; applets CONTEXT.md; grep audit + shell-off build PASS |
| P5 | DONE | 155/156 cases (3 configs); CMake guards; headless tests |
| P6 | DONE | **24 configs, 392/392 cases**; CI updated (nfc_apdu_asm); 6 doc drift fixes |
| P7 | DONE | NFC_STACK_ARCHITECTURE.md (9 sections); cross-links verified |
| P8 | DONE | Memory profile: 55 KB RAM, 119 KB FLASH (PN7160 reader); 4 presets |

---

## Program Completion Summary

**Program:** NFC Harmonization (P1–P8)  
**Status:** COMPLETE  
**Date:** 2026-06-14

### Final Metrics

| Metric | Value |
|--------|-------|
| QEMU test configs | 24 |
| QEMU test cases | 392/392 PASS |
| CONTEXT.md files | 18 |
| Architecture doc sections | 9 |
| Protocols documented | 9 |
| Profile overlays | 4 |
| Memory profile | 55 KB RAM / 119 KB FLASH (PN7160 reader) |
| Kconfig presets | 4 (pn7160-reader, pn7160-ce, pn7160-reader-ce, nfct-ce) |

### Open Items

| Item | Category | Status | Notes |
|------|----------|--------|-------|
| P5b shell independence audit | Test fidelity | **DEFERRED** | DESFire loopback fails with SHELL=n; workaround in place |
| HIL Gates 2–5 | Hardware | **BENCH READY** | Run when hardware available; see NFC_HIL_PROTOCOL_GUIDE.md |
| RW+CE concurrent | Feature | **DEFERRED** | NXP combined-discovery sequencing not implemented |
| tech_mask NFC-F | Feature | **DEFERRED** | FeliCa discovery via tech_mask not honored |
| nfc_t2t_lib Type-2 listener | Feature | **DEFERRED** | T4T adapter covers v1 |

### Landed Artifacts

| Category | Items |
|----------|-------|
| **Docs** | NFC_STACK_ARCHITECTURE.md, NFC_MEMORY_PROFILE.md, 18× CONTEXT.md, NFC_SHELL_APPLETS.md, NFC_HIL_PROTOCOL_GUIDE.md |
| **Code** | L1/L2 deconvolution (Phase A), CMake protocol guards (P5), headless applet tests |
| **Config** | `confs/` directory with 4 production presets (pn7160-reader, pn7160-ce, pn7160-reader-ce, nfct-ce) |
| **CI** | 11 unit test directories in twister.yaml; 24 configs green |

### Authority Documents

| Doc | Role |
|-----|------|
| `NFC_STACK_ARCHITECTURE.md` | Assembled architecture (block diagram, data flow, matrices) |
| `NFC_SHELL_APPLETS.md` | Shell command reference (LOCKED) |
| `NFC_HIL_PROTOCOL_GUIDE.md` | Bench procedure for Gates 2–5 |
| `plans/NFC_CONSOLIDATED_EXECUTION.md` | Operational plan of record |
| `plans/NFC_HARMONIZATION_MASTER_PLAN.md` | Authority on layering, acceptance philosophy |
