# NFC Sequential Execution Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Unblock HAL gaps (Step A), then land Gates 2–5 sequentially — NDEF clone on PN7160, listen/router on PN7160, applets loop, NFCT port — one commit per completed sub-step.

**Architecture:** Flipper-aligned layering with two orchestrators (`reader/` poll, `nfc_stack/` listen). Protocols never touch HAL; poll transceive goes through `nfc_reader_session_*`; listen APDUs flow HAL → `nfc_apdu_pool` → `on_apdu` → router (Gate 3). PN7160 = reader + optional CE; NFCT = listen-only T4T PICC. Driver frozen @ `21bdd71`.

**Tech Stack:** Zephyr RTOS (NCS v3.2.4), PN7160 NCI driver (`modules/nfc_pn7160/`), nrfxlib `nfc_t4t_lib`, `net_buf` FIXED pools, ztest/twister, MISRA C:2012.

**References:** [`NFC_STACK_PLAN.md`](../NFC_STACK_PLAN.md), [`NFC_HAL_GUIDE.md`](../NFC_HAL_GUIDE.md), [`NFC_PROTOCOLS_COOKBOOK.md`](../NFC_PROTOCOLS_COOKBOOK.md), [`NFC_STACK_CONVENTIONS.md`](../NFC_STACK_CONVENTIONS.md).

**Locked product rules:** Ultralight = T4 adapter; Classic = PN7160 poller only; ISO-DEP listeners use PICC/raw lane; no NCI API changes without spec revision.

---

## Step A — HAL unblock (before Gate 2)

Must complete before any `protocols/` work. One commit per sub-step.

### A1: Hold poll session after discover

**Problem:** `nfc_reader_engine.c` called `discover_stop()` + `session_clear()` immediately after scan, so Gate 2 pollers cannot transceive.

**Files:**
- Modify: `src/nfc/reader/nfc_reader_engine.c`
- Modify: `src/nfc/reader/nfc_reader_engine.h`

**Implementation:**
- On successful `discover_wait`, set `s_session.active = true` and **do not** stop discovery.
- Add `nfc_reader_session_get()` and `nfc_reader_session_end()` (stop + clear).
- Timeout/error paths still stop discovery and clear session.

**Exit tests:**
```bash
source scripts/env/ncs-env.sh
cd "$NCS_ROOT"
west twister -T "$REPO/tests/unit/nfc_apdu_asm" -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v
# Build reader overlay (compile check):
west build -b qemu_cortex_m3/ti_lm3s6965 "$REPO" --no-sysbuild \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf" -DCONFIG_NFC_READER=y
```
Expected: ztest PASS; reader engine compiles with session API.

**Commit:** `nfc/reader: hold poll session after discover for Gate 2`

- [x] **Done** (`688c370`)

---

### A2: PN7160 listen `on_apdu` parity

**Problem:** `listen_recv_work_handler` in `nfc_transport_pn7160.c` incremented drop stats only; NFCT backend already had full pool/fifo/`on_apdu` path.

**Files:**
- Modify: `src/nfc/hal/nfc_transport_pn7160.c`
- Reuse: `src/nfc/hal/nfc_apdu_pool.c`, `src/nfc/hal/nfc_apdu_asm.c` (via `CONFIG_NFC_ROLE_LISTEN`)

**Implementation:**
- WQ recv loop: `net_buf_alloc(nfc_apdu_pool_get())` → copy `card_mode_recv` payload → `k_fifo` → `apdu_work_handler` → `ops.on_apdu` (callee `net_buf_unref`).
- Oversize → `6700` via `nfc_transport_send_response`; stats mirror nrfx.
- Init fifo/work in `nfc_transport_init`; drain on `stop`/`shutdown`.

**Exit tests:**
```bash
west build -b qemu_cortex_m3/ti_lm3s6965 "$REPO" --no-sysbuild \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf" \
     -DCONFIG_NFC_ROLE_LISTEN=y -DCONFIG_NFC_ROLE_READER=y
```
Expected: build succeeds with listen + reader roles.

**Commit:** `nfc/hal: PN7160 listen on_apdu path (Gate 3 prep)`

- [x] **Done** (`cfd30b0`)

---

### A3: NFCT explicit PICC mode

**Problem:** nrfx HAL relied on implicit default (no `nfc_t4t_ndef_*` calls) but did not document locked PICC profile.

**Files:**
- Modify: `src/nfc/hal/nfc_transport_nrfx.c`

**Implementation:**
- `static const nfc_t4t_emu_mode_t s_t4t_emu_mode = NFC_T4T_EMUMODE_PICC;`
- `BUILD_ASSERT(s_t4t_emu_mode == NFC_T4T_EMUMODE_PICC);`
- Comment in `nfc_transport_init`: no `nfc_t4t_ndef_*` before `emulation_start`.

**Exit tests:**
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-nfct-stack.conf"
```
Expected: NFCT listen overlay builds (hardware optional for runtime).

**Commit:** `nfc/hal: NFCT T4T raw PICC mode explicit`

- [x] **Done** (`2bd1e77` → `646c5ab`)

---

### A4: Plan + STACK_PLAN sync

**Files:**
- Create: `docs/nfc/plans/2026-06-14-nfc-sequential-execution.md` (this file)
- Modify: `docs/nfc/NFC_STACK_PLAN.md` — insert Step A in Next steps; mark closed HAL gaps

**Exit tests:** Doc review only; links resolve.

**Commit:** `docs/nfc: sequential execution plan with Step A gate`

- [x] **Done** (`2c9fab1`)

---

## Gate 2 (B) — `protocols/ndef` poller + clone

**Scope:** First `protocols/` tree; minimal `store/`; reader clone shell.

**Files (create):**
- `src/nfc/protocols/ndef/ndef.h`, `ndef.c`, `ndef_poller.c`, `CMakeLists.txt`, `Kconfig`
- `src/nfc/store/nfc_store.h`, `nfc_store.c` (minimal envelope)
- Modify: `src/nfc/reader/nfc_reader_engine.c` — poller walk after session active
- Modify: `src/nfc/reader/nfc_reader_shell_cmds.c` — `nfc reader clone`

**Exit tests:**
- `nfc reader scan` → session active → `nfc_reader_session_transceive` works (tag present)
- `nfc reader clone tag1` → valid `.card` hex blob
- Unit: `ndef` serialize round-trip ztest
- `west twister -T "$REPO/tests/unit/..." -t ci_unit`

**Commit:** `nfc/protocols: ndef poller and reader clone (Gate 2)`

**Not in Gate 2:** listeners, router, applets emulate.

---

## Gate 3 (C) — listen path: router + `nfc_stack` + PN7160 CE

**Scope:** `framing/`, `router/aid_router`, `nfc_stack.c`, `protocols/ndef/ndef_listener.c`; wire PN7160 + NFCT `on_apdu` to router.

**Files (create):**
- `src/nfc/framing/apdu_assembler.c` (if not folded into router)
- `src/nfc/router/aid_router.c`, `service.h`
- `src/nfc/nfc_stack/nfc_stack.c`, `nfc_stack.h`
- `src/nfc/protocols/ndef/ndef_listener.c`

**Exit tests:**
- PN7160 listen: external reader SELECT NDEF AID → READ BINARY CC/NDEF → valid SW
- Stats: `apdu_assembled_count` > 0, `apdu_dropped_no_consumer` == 0 when router registered

**Commit:** `nfc/stack: ndef listener and aid router (Gate 3)`

---

## Gate 4 (D) — applets emulate + verify on PN7160

**Scope:** `applets/clone`, `applets/emulate`, `applets/verify` shells; full clone → emulate → verify on same PN7160.

**Files (create):**
- `src/nfc/applets/nfc_applet_clone.c`, `nfc_applet_emulate.c`, `nfc_applet_verify.c`

**Exit tests:**
- `nfc clone` → `nfc emulate` → `nfc verify` → **PASS** on PN7160 hardware
- QEMU/build-only acceptable for CI; HIL for sign-off

**Commit:** `nfc/applets: PN7160 emulate and verify loop (Gate 4)`

---

## Gate 5 (E) — NFCT port + cross-backend verify

**Scope:** Same `.card` on NFCT emulate; re-verify on PN7160 reader.

**Files:**
- Modify: `src/nfc/run/` profile selection (PN7160 reader vs NFCT listen)
- Ensure `overlay-nfct-stack.conf` + `overlay-pn7160-stack.conf` documented in run path

**Exit tests:**
- Load `.card` → NFCT emulate → PN7160 verify **PASS**
- No concurrent poll+listen on same controller

**Commit:** `nfc/run: NFCT emulate with PN7160 verify (Gate 5)`

---

## Backlog F — per-protocol expansion

After Gate 5 green. See [`NFC_STACK_PLAN.md`](../NFC_STACK_PLAN.md) backlog table.

| Priority | Module | Notes |
|----------|--------|-------|
| F1 | `protocols/ultralight/` | Poller page READ; listener via ndef T4 adapter |
| F2 | `protocols/classic/` | PN7160 only; clone-only |
| F3 | `protocols/desfire/`, `emv/`, `aliro/` | Post–Gate 5; PICC raw lane |
| F4 | `protocols/felica/`, `iso15693_3/` | Defer; clone-only |
| F5 | HAL gaps | `tech_mask`, ATQA/SAK in `tag_info`, FeliCa discovery table |

One commit per protocol module when started; surgical review per [`NFC_PROTOCOLS_COOKBOOK.md`](../NFC_PROTOCOLS_COOKBOOK.md).

---

## Remaining HAL gaps (post–Step A)

| Gap | Owner | When |
|-----|-------|------|
| `tech_mask` ignored in PN7160 `discover_start` | HAL | Backlog / FeliCa |
| `tag_info` missing ATQA/SAK/ATS/ATQB | HAL + driver doc | Gate 2 poller detect |
| FeliCa / NFC-F discovery | HAL | Backlog F4 |
| 255 B READ BINARY chunking | `protocols/ndef` | Gate 2 |
| PN7160 `set_uid` during listen | HAL | Gate 3 |

---

## Self-review

| Spec requirement | Task |
|------------------|------|
| Step A before Gate 2 | A1–A4 |
| Sequential gates B→E | Gates 2–5 sections |
| One commit per sub-step | Commit messages per section |
| Driver frozen | No `modules/nfc_pn7160/` NCI changes in A–E |
| Ultralight = T4 adapter | Backlog F1 |
| Classic PN7160 only | Backlog F2 |
| PICC for ISO-DEP listeners | A3, Gate 3 |

No placeholders in executable steps A1–A3 (completed). Gates 2–5 list concrete file paths and exit commands.
