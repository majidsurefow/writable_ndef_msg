# NFC Stack Harmonization — Master Plan

**Status:** DRAFT — awaiting user approval (Phase 0). No code or docs touched until sign-off (§12).
**Branch:** `nfc-stack`
**Authority:** [`NFC_STACK_CONVENTIONS.md`](../NFC_STACK_CONVENTIONS.md) (LOCKED) · [`NFC_KCONFIG_ARCHITECTURE.md`](NFC_KCONFIG_ARCHITECTURE.md) · [`NFC_SHELL_KCONFIG_AUDIT.md`](NFC_SHELL_KCONFIG_AUDIT.md)
**Produces:** one `CONTEXT.md` per layer/protocol/applet + per-phase findings file + final [`../NFC_STACK_ARCHITECTURE.md`](../NFC_STACK_ARCHITECTURE.md).

This is the spec to agree on. It defines *what* the audit checks, *how* each doc looks, *what order* the layers are audited, and *which agent* runs each phase. Execution starts only after §12 is signed.

---

## 1. Goals & non-goals

**Goals — what "done" means:**

- Every layer dir (`modules/nfc_pn7160`, `src/nfc/*`, each `protocols/*`, each applet, `tests/common`) has a `CONTEXT.md` matching the §3/§4/§9 templates — concise, current, cross-linked.
- A top-level `NFC_STACK_ARCHITECTURE.md` assembled from those `CONTEXT.md` files: ASCII block diagram + mermaid data flow + HAL/profile/overlay/test matrices.
- Audit findings per layer (§5 checklist) with a single rolled-up `NFC_HARMONIZATION_FINDINGS.md`: buffer-bound risks, silent drops, error-propagation gaps, Kconfig↔CMake mismatches, shell leakage (per [`NFC_SHELL_KCONFIG_AUDIT.md`](NFC_SHELL_KCONFIG_AUDIT.md)), and tests that pass but prove nothing.
- Test fidelity verified: mock behavior matches real behavior; fixtures ↔ goldens aligned; tier minimums met (§6).
- Zero behavior change required to *pass* the program — findings are recorded; fixes are separate, scoped follow-ups (see §1 non-goals).

**Non-goals — explicitly out of scope:**

- Porting/relicensing GPL code (Flipper et al.) into the tree. Behavioral reference only.
- Wholesale Flipper macro / data-generator import.
- Kconfig Phase 2 (overlay renames) and Phase 3 (test `rsource` unification) — deferred per [`NFC_KCONFIG_ARCHITECTURE.md`](NFC_KCONFIG_ARCHITECTURE.md) §11.
- Landing the §remediation from the shell audit — recorded as findings, fixed later.
- New protocols, new HAL backends, new product features.
- HIL execution (Gates 2–5) — parallel track, not gated by this program (§11).

---

## 2. Layer stack — audit order (bottom-up)

Audit bottom-up: a layer's `CONTEXT.md` may reference invariants of layers below it, so those must be done first.

| # | Layer dir | Depends on (below) | Feeds (above) |
|---|-----------|--------------------|----------------|
| 1 | `modules/nfc_pn7160` | hardware / DT | `src/nfc/hal` |
| 2 | `src/nfc/hal` | module (PN7160), nrfxlib | framing, reader |
| 3 | `src/nfc/framing` | hal (`net_buf` APDU) | router |
| 3 | `src/nfc/router` | framing (`nfc_apdu_t`) | protocols, nfc_stack |
| 4 | `src/nfc/reader` | hal, poller registry | applets, protocols (pollers) |
| 5 | `src/nfc/nfc_stack` + `src/nfc/run` | hal, framing, router, store | applets |
| 6 | `src/nfc/store` | protocols (serialize vtable) | nfc_stack, applets |
| 7 | `src/nfc/protocols/<each>` | router (`service.h`), store, reader registry | reader, nfc_stack, store |
| 8 | `src/nfc/applets` | reader, store, nfc_stack, protocols | shell |
| 9 | `tests/common` + `tests/unit/*` | every layer above | CI |

Protocols (step 7), audit order: `ndef` → `ultralight` → `classic` → `felica` → `iso15693_3` → `slix` → `desfire` → `emv` → `aliro` (model-only first, then crypto/FSM ones last). `ndef` first — every other protocol and the store envelope reference it.

---

## 3. Per-directory doc template

**Filename: `CONTEXT.md`** (one decision, applied everywhere; existing `tests/**/README.md` files are left untouched). Max ~40–60 lines. No "this module provides…", no filler.

```markdown
# <dir> — CONTEXT

**Layer:** <stack position> · **Lifecycle:** <full|minimal per CONVENTIONS §2>

## Purpose
<2 sentences max: what it owns, what it does NOT own.>

## Key files
| File | Owns |
|------|------|
| x.c  | … |

## Kconfig
| Symbol | Default | Effect |
|--------|---------|--------|
| CONFIG_NFC_… | … | … |

## Deps
- Upstream (calls down into): …
- Downstream (calls up via callback): … (CONVENTIONS §3 boundary)

## Invariants & safety
- Buffers: <pool/static, sizing symbol, oversize handling — CONVENTIONS §5>
- Lifetimes: <who owns net_buf ref / when freed>
- Concurrency: <thread(s); -EBUSY while STARTED; atomic vs spinlock — §6/§7>
- Silent-drop counters: <named stats>

## Tests that prove it
| Test | Tier | Proves |
|------|------|--------|

## Shell
<pointer to <module>_shell_cmds.c + Kconfig gate; do not duplicate logic>
```

---

## 4. Per-protocol doc template

`protocols/<name>/CONTEXT.md` — same shape as §3 plus protocol specifics:

```markdown
## Roles
- Poller: <file> (Tier B) · Listener: <file or "skip native — ndef T4 adapter">
## Wire/spec
- <spec ref; emulatable? clone-only?>
## Capacity symbols
| CONFIG_NFC_<P>_* | default | bound it protects |
## Fixtures ↔ goldens
- <fixtures dir> ↔ <store golden name> (§6 alignment)
```

---

## 5. Audit checklist (run per layer, recorded in findings file)

| # | Category | Check | Authority |
|---|----------|-------|-----------|
| A | Buffer bounds | Static/FIXED pools only; every assemble path bounds-checked; oversize → SW `6700` + counter, never `__ASSERT` | CONVENTIONS §5 |
| B | Error propagation | Every failure returns errno (§9) or bumps a named stat; no swallowed returns; no silent `break`/`continue` on error | CONVENTIONS §6, §9 |
| C | Session / state machine | Poll vs listen paths separated; `-EBUSY` while STARTED; field-off resets session; profile switch deferred to next field-off | CONVENTIONS §7, §8 |
| D | Kconfig ↔ CMake | Every `*.c` gated in CMake matches its Kconfig symbol; no source compiled without its gate; capacity ints have defaults | KCONFIG_ARCH A.x |
| E | Shell leakage | No `shell.h` / `shell_*` outside `*_shell_cmds.c` gated by `*_SHELL` (depends on SHELL) | SHELL_AUDIT |
| F | Test fidelity | Mock matches real (session_mock vs HAL; response_spy vs send_response); negative paths asserted; no test that only asserts `== 0` of a no-op | §6 |
| G | Fixture ↔ golden | `tests/fixtures/<p>/*` decode to the same bytes as `tests/fixtures/store/<Card>.card.bin`; goldens regenerable | §6 |

Each finding: `severity (drop/leak/bound/silent) · file:line · category · one-line fix sketch`. No fix applied in-phase.

---

## 6. Test harmonization rules

**Tier minimums** (per CONVENTIONS / cookbook §14; Tier A model, B poller, C listener, D HIL, E applet):

| Protocol class | Tier A | Tier B | Tier C | Notes |
|----------------|--------|--------|--------|-------|
| Full emulatable (ndef, desfire, emv, aliro) | ≥12 | ≥12 | ≥12 | C native |
| Clone-only (ultralight, classic, felica, slix, iso15693_3) | ≥8 | ≥8 | via ndef T4 adapter | skip native listener |

- **`nfc_reader` (Tier E):** both `store` and `store_ram` scenarios must run (`OVERLAY_CONFIG=overlay-store-ram.conf`); store-RAM slot/clone/verify roundtrip asserted, not just init.
- **Virtual loopback eligibility:** a protocol qualifies for `tests/common/nfc_virtual_loopback` only if poller+listener both exist natively (ndef, desfire, emv, aliro). Clone-only protocols use `nfc_session_mock` script fixtures (`*.inc`) — record in each protocol `CONTEXT.md`.
- **"Passes but proves nothing" detection:** flag tests that (1) assert only the return code of a stubbed function, (2) feed a mock that returns the expected output unconditionally, (3) have no negative/oversize/`-EBUSY` case, (4) use a golden generated by the same code under test (circular).

---

## 7. Phase breakdown (execution work packages)

Each phase = one agent work package. Inputs are read-first. Outputs are `CONTEXT.md` files + appended findings. Verify = twister + build before phase closes.

| Phase | Scope (dirs) | Inputs (read first) | Outputs | Verify | Effort | Model |
|-------|--------------|---------------------|---------|--------|--------|-------|
| **0** | this plan | — | this file + arch stub | user sign-off (§12) | S | (done) |
| **1** | `modules/nfc_pn7160`, `src/nfc/hal` | CONVENTIONS §2/5/7, SHELL_AUDIT | 2 `CONTEXT.md` + findings; **sets the CONTEXT.md gold standard** all later phases copy | `twister -T modules/nfc_pn7160/tests -t ci_unit`; build `overlay-pn7160-hal.conf` | L | **Opus** (orchestrator — defines pattern) |
| **2** | `src/nfc/framing`, `src/nfc/router` | Phase 1 docs, CONVENTIONS §3/§4 | 2 `CONTEXT.md` + findings | `twister -T tests/unit/nfc_apdu_asm -t ci_unit` | M | Opus |
| **3** | `src/nfc/reader` | Phase 1–2 docs | 1 `CONTEXT.md` + findings | `twister -T tests/unit/nfc_reader -t ci_unit` | M | Composer |
| **4** | `src/nfc/nfc_stack`, `src/nfc/run` | Phase 1–3 docs, CONVENTIONS §3/§7/§8 | 2 `CONTEXT.md` + findings | build `overlay-pn7160-listen.conf` + `overlay-nfct-stack.conf` | M | **Opus** (orchestration-heavy) |
| **5** | `src/nfc/store` | Phase 4 docs, CONVENTIONS §3 (store boundary) | 1 `CONTEXT.md` + findings | `twister -T tests/unit/nfc_reader -t ci_unit` (store + store_ram) | M | Composer |
| **6** | `src/nfc/protocols/*` (9) | Phase 5 docs, §4 template, cookbook §14 | 9 `CONTEXT.md` + findings; 1 protocol parallel subagent each | `twister -T tests/unit/nfc_{ndef,desfire,emv,aliro,...} -t ci_unit` | L | Opus orchestrator + **Composer** per-protocol subagents |
| **7** | `src/nfc/applets` | Phase 3/5/6 docs, SHELL_AUDIT, applet API plan (§11) | 5 applet `CONTEXT.md` + findings | build reader + listen overlays; `twister -T tests/unit/nfc_reader` | M | Opus (applet API layering) |
| **8** | `tests/common`, `tests/unit/*` | all CONTEXT.md, §6 rules | `tests/common/CONTEXT.md` + test-fidelity findings | full `twister -T tests/unit -t ci_unit -p qemu_cortex_m3` | L | **Opus** (fidelity judgment) |
| **9** | assemble | all CONTEXT.md + findings | `NFC_STACK_ARCHITECTURE.md` (full) + `NFC_HARMONIZATION_FINDINGS.md` | doc cross-link lint; diagrams render | M | Opus |

Effort: S < ½ day, M ≈ 1 day, L ≈ 2 days agent time.

---

## 8. `NFC_STACK_ARCHITECTURE.md` outline (assembled in Phase 9)

1. **Block diagram (ASCII)** — HAL → framing → router → {protocols} ↔ store, reader engine sidecar, nfc_stack orchestrator.
2. **Data flow (mermaid)** — inbound ISR→pool→fifo→WQ→framing→router→service→response (CONVENTIONS §5/§7); reader scan→poller→store path.
3. **HAL profiles** — PN7160 vs NRFX capability matrix; role ceilings.
4. **Protocol registry** — table from the 9 `protocols/*/CONTEXT.md` (poller/listener/emulatable/capacity).
5. **Store envelope** — blob layout, CRC, serialize/deserialize vtable, golden set.
6. **Applet service layer** — scan/read/emulate/verify/loop; reference applet API separation plan (§11).
7. **Test pyramid** — Tier A/B/C/E counts + Tier D HIL pointer.
8. **Overlay matrix** — profiles × overlays (from KCONFIG_ARCH Appendix B).

---

## 9. Per-applet doc template

One `CONTEXT.md` per applet (scan, read, emulate, verify, loop; plus shared `policy`):

```markdown
# nfc_applet_<x> — CONTEXT
**Entry:** nfc_applet_<x>(…) · **Caller:** shell (`nfc <x>`) / loop
## Flow
<scan→read→store→verify pipeline position; 3 lines>
## Deps
- reader/store/stack APIs used; protocols required at runtime
## Safety
- Shell gating (SHELL_AUDIT finding ref); -EBUSY during session; buffer reuse
## Tests
- Tier E case(s) in tests/unit/nfc_reader
```

---

## 10. Agent orchestration pattern

- **Architect (Opus) per phase** reviews the previous phase's `CONTEXT.md` + findings *before* starting, to keep section headers, table columns, and cross-link style identical. Phase 1 is the canonical template; Opus owns the architecture-heavy phases (1, 4, 6-orchestrator, 8, 9).
- **Subagent loop per dir:** `explore` (read-only map) → `audit` (run §5 checklist, write findings) → `doc` (write `CONTEXT.md` from template) → `twister verify` (run the phase's command, paste PASS/FAIL counts). Doc-only sweeps (protocols, reader, store) run on **Composer** subagents under the Opus orchestrator.
- **Consistency rules:** identical H2 headers (`Purpose / Key files / Kconfig / Deps / Invariants & safety / Tests that prove it / Shell`); relative links between adjacent layers; every Kconfig symbol cited with `CONFIG_` prefix; findings use the §5 one-line format.
- **Model fit:** **Opus** = architecture + fidelity judgment + orchestration (pattern-setting, nfc_stack, test fidelity, assembly). **Composer** = template-following doc sweeps + mechanical audit on a single dir. **Fable** = optional cross-link/consistency lint pass at end of each phase (cheap, high-volume diff review). Never let Composer set a new pattern — it copies Phase 1.

---

## 11. Sequencing vs other tracks

| Track | When | Interaction |
|-------|------|-------------|
| HIL Gates 2–5 | parallel, anytime | independent; findings may inform HIL but do not block it |
| Applet API refactor (shell/applet layering, SHELL_AUDIT remediation) | **during Phase 7** | Phase 7 *documents* the target separation and records leakage findings; the actual refactor lands as a follow-up PR after the plan, not inside the audit |
| Kconfig Phase 2 (overlay renames) | **deferred** | not in this program; arch doc uses current overlay names |
| Kconfig Phase 3 (test `rsource`) | **deferred** | Phase 8 records the duplication as a finding only |

---

## 12. Approval checklist (sign off before Phase 1 runs)

- [ ] **Doc name = `CONTEXT.md`** for all layer/protocol/applet docs (not README); existing test READMEs untouched.
- [ ] **Audit is read-only** — findings recorded, *no* code fixes inside any phase (fixes are separate scoped PRs).
- [ ] **Bottom-up order** (§2) and the **9-phase breakdown** (§7) accepted, incl. model assignments.
- [ ] **Findings live in** per-phase files + one rolled-up `NFC_HARMONIZATION_FINDINGS.md`.
- [ ] **Tier minimums & fidelity rules** (§6) are the acceptance bar for the test phase.
- [ ] **Out of scope** (§1 non-goals) confirmed: no GPL port, no Kconfig Phase 2/3, no applet refactor inside audit.
- [ ] **HIL stays parallel** (§11) — this program does not gate hardware validation.
