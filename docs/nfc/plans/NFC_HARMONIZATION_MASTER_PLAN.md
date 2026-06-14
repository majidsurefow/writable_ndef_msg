# NFC Stack Harmonization — Master Plan (Single Source of Truth)

**Status:** DRAFT — awaiting user approval (§Part G). No code or docs touched until sign-off.
**Branch:** `nfc-stack`
**Authority:** [`NFC_STACK_CONVENTIONS.md`](../NFC_STACK_CONVENTIONS.md) (LOCKED) · [`NFC_KCONFIG_ARCHITECTURE.md`](NFC_KCONFIG_ARCHITECTURE.md) (profiles, implemented) · [`NFC_PROTOCOLS_COOKBOOK.md`](../NFC_PROTOCOLS_COOKBOOK.md) §14 (test tiers, LOCKED)
**Produces:** one `CONTEXT.md` per layer/protocol/applet + per-phase findings + rolled-up [`NFC_HARMONIZATION_FINDINGS.md`](NFC_HARMONIZATION_FINDINGS.md) + final [`../NFC_STACK_ARCHITECTURE.md`](../NFC_STACK_ARCHITECTURE.md).

> **Execution pointer:** the **staged, cross-track operational plan of record** is now
> [`NFC_CONSOLIDATED_EXECUTION.md`](NFC_CONSOLIDATED_EXECUTION.md), which merges this plan
> with the HIL/polish, applet-layering, shell-audit, and Kconfig work into one ordered
> phase sequence (P0→P7 + parallel HIL). **This file remains the authority** on layering,
> the three truths, audit categories (D.4), and acceptance philosophy — the consolidated
> doc schedules *when* each part lands; this doc defines *what correct looks like*.

This is the **one** plan to agree on. It absorbs and supersedes:
- [`NFC_APPLET_API_LAYERING.md`](NFC_APPLET_API_LAYERING.md) — applet/shell layering (now Part C + Phases 5 & 8 here).
- [`NFC_SHELL_KCONFIG_AUDIT.md`](NFC_SHELL_KCONFIG_AUDIT.md) — shell-gating findings (now the Part C target + audit category E).

Those two stay as **reference detail**; the *plan of record* is here.

---

# Phase A — Applet foundation (LANDED 2026-06-14, prerequisite to Phase 1)

> **Status: LANDED.** Phase A is a **prerequisite that runs before** the
> bottom-up harmonization audit (Phases 1–9 / P1–P7). It locks the applet/shell
> layer model and lands the deconvolution code that the rest of the program
> assumed would happen "during the audit" — pulled forward so the audit phases
> start from a headless, gated baseline.

**What Phase A locked (authoritative applet model):**

| Layer | Owns | Shell? |
|-------|------|--------|
| L0 engines | reader engine, nfc_stack, store, HAL | no |
| L1 applets | scan / read / emulate / loop / check business logic — errno + result structs + optional log cb | **no** |
| L2 shell | `*_shell_cmds.c` adapters — argv parse + human print | the **only** `struct shell *` site |
| L3 app/SMF | future consumer of L1 | n/a |

**L1 product applets (4 + helpers):** Scan (`nfc_applet_scan`), Read
(`nfc_applet_read`), Emulate (`nfc_applet_emulate`), Loop (`nfc_applet_loop`),
with Check (`nfc_applet_verify`, internal to Loop / DK shell only) and Policy
(`nfc_applet_policy`, internal). **Not applets:** store, reader engine, stack,
HAL, golden import.

**L2 shell mapping (LOCKED):** `nfc scan start` / `nfc scan stop` → Scan
(continuous discovery + per-tag callback); `nfc read <slot>` → Read;
`nfc emulate <slot>` / `nfc emulate golden <name>` → Emulate;
`nfc loop <slot>` → Loop; `nfc check <slot>` → Check (**DK only**, was
`nfc verify`).

**DROPPED in Phase A:** `nfc reader clone` (use `nfc read`); `nfc verify` as a
product command (renamed to DK `nfc check`); blocking `nfc scan` (replaced by
`nfc scan start`/`stop` continuous discovery + callback).

**What landed (code):** `nfc_applet_service.h` (headless L1 API),
`nfc_applet_service.c` (card-meta helper), continuous-discovery scan,
`nfc_applet_loop_run` + thin `cmd_nfc_loop`, store default-save made inert
(no shell), `nfc_store_ram.c` shell include gated, `nfc reader clone` removed,
`nfc verify`→`nfc check`. All four shell leaks (Part C.1) are **closed**.

**Verification:** `nfc_reader` twister 2/2 configs, 97/97 cases green; reader,
reader+listen, and reader `CONFIG_SHELL=n` images build/link; no `shell.h` /
`shell_*` below L2.

**Phase A blocks nothing in the audit** — harmonization P1+ runs after Phase A
green (Part H). The Part C deconvolution work below is therefore **already
done**; the audit phases consume it rather than producing it.

---

# Part A — Vision & principles

## A.1 Layer stack

```text
L3  App / UI consumers      SMF · BLE GATT · LVGL · HIL harness · unit test
                              │  (errno + result structs only)
L2  Shell adapter            *_shell_cmds.c   (CONFIG_*_SHELL, depends on SHELL)
                              │  argv parse + human print — the ONLY place struct shell* lives
L1  Applet service (headless) nfc_applet_service.h: scan/read/emulate/verify/loop
                              │  structured results + errno + optional log cb. Never prints.
L0  Engine / stack / store / HAL
      reader engine · nfc_stack · framing · router · protocols ↔ store · HAL (PN7160/NFCT)
```

Audit and refactor run **bottom-up**: a layer's `CONTEXT.md` may cite invariants of layers below it, so lower layers are documented first.

## A.2 Three truths (the acceptance philosophy)

1. **Safety truth** — every assemble/serialize path is bounds-checked against a FIXED pool or capacity int; oversize → SW status (`6700`) + named counter, never `__ASSERT`/silent drop (CONVENTIONS §5/§6).
2. **Kconfig truth** — what compiles is determined by `NFC_PROFILE_*` + backend, never by a hand-listed source set. A `struct shell *` may appear only in a `*_shell_cmds.c` gated by `*_SHELL`. No source is built outside its Kconfig gate.
3. **Test truth** — a passing test must exercise the **production path the active profile compiles**, not a mock that returns the expected answer. Tests are **gated on the same Kconfig** as production: a suite only runs the tiers its profile/overlay enables.

## A.3 Goals / non-goals

**Done means:**
- Every layer/protocol/applet dir has a `CONTEXT.md` (templates in Part D).
- `NFC_STACK_ARCHITECTURE.md` assembled: block diagram + data flow + HAL/profile/overlay/test matrices, with explicit L0–L3 layering and the test pyramid.
- Findings rolled into `NFC_HARMONIZATION_FINDINGS.md` (bounds, silent drops, error-prop gaps, Kconfig↔CMake mismatches, shell leakage, "passes but proves nothing").
- **Applet/shell deconvolution landed** (Part C) — not deferred — because it is the precondition for headless Layer-1 tests (Part E).
- **Test gating model enforced** (Part B/E): every suite's tiers are Kconfig-gated and every twister scenario = profile + overlay + explicit tier allowlist; full twister rerun green per phase.

**Out of scope:** GPL port (Flipper behavioral reference only); new protocols/HAL backends/products; Kconfig Phase 2 overlay *renames* and Phase 3 test `rsource` (deferred per KCONFIG_ARCH §6 — but the **test-gating fixes in Part E are in scope**, distinct from the rename churn); HIL execution (Part H, parallel track).

---

# Part B — Kconfig → test gating model

The single most important correctness lever. Today's gap (see §B.5) is that `tests/unit/nfc_reader` compiles **all** protocol `.c` files unconditionally and `prj.conf` force-enables every protocol — so the suite never reflects what a real profile build (`NFC_PROFILE_READER`, `NFC_PROFILE_CARD_EMULATION`, lab) actually compiles. That is the root of the "tests pass but prove the wrong thing" skepticism.

## B.1 Profile → compiles → may-test mapping

| Profile (`CONFIG_*`) | Compiles (production) | Tests that may run |
|----------------------|------------------------|--------------------|
| `NFC_PROFILE_READER` | reader engine, store+RAM, applets (service+shell), **all poller** protocols; **no listen stack** | protocol Tier A+B (all 9), `nfc_reader` Tier E (store + store_ram), applet Layer-1 (headless) |
| `NFC_PROFILE_CARD_EMULATION` | listen stack (framing+router), store+RAM, applets, **emulate subset** (NDEF, Ultralight, DESFire, EMV, Aliro) | Tier A+C for emulate subset, virtual loopback (emulatable only), `nfc_reader` Tier E |
| `NFC_PROFILE_LAB` | HAL + role scaffold only | module/HAL tier only (`pn7160_tml`, `nfc_apdu_asm`); no protocol/store/applet tiers |

**Rule:** a tier may run **iff** the profile/overlay under test enables the Kconfig that compiles its production source. A reader-profile scenario must **not** run a native listener tier; a lab scenario must **not** run store tiers.

## B.2 Twister scenario design

Each twister scenario is a triple: **profile/conf + overlay + explicit tier allowlist**. No scenario silently inherits "all tiers."

```
scenario = {
  CONF_FILE | OVERLAY_CONFIG  →  selects profile + capacities
  CONFIG_<SUITE>_TEST_TIER_*  →  explicit allowlist of tiers built
  platform                    →  qemu_cortex_m3 (PN7160 emul) | native_sim (virtual)
}
```

Per-suite tier gates already exist (`NFC_<P>_TEST_TIER_POLLER`, `..._LISTENER`); Part E extends this pattern to (a) `nfc_reader` protocol selection and (b) profile-scoped scenarios.

## B.3 Source-gating rules (production + test CMake)

1. **Production:** every `*.c` is added in CMake under `if(CONFIG_<its symbol>)` — no unconditional protocol/store/applet source lists. (`nfc_reader` test CMake violates this today — §B.5 fix.)
2. **Test sources:** a tier's `.c` is built only under its `CONFIG_<SUITE>_TEST_TIER_*` gate (already true for protocol suites; extend to `nfc_reader`).
3. **In-code gates:** any test that touches a protocol must `#if IS_ENABLED(CONFIG_NFC_PROTOCOL_X)` around that protocol's assertions, OR the protocol's source must be CMake-gated out — never both compiled-in-and-unasserted.
4. **Shell gate:** `struct shell *` / `shell_*()` only in `*_shell_cmds.c` under `*_SHELL` (Part C enforces this in source + CMake).

## B.4 Anti-patterns to flag (audit category F)

- Test passes because a mock returns the expected output unconditionally (production decode path never run).
- Test asserts only `== 0` of a stubbed/no-op function.
- Golden generated by the same code under test (circular).
- No negative / oversize / `-EBUSY` case.
- Suite compiles a protocol the active profile would **not** ship, giving false coverage confidence.
- `nfc_reader` "store roundtrip" green while the profile build that ships that store path was never exercised headless.

## B.5 The concrete divergence to fix (in Part E)

| Symptom | File | Fix phase |
|---------|------|-----------|
| All 9 protocol `.c` compiled unconditionally | `tests/unit/nfc_reader/CMakeLists.txt` | Phase 6 (Part E) — wrap each protocol block in `if(CONFIG_NFC_PROTOCOL_X)` |
| `prj.conf` force-enables all protocols regardless of scenario | `tests/unit/nfc_reader/prj.conf` | Phase 6 — split into profile-scoped conf fragments |
| No headless applet (Layer-1) test exists | `tests/unit/nfc_reader/` | Phase 8 (post-deconvolution) — add `scan_get_result`/`get_card_meta`/`loop_run`/store-save-via-RAM tests with `log==NULL` |
| No `CONFIG_SHELL=n` build locks the shell gate | CI | Phase 8 — add reader-profile `SHELL=n` build scenario |

## B.6 Retest matrix (run at the close of every phase)

Each phase ends with a **profile-scoped** twister rerun, not a blanket "all tests":

| Phase closes | Twister command (scenario-scoped) |
|--------------|-----------------------------------|
| 1 (HAL/module) | `west twister -T modules/nfc_pn7160/tests -t ci_unit -p qemu_cortex_m3` |
| 2 (framing/router) | `west twister -T tests/unit/nfc_apdu_asm -t ci_unit -p qemu_cortex_m3` |
| 3 (reader engine) | `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3` (store + store_ram scenarios) |
| 4 (nfc_stack/run) | build `overlay-pn7160-listen.conf` + `overlay-nfct-stack.conf` |
| 5 (store + applet deconvolution) | `nfc_reader` (store+store_ram) **+** new reader-profile `SHELL=n` compile |
| 6 (protocols + test gating) | `west twister -T tests/unit/nfc_{ndef,ultralight,classic,felica,iso15693_3,slix,desfire,emv,aliro} -t ci_unit` |
| 7 (shell adapters) | reader + CE overlay builds with `SHELL=y` and `SHELL=n` |
| 8 (test harmonization) | **full** `west twister -T tests/unit -t ci_unit -p qemu_cortex_m3` + headless applet tier |
| 9 (assembly) | doc cross-link lint; diagrams render |

---

# Part C — Applet / shell deconvolution (integrated, not parallel)

This is the [`NFC_APPLET_API_LAYERING.md`](NFC_APPLET_API_LAYERING.md) work, folded into the bottom-up phases at the **store** and **applet** layers so headless tests (Part E) become possible. It **lands as code** (scoped commits), unlike the read-only audit of other layers.

> **LANDED in Phase A (2026-06-14).** All of Part C below is **done** — pulled
> ahead of the audit. The four leaks (C.1) are closed, the new Layer-1 API (C.2)
> exists as `nfc_applet_service.h`, and the Kconfig/CMake gating (C.3) is
> enforced. The audit phases (5 & 7) now *verify* this state instead of
> producing it; their "Part C" code columns are marked LANDED in Part D.

## C.1 The four leaks (from the shell audit) — ALL CLOSED in Phase A

| File | Leak | Fix |
|------|------|-----|
| `src/nfc/store/nfc_store.c` | `nfc_store_default_save()` is the **default** save cb and calls `shell_print`/`shell_fprintf`; treats `user_ctx` as `struct shell *`; unconditional `#include <shell.h>` | Replace default with inert no-shell stub (`-ENODEV` / `LOG_*`). Hex dump becomes a **shell-registered** cb (`nfc_applet_shell_save_cb`), never the store default. Drop the include. |
| `src/nfc/store/nfc_store_ram.c` | `#include <shell.h>` sits under `NFC_STORE_RAM`, outside `NFC_STORE_RAM_SHELL` | Move include inside `#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)`. |
| `src/nfc/applets/nfc_applet_scan.c` | `nfc_applet_scan_print(struct shell *)` is pure rendering compiled under `NFC_APPLETS` | Add headless `nfc_applet_scan_get_result(...)`; move print body verbatim into `nfc_applet_shell_cmds.c`. |
| `src/nfc/applets/nfc_applet_loop.c` | `nfc_applet_loop(struct shell *, ...)` interleaves orchestration + `shell_print` | Add headless `nfc_applet_loop_run(slot, timeout, log_cb, ctx, &result)`; reduce `cmd_nfc_loop` to a thin adapter. |
| `src/nfc/applets/nfc_applet.h` (header) | declares shell-typed prototypes unconditionally | Drop shell prototypes from `nfc_applet.h`; new `nfc_applet_service.h` holds headless API. |

## C.2 New Layer-1 API (`src/nfc/applets/nfc_applet_service.h`)

Headless, no `struct shell`. Result structs + errno + optional progress callback:

```c
typedef void (*nfc_applet_log_fn)(void *ctx, const char *stage, int code); /* NULL = silent */

int nfc_applet_scan_get_result(nfc_applet_scan_result_t *out);        /* replaces scan_print */
int nfc_applet_get_card_meta(const char *slot, nfc_applet_card_meta_t *out);
int nfc_applet_loop_run(const char *slot, k_timeout_t,
                        nfc_applet_log_fn log, void *ctx,
                        nfc_applet_loop_result_t *out);                /* replaces loop(shell,…) */
```

`read`, `emulate`, `verify`, `scan_start/wait` are **already** headless — keep them. (Full struct definitions: [`NFC_APPLET_API_LAYERING.md`](NFC_APPLET_API_LAYERING.md) §3.)

## C.3 Kconfig / CMake enforcement

| Kconfig | Compiles | Enforcement |
|---------|----------|-------------|
| `NFC_APPLETS` | headless service: `nfc_applet_{scan,read,emulate,verify,verify_compare,policy,loop,service}.c` | CMake adds these under `if(CONFIG_NFC_APPLETS)`; **no shell symbol** pulled |
| `NFC_APPLETS_SHELL` (`depends on SHELL`) | adapter only: `nfc_applet_shell_cmds.c` | CMake gates under `if(CONFIG_NFC_APPLETS_SHELL)` |
| `NFC_STORE` | `nfc_store.c` — **no shell include after fix** | grep gate: no `shell.h` under non-`*_SHELL` |
| `NFC_STORE_RAM_SHELL` | `nfc_store_shell_cmds.c` + shell block in `nfc_store_ram.c` | include moved inside gate |

**Invariant (CI-checkable):** a reader-profile build with `CONFIG_SHELL=n` compiles and links with **zero** shell references below L2.

## C.4 Deconvolution sequencing (mapped to phases)

| Step | Lands in | Risk | Status |
|------|----------|------|--------|
| `nfc_applet_service.h` skeleton (structs/signatures) | ~~Phase 5~~ → **Phase A** | trivial | **LANDED** |
| scan+read: `scan_get_result` + `get_card_meta`, move print to adapter, clean `nfc_applet.h` | ~~Phase 5~~ → **Phase A** | low | **LANDED** (scan also became continuous discovery start/stop) |
| **store default-save fix** + `nfc_store_ram.c` include move | ~~Phase 5~~ → **Phase A** | medium | **LANDED** |
| loop: `nfc_applet_loop_run` + thin `cmd_nfc_loop` | ~~Phase 7~~ → **Phase A** | medium | **LANDED** |
| headless Layer-1 tests + `SHELL=n` CI build | Phase 8 / P5 | low | **deferred** (Phase A proved the `SHELL=n` build manually; the Ztest tier + CI scenario remain for the test-harmonization phase) |

---

# Part D — Bottom-up harmonization phases (0–9)

Each phase = one agent work package. Inputs read-first. Outputs = `CONTEXT.md` + appended findings + **(where Part C applies) scoped code commits**. Verify = the Part B.6 scenario-scoped twister run before the phase closes.

| Phase | Layer dir(s) | Audit + CONTEXT.md | Code changes (Part C) | Verify (Part B.6) | Model |
|-------|--------------|---------------------|------------------------|-------------------|-------|
| **0** | this plan | this file + arch stub | — | user sign-off (Part G) | (done) |
| **A** | `src/nfc/applets`, `src/nfc/store`, `src/nfc/reader` (shell) | Phase-A banner (above) | **Part C.4 steps 1–4 ALL LANDED** — service.h, scan→continuous discovery, store default-save inert, RAM include gate, loop_run + thin adapter, drop `nfc reader clone`, `verify`→`check` | `nfc_reader` twister 2/2·97/97; reader/reader+listen/`SHELL=n` builds | **Opus** (done) |
| **1** | `modules/nfc_pn7160`, `src/nfc/hal` | 2 `CONTEXT.md` — **sets the gold standard** | — | module twister | **Opus** |
| **2** | `src/nfc/framing`, `src/nfc/router` | 2 `CONTEXT.md` | — | `nfc_apdu_asm` twister | Opus |
| **3** | `src/nfc/reader` | 1 `CONTEXT.md` | — | `nfc_reader` twister | Composer |
| **4** | `src/nfc/nfc_stack`, `src/nfc/run` | 2 `CONTEXT.md` | — | listen + nfct overlay build | **Opus** |
| **5** | `src/nfc/store` **+ applet scan/read** | 1 store `CONTEXT.md` + applet service header | ~~Part C.4 steps 1–3~~ **LANDED in Phase A** — Phase 5 now only *verifies* the headless store/scan baseline and adds the headless Tier-E tests + `SHELL=n` CI scenario (Part E) | `nfc_reader` store+store_ram **+** reader `SHELL=n` compile | **Opus** (boundary-sensitive) |
| **6** | `src/nfc/protocols/*` (9) **+ test gating fix** | 9 `CONTEXT.md` | **Part B.5**: wrap protocol blocks in `nfc_reader/CMakeLists.txt` under `if(CONFIG_NFC_PROTOCOL_X)`; split `nfc_reader/prj.conf` into profile-scoped fragments | per-protocol twister, all tiers | Opus orch + **Composer** per-protocol |
| **7** | `src/nfc/applets` **+ shell adapters** | 5 applet `CONTEXT.md` | ~~Part C.4 step 4~~ **LANDED in Phase A** (`loop_run`, thin `cmd_nfc_loop`, all `*_shell_cmds.c` are the only L2). Phase 7 now only documents/verifies the adapter layer | reader + CE builds, `SHELL=y`/`n` | **Opus** |
| **8** | `tests/common`, `tests/unit/*` | `tests/common/CONTEXT.md` + fidelity findings | **Part E**: profile-scoped scenarios, headless applet Tier-E, `SHELL=n` CI build, anti-pattern remediation | **full** `tests/unit` twister + headless tier | **Opus** |
| **9** | assemble | `NFC_STACK_ARCHITECTURE.md` + `NFC_HARMONIZATION_FINDINGS.md` | — | cross-link lint; diagrams render | Opus |

Effort: S < ½ day, M ≈ 1 day, L ≈ 2 days agent time. Phases 1 L, 5/6/7/8 L, others M.

## D.1 Per-directory `CONTEXT.md` template

Filename `CONTEXT.md` (existing `tests/**/README.md` left untouched). Max ~40–60 lines, no filler.

```markdown
# <dir> — CONTEXT
**Layer:** <L0/L1/L2 + stack position> · **Lifecycle:** <full|minimal, CONVENTIONS §2>
## Purpose
<2 sentences: what it owns / does NOT own>
## Key files
| File | Owns |
## Kconfig
| Symbol | Default | Effect |
## Deps
- Upstream (calls down): … · Downstream (callback up): … (CONVENTIONS §3 boundary)
## Invariants & safety
- Buffers / Lifetimes / Concurrency (-EBUSY while STARTED) / silent-drop counters
## Tests that prove it
| Test | Tier | Profile gate (CONFIG_*) | Proves |
## Shell
<pointer to *_shell_cmds.c + *_SHELL gate; no logic duplicated>
```

## D.2 Per-protocol addendum

```markdown
## Roles
- Poller: <file> (Tier B) · Listener: <file | "skip native — ndef T4 adapter">
## Wire/spec  <ref; emulatable? clone-only?>
## Capacity symbols  | CONFIG_NFC_<P>_* | default | bound it protects |
## Fixtures ↔ goldens  <fixtures dir> ↔ <store golden>
## Profile membership  reader: yes/no · CE emulate subset: yes/no
```

## D.3 Per-applet addendum

```markdown
# nfc_applet_<x> — CONTEXT
**Entry (L1):** nfc_applet_<x>(…) · **Adapter (L2):** nfc_applet_shell_cmds.c (nfc <x>)
## Flow  <scan→read→store→verify position>
## Deps  reader/store/stack APIs; protocols required at runtime
## Safety  headless (no shell below L2); -EBUSY during session; buffer reuse
## Tests  Tier E in tests/unit/nfc_reader (headless, log==NULL)
```

## D.4 Audit checklist (per layer → findings)

| # | Category | Check | Authority |
|---|----------|-------|-----------|
| A | Buffer bounds | FIXED pools; bounds-checked; oversize → `6700` + counter, never `__ASSERT` | CONVENTIONS §5 |
| B | Error propagation | errno returned or named stat bumped; no swallowed return; no silent break/continue | CONVENTIONS §6/§9 |
| C | Session/state | poll vs listen separated; `-EBUSY` while STARTED; field-off resets; profile switch deferred | CONVENTIONS §7/§8 |
| D | Kconfig↔CMake | every `*.c` gated to its symbol; no source without gate; capacity ints have defaults | KCONFIG_ARCH |
| E | Shell leakage | no `shell.h`/`shell_*` outside `*_shell_cmds.c` gated by `*_SHELL` | Part C |
| F | Test fidelity | mock matches real; negative paths asserted; no `==0`-of-no-op; **tier runs production path the profile compiles** | Part B |
| G | Fixture↔golden | fixtures decode to same bytes as store golden; goldens regenerable, not circular | Part E |

Finding format: `severity (drop/leak/bound/silent/divergence) · file:line · category · one-line fix`.

---

# Part E — Test harmonization program

Goal: make the test suite obey **Test truth** (A.2.3). The audit's job is to align every suite's tiers with the Kconfig the profile compiles, then add the missing headless coverage that the Part C deconvolution unlocks.

## E.1 Suite inventory vs Kconfig coverage

| Suite | Tiers today | Gate today | Gap → fix |
|-------|-------------|------------|-----------|
| `nfc_ndef` | model/poller/listener | `prj_*.conf` + `*_TIER_*` | OK — template |
| `nfc_ultralight,classic,felica,slix` | model/poller | `*_TIER_POLLER` | clone-only: Tier C via ndef T4 adapter only — record in CONTEXT |
| `nfc_iso15693_3` | (via reader) | — | confirm poller tier present |
| `nfc_desfire,emv,aliro` | model/poller/listener | `*_TIER_*` | emulatable: confirm listener tier in CE scenario |
| `nfc_reader` (Tier E) | store, store_ram | overlay | **compiles all protocols unconditionally** → gate per `CONFIG_NFC_PROTOCOL_X` (B.5); add headless applet tier |
| `nfc_apdu_asm` | framing | `prj.conf` | OK |
| `pn7160_tml` (module) | TML | module Kconfig | OK |

## E.2 Scenario design tied to profiles

Every `testcase.yaml` scenario names its profile + tier allowlist:

| Scenario | CONF/OVERLAY | Tiers built | Platform |
|----------|--------------|-------------|----------|
| `nfc_reader.store` | base `prj.conf` | store roundtrip (reader profile capacities) | qemu_cortex_m3 |
| `nfc_reader.store_ram` | `overlay-store-ram.conf` | store_ram slot/clone/verify | qemu + native_sim |
| `nfc_reader.applet_headless` *(new)* | base + `NFC_APPLETS=y` | `scan_get_result`/`get_card_meta`/`loop_run` with `log==NULL` | qemu + native_sim |
| `nfc_reader.shell_off` *(new)* | reader profile + `CONFIG_SHELL=n` | compile-only (locks Part C gate) | qemu_cortex_m3 |
| `nfc_<proto>.poller` | `prj_poller.conf` + `*_TIER_POLLER` | Tier B | qemu + native_sim |
| `nfc_<proto>.listener` (emulatable) | `prj_listener.conf` + `*_TIER_LISTENER` | Tier C | qemu + native_sim |

## E.3 Tier minimums (preconditioned on Kconfig)

| Protocol class | Tier A | Tier B | Tier C | Precondition |
|----------------|--------|--------|--------|--------------|
| Full emulatable (ndef, desfire, emv, aliro) | ≥12 | ≥12 | ≥12 | `*_TIER_LISTENER=y` only in CE scenario |
| Clone-only (ultralight, classic, felica, slix, iso15693_3) | ≥8 | ≥8 | ndef T4 adapter only | `*_TIER_POLLER=y`; **no** native listener |
| Tier E (`nfc_reader`) | store + store_ram both run | — | — | RAM via `overlay-store-ram.conf` |

## E.4 Virtual loopback eligibility

A protocol qualifies for `tests/common/nfc_virtual_loopback` **iff** poller+listener both exist natively (ndef, desfire, emv, aliro) **and** the scenario is a CE-capable profile. Clone-only protocols use `nfc_session_mock` `*.inc` scripts — recorded per-protocol `CONTEXT.md`. A reader-only scenario must not schedule loopback for emulate-only protocols.

## E.5 Headless applet tests (post-deconvolution, Phase 8)

Unlocked once Part C lands. Each asserts the **result struct**, with `log==NULL` (no shell mock):
- `scan_get_result` → populated `nfc_applet_scan_result_t` from a mock session.
- `get_card_meta` → correct `persist_id`/`flags`/`protocol_name` for a stored slot.
- `loop_run` → `failed_stage`/`verify_result` across read→emulate→verify (silent).
- store save via RAM backend with **no** shell `user_ctx` (proves C.1 store fix).
- reader profile builds with `CONFIG_SHELL=n` (proves no L0/L1 shell leakage).

## E.6 Retest discipline

No phase closes on a stale green. Re-run the Part B.6 scenario for the phase, paste PASS/FAIL counts into the findings file. Phase 8 runs the **full** `tests/unit` twister and is the gate for the architecture assembly.

---

# Part F — Findings & architecture assembly

## F.1 `NFC_HARMONIZATION_FINDINGS.md` (rollup, Phase 9)

One file aggregating per-phase findings, grouped by category (A–G of D.4), each: `severity · file:line · category · fix`. Cross-references the `CONTEXT.md` of the owning dir. Distinguishes **landed** (Part C code fixes) from **recorded-only** (deferred remediation).

## F.2 `NFC_STACK_ARCHITECTURE.md` (final assembly, Phase 9)

1. **Block diagram (ASCII)** — L0→L3 with HAL → framing → router → {protocols} ↔ store, reader sidecar, nfc_stack orchestrator.
2. **Data flow (mermaid)** — inbound ISR→pool→fifo→WQ→framing→router→service→response; reader scan→poller→store; field-off/profile-switch edges.
3. **HAL profiles** — PN7160 vs NRFX capability + role ceilings.
4. **Protocol registry** — table from the 9 `protocols/*/CONTEXT.md`.
5. **Store envelope** — blob layout, CRC, serialize/deserialize vtable, golden set.
6. **Applet service layer** — L1 headless API + L2 shell adapter split (Part C).
7. **Test pyramid** — Tier A/B/C/E counts **with their Kconfig gates** + Tier D HIL pointer.
8. **Overlay matrix** — profiles × overlays (KCONFIG_ARCH Appendix B).

---

# Part G — Approval checklist (sign off before Phase 1)

- [ ] **One plan of record** — this file supersedes the standalone applet plan + shell audit (they remain reference). Confirmed.
- [ ] **Applet/shell deconvolution is IN scope and lands as code** at Phases 5 & 7 (not a deferred orphan PR).
- [ ] **Test gating policy** (Part B) accepted: every twister scenario = profile + overlay + explicit tier allowlist; tiers Kconfig-gated; `nfc_reader` per-protocol CMake gating fixed (B.5).
- [ ] **Read-only vs fix-as-you-go split** accepted: layers 1–4/6 audit = read-only findings; **store default-save (C.1), applet extraction (C.2), and test-gating (B.5/E)** are fix-as-you-go code commits.
- [ ] **Doc name = `CONTEXT.md`** for all layer/protocol/applet docs; existing test READMEs untouched.
- [ ] **Bottom-up order + 0–9 phases** (Part D) accepted, incl. model assignments.
- [ ] **Tier minimums + fidelity rules** (Part E) are the test-phase acceptance bar.
- [ ] **Findings** live in per-phase files + rolled `NFC_HARMONIZATION_FINDINGS.md`.
- [ ] **Out of scope** confirmed: no GPL port; Kconfig Phase 2 renames / Phase 3 `rsource` deferred (test-gating fixes in Part E are separate and IN scope).
- [ ] **HIL stays parallel** (Part H) — this program does not gate hardware validation.

---

# Part H — Sequencing vs HIL

| Track | When | Blocks HIL? | Interaction |
|-------|------|-------------|-------------|
| **Phase A (applet foundation)** | **LANDED — before P1** | No | **Blocks nothing**; harmonization P1+ runs after Phase A green. Pulled the Part C deconvolution ahead of the audit so all audit phases start headless+gated. Externally observable applet output is preserved except the LOCKED command renames (`scan`→`scan start/stop`, `verify`→`check`, `reader clone` dropped). |
| Phases 1–4 (audit, read-only) | anytime, parallel (after Phase A) | No | findings may inform HIL |
| **Phase 5 store default-save fix (C.1)** | between HIL cycles | **Caution** | touches reader-only `nfc read` path; land when no HIL `read` sign-off in flight |
| Phase 5 scan/read extract | parallel | No | externally observable output identical |
| Phase 6 test-gating (B.5) | parallel | No | test-only |
| **Phase 7 loop deconvolution** | between HIL cycles | **Caution** | restructures `loop` orchestration — **not** during a HIL `loop` sign-off |
| Phase 8 test harmonization | parallel | No | test-only |
| HIL Gates 2–5 | parallel, anytime | (independent) | uses current overlays; not gated by this program |
| Kconfig Phase 2 (overlay renames) / Phase 3 (`rsource`) | deferred | No | not in this program |

**Parallel-safe:** all read-only audit phases, scan/read extract, all test-gating work.
**Serialize against HIL:** store default-save (C.1) and loop deconvolution (C.4 step 4) — land between HIL cycles.
