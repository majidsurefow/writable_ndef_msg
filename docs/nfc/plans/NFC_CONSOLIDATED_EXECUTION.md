# NFC Consolidated Execution Plan (Single Operational Plan of Record)

**Branch:** `nfc-stack` · **Status:** DRAFT — awaiting approval (§6).
**Supersedes (as the operational plan):** this doc merges and stages the work
previously scattered across:
- [`NFC_HARMONIZATION_MASTER_PLAN.md`](NFC_HARMONIZATION_MASTER_PLAN.md) — bottom-up audit + applet/shell deconvolution + test gating (the *authority* on layering & acceptance philosophy; this doc is its execution schedule).
- [`NFC_HIL_AND_POLISH_PLAN.md`](NFC_HIL_AND_POLISH_PLAN.md) — HIL Gates 2–5 + doc-drift + CI polish.
- [`NFC_APPLET_API_LAYERING.md`](NFC_APPLET_API_LAYERING.md) — result-struct / per-applet detail (reference).
- [`NFC_SHELL_KCONFIG_AUDIT.md`](NFC_SHELL_KCONFIG_AUDIT.md) — the four shell leaks (reference).
- [`NFC_KCONFIG_ARCHITECTURE.md`](NFC_KCONFIG_ARCHITECTURE.md) — profile `imply` chains (Phase 1 landed; Phases 2/3 deferred).

Those remain as **reference detail**. The *staged plan you execute* is here.
New end-user guides produced alongside this plan:
[`../NFC_HIL_PROTOCOL_GUIDE.md`](../NFC_HIL_PROTOCOL_GUIDE.md),
[`../NFC_SHELL_APPLETS.md`](../NFC_SHELL_APPLETS.md).

---

## 1. Executive summary

The NFC stack is **green on QEMU and feature-complete in logic**; the remaining
work is (a) making *what compiles* and *what is tested* provably the same thing
(Kconfig + test gating), (b) finishing the shell→service decoupling so the stack
is headless below Layer 2, (c) documenting every layer and command, and (d)
proving the RF path on silicon (HIL). This plan stages all of that into one
ordered sequence with hard entry/exit gates.

**Baseline measured at plan time** (NCS v3.2.4, `zephyr` toolchain):

| Suite | Platform | Result |
|-------|----------|--------|
| `tests/unit/nfc_reader` (`.store` + `.store_ram`) | qemu_cortex_m3 | **PASS** — 2/2 configs, **97/97** cases |

Kconfig profile `imply` chains (reader / CE / lab) are landed; overlays are
already reduced to one profile line each.

### 1.1 Current applet inventory (top-level `nfc` commands)

| Command | Action | Profile / overlay | Kconfig gate |
|---------|--------|-------------------|--------------|
| `nfc scan [t]` | Discover tag; print UID/protocol/tech; hold session | Reader (`overlay-pn7160-stack.conf`), CE | `NFC_APPLETS_SHELL` |
| `nfc read <slot> [t]` | One-shot scan + clone to slot | Reader, CE | `NFC_APPLETS_SHELL` + `NFC_STORE` |
| `nfc reader clone <slot>` | Alias of `nfc read` (LOCKED) | Reader, CE | `NFC_READER_SHELL` |
| `nfc verify <slot> [t]` | Poll tag + diff vs stored slot → PASS/FAIL | Reader, CE | `NFC_APPLETS_SHELL` |
| `nfc emulate <slot> [ndef]` | Load slot + start listen (clone-only refused) | CE / reader+`overlay-pn7160-listen.conf` | `NFC_APPLETS_SHELL` + `NFC_LISTEN_STACK` |
| `nfc emulate golden <name>` | Import compiled-in golden + emulate | CE | + `NFC_STORE_RAM` + `NFC_STORE_GOLDENS` |
| `nfc loop <slot> [t]` | Orchestrated read→emulate→verify | CE / reader+listen | `NFC_APPLETS_SHELL` + `NFC_LISTEN_STACK` |

Debug primitives (`nfc reader`, `nfc stack`, `nfc store`, `nfc_transport`,
`pn7160`) are inventoried in full in [`../NFC_SHELL_APPLETS.md`](../NFC_SHELL_APPLETS.md).

### 1.2 Program tracks → where they land

| Track | Theme | Lands in phases |
|-------|-------|-----------------|
| 1 | Kconfig truth audit (imply ↔ overlay ↔ test gating) | P1, P5 |
| 2 | Shell/applet decoupling (master plan Part C) | C1 (scan/read/store), C2 (loop/adapters) |
| 3 | Per-subdir docs (`CONTEXT.md` decision §3.1) | P2–P4 (audit phases) |
| 4 | Protocol parity audit (Aliro vs NCS access-control; Flipper matrix) | P4 |
| 5 | HIL protocol guide (NEW doc) | P0 (done) + bench track |
| 6 | Shell usage guide (NEW doc) | P0 (done) |
| 7 | QEMU green (full twister matrix + CI alignment) | P5, P6 |

---

## 2. Definition of done

**Software ship-ready (no hardware):**
- [ ] **Kconfig truth:** every `*.c` is CMake-gated to its `CONFIG_*` symbol;
  `tests/unit/nfc_reader` no longer compiles all 9 protocols unconditionally
  (wrapped per `CONFIG_NFC_PROTOCOL_X`); profile→compiles→may-test mapping holds.
- [ ] **Test truth:** every twister scenario = profile + overlay + explicit tier
  allowlist; a reader scenario never runs a native-listener tier; headless
  applet tier (`log==NULL`) exists; a `CONFIG_SHELL=n` reader build links.
- [ ] **Shell truth:** no `shell.h` / `shell_*` below Layer 2; the four leaks
  (`nfc_store.c`, `nfc_store_ram.c`, `nfc_applet_scan.c`, `nfc_applet_loop.c`)
  are closed; `nfc_applet_service.h` is the headless API.
- [ ] **Docs:** every `src/nfc/**` dir has a `CONTEXT.md` (§3.1 decision); the two
  new guides exist and match the code; doc-drift fixes (HIL plan §3) applied;
  CI doc = `twister.yaml`.
- [ ] **QEMU green:** full `west twister -T tests/unit -t ci_unit -p qemu_cortex_m3`
  passes; `nfc_apdu_asm` is in the CI matrix.

**Product ship-ready (adds hardware):**
- [ ] HIL Gates 2–5 PASS on silicon per [`../NFC_HIL_PROTOCOL_GUIDE.md`](../NFC_HIL_PROTOCOL_GUIDE.md),
  with captured logs + `nfc_transport stats` (`apdu_assembled > 0`,
  `apdu_drop_cons == 0`, `nfc verify` PASS, `nfc loop` PASS, cross-backend Gate 5).

---

## 3. Staged phases

Each phase: **entry gate → work → exit gate (verify command)**. Read-only audit
phases produce `CONTEXT.md` + findings; code phases land scoped commits. Models
follow the master plan (Opus for boundary-sensitive layers, Composer for
mechanical per-protocol work).

### 3.1 Decision — `CONTEXT.md` vs `README.md` (Track 3)

**Resolved: use `CONTEXT.md` in every `src/nfc/**` subdir; test dirs keep their
existing `README.md`.**

Rationale: the harmonization master plan's approval checklist already **locks**
`CONTEXT.md` as the per-layer doc name, and `NFC_STACK_CONVENTIONS.md` is the
cited authority. `CONTEXT.md` is the agent-/maintainer-facing "what this layer
owns, its invariants, its Kconfig, its tests" doc — distinct in purpose from a
human onboarding README. Renaming to README would (a) contradict a locked
decision and (b) collide with the `tests/**/README.md` files that already serve
the human-onboarding role. So: **production source dirs → `CONTEXT.md`**;
**`tests/**` dirs → keep `README.md`** (and fix their drift, HIL plan §3 D4).

---

| Phase | Track(s) | Entry gate | Work | Deliverables | Exit gate (verify) | Model |
|-------|----------|-----------|------|--------------|--------------------|-------|
| **P0** | 5,6 | — | This plan + two end-user guides (HIL, shell) + master-plan pointer | `NFC_CONSOLIDATED_EXECUTION.md`, `NFC_HIL_PROTOCOL_GUIDE.md`, `NFC_SHELL_APPLETS.md` | Approval (§6); baseline twister green (done) | (done) |
| **P1** | 1 | P0 approved | Kconfig truth audit: verify each `NFC_PROFILE_*` imply chain matches its overlay; confirm tests gated on the Kconfig the profile compiles; document the `nfc_reader` unconditional-compile divergence; record pass/fail baseline | `KCONFIG_TRUTH_FINDINGS` (append to harmonization findings); confirmed profile→compiles→may-test table | `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` green; findings written | Opus |
| **P2** | 3 | P1 | Bottom-up `CONTEXT.md` for HAL + module + framing + router (gold-standard template) | `modules/nfc_pn7160/CONTEXT.md`, `src/nfc/hal/CONTEXT.md`, `framing/`, `router/` | `nfc_apdu_asm` + `pn7160_tml` twister green; docs lint | Opus |
| **P3** | 3 | P2 | `CONTEXT.md` for reader engine, nfc_stack, run | `src/nfc/reader/`, `nfc_stack/`, `run/CONTEXT.md` | reader twister green; listen+nfct overlay **builds** | Composer/Opus |
| **P4** | 3,4 | P3 | `CONTEXT.md` per protocol (9) + **protocol parity audit**: Aliro listener vs NCS door-lock/access-control patterns (local `aliro/`, `wave5-aliro.md`); Flipper `.nfc` fixture↔golden↔loopback coverage matrix; per-protocol "QEMU proves vs HIL must prove" | 9× `protocols/*/CONTEXT.md`; `NFC_PROTOCOL_PARITY_MATRIX` (folded into HIL guide §4) | per-protocol twister (all tiers) green | Opus orch + Composer |
| **C1** | 2 | P3 (store touched) | **Deconvolution part 1** (master plan Part C.4 steps 1–3): add `nfc_applet_service.h`; `scan_get_result` + `get_card_meta`, move `nfc_applet_scan_print` body to adapter; fix `nfc_store_default_save` (no-shell stub) + move `nfc_store_ram.c` shell include under `NFC_STORE_RAM_SHELL`; clean `nfc_applet.h` | service header + scoped code commits | `nfc_reader` store+store_ram green **+** reader `CONFIG_SHELL=n` compile | **Opus** |
| **C2** | 2 | C1 landed, no HIL `loop` sign-off in flight | **Deconvolution part 2** (Part C.4 step 4): `nfc_applet_loop_run` headless + log cb; reduce `cmd_nfc_loop` to thin adapter; finalize all `*_shell_cmds.c` as the only L2 | scoped code commits | reader + CE builds with `SHELL=y` **and** `SHELL=n` | **Opus** |
| **P5** | 1,7 | P4, C1 | **Test gating fix** (master plan B.5/E): wrap each protocol block in `nfc_reader/CMakeLists.txt` under `if(CONFIG_NFC_PROTOCOL_X)`; split `prj.conf` into profile-scoped fragments; add headless applet tier (`scan_get_result`/`get_card_meta`/`loop_run`, `log==NULL`) + `nfc_reader.shell_off` scenario | gated CMake + new scenarios | per-protocol + `nfc_reader` twister green; `SHELL=n` scenario builds | **Opus** |
| **P6** | 7 | P5, C2 | **QEMU green + CI alignment:** full twister matrix; add `tests/unit/nfc_apdu_asm` to `twister.yaml` `UNIT_DIRS`; apply doc-drift fixes (HIL plan §3 D1–D7); reconcile `CI_TESTING.md` ↔ `twister.yaml` | green report; CI + doc fixes | `west twister -T tests/unit -t ci_unit -p qemu_cortex_m3` **all green**; `nfc_apdu_asm` in CI | Opus |
| **P7** | — | P6 | Assemble `NFC_STACK_ARCHITECTURE.md` + roll up `NFC_HARMONIZATION_FINDINGS.md` | architecture doc + findings rollup | cross-link lint; diagrams render | Opus |
| **HIL** | 5 | image builds (any time after C-phases stable) | Bench Gates 2→3→4→5 per HIL guide | captured logs + stats per gate | Gate PASS recorded in sequential plan | human |

Effort: S < ½ day, M ≈ 1 day, L ≈ 2 days agent time. P4/C1/C2/P5 are L; others M.

---

## 4. Doc map (which `.md` lives where, and its role)

| Doc | Role | Status after this plan |
|-----|------|------------------------|
| `plans/NFC_CONSOLIDATED_EXECUTION.md` (this) | **Operational plan of record** — staged phases | active |
| `plans/NFC_HARMONIZATION_MASTER_PLAN.md` | Authority on layering, acceptance philosophy, audit categories | reference (pointer added → here) |
| `plans/NFC_HIL_AND_POLISH_PLAN.md` | HIL runbook detail + doc-drift/CI checklist | reference (folded into P6 + HIL track) |
| `plans/NFC_APPLET_API_LAYERING.md` | Result structs + per-applet mapping | reference |
| `plans/NFC_SHELL_KCONFIG_AUDIT.md` | The four shell leaks + gating table | reference |
| `plans/NFC_KCONFIG_ARCHITECTURE.md` | Profile imply chains, overlay matrix, symbol inventory | reference (Phase 1 landed) |
| `NFC_HIL_PROTOCOL_GUIDE.md` (NEW) | Bench procedure: flash lines, per-protocol PASS criteria, gates | **active end-user guide** |
| `NFC_SHELL_APPLETS.md` (NEW) | Complete shell command inventory (applets + primitives) | **active end-user guide** |
| `NFC_STACK_ARCHITECTURE.md` | Final assembled architecture (block + data flow + matrices) | produced in P7 |
| `src/nfc/**/CONTEXT.md` | Per-layer ownership/invariants/Kconfig/tests | produced P2–P4 |
| `tests/**/README.md` | Human test-dir onboarding | kept, drift fixed (P6) |

---

## 5. Sequencing & HIL parallelism

```
P0 ─approve─▶ P1 ─▶ P2 ─▶ P3 ─▶ P4 ─▶ P5 ─▶ P6 ─▶ P7
                         └▶ C1 ─▶ C2 ─┘
HIL Gates 2→3→4→5 ───── parallel bench track (no agent gate) ─────▶
```

- **Read-only audit phases (P1–P4)** and **both new guides** are parallel-safe
  with HIL — they touch no shipping code path.
- **C1 store default-save fix** touches the reader-only `nfc read` path → land
  **between HIL cycles** (no `read` sign-off in flight).
- **C2 loop deconvolution** restructures `nfc loop` → **not** during a HIL `loop`
  sign-off.
- **scan/read extract (C1 first half)** is parallel-safe — externally observable
  output is identical (strings just relocate to the adapter).
- **HIL Gates 2–5** run on the **current** overlays at any time; this program
  does not gate hardware validation, and hardware validation does not gate the
  software phases except the two "caution" items above.

---

## 6. Approval checklist (sign off before P1)

- [ ] **One operational plan of record** — this file stages the harmonization,
  HIL/polish, applet-layering, shell-audit, and Kconfig work. The other plans
  remain reference. Confirmed.
- [ ] **`CONTEXT.md` for production dirs, `README.md` kept in `tests/**`** (§3.1
  decision + rationale) accepted.
- [ ] **Track integration** accepted: Kconfig audit (P1/P5), shell decoupling
  (C1/C2), per-dir docs (P2–P4), protocol parity (P4), HIL guide + shell guide
  (P0), QEMU green + CI (P6).
- [ ] **Test gating policy** accepted: every twister scenario = profile + overlay
  + explicit tier allowlist; `nfc_reader` per-protocol CMake gating fixed;
  headless applet tier + `SHELL=n` scenario added.
- [ ] **Shell decoupling lands as code** at C1/C2 (not a deferred orphan).
- [ ] **HIL stays a parallel bench track**; only C1 store-fix and C2 loop-rework
  serialize against HIL `read`/`loop` sign-offs.
- [ ] **Baseline accepted:** `nfc_reader` 2/2 configs, 97/97 cases green (QEMU).

---

## 7. First three phases at a glance (for the executor)

1. **P1 — Kconfig truth audit (read-only):** prove `NFC_PROFILE_READER/CE/LAB`
   imply chains match `overlay-pn7160-stack/-nfct-stack/-pn7160-hal.conf`; confirm
   the `nfc_reader` suite's unconditional 9-protocol compile is the divergence to
   fix in P5; write findings. Exit: reader twister green + findings.
2. **P2 — Gold-standard CONTEXT.md (HAL + module + framing + router):** establish
   the per-layer doc template bottom-up. Exit: `nfc_apdu_asm` + `pn7160_tml`
   green; docs lint.
3. **P3 — CONTEXT.md (reader engine + nfc_stack + run):** document the
   orchestration layer. Exit: reader twister green; listen + nfct overlays build.
