# NFC Implementation Agent Protocol

**Status:** BINDING for every implementation agent dispatched against a wave plan.
**Scope:** `src/nfc/` in `sigmation_experimental`, branch `nfc-stack` (waves merge to `dev`).

Every dispatch prompt references this file. If anything here conflicts with your
dispatch prompt, this file wins unless the prompt explicitly overrides a numbered
rule by ID.

---

## 1. Identity and scope

- **R1.** You implement exactly ONE wave plan (or one named subset of its tasks).
  The plan is your complete specification. Anything not in the plan is out of
  scope — including improvements, refactors, helpers, and "obvious" additions.
- **R2.** You may read: your wave plan, `NFC_STACK_CONVENTIONS.md`, the five
  upstream design docs it cites, spec v3.1
  (`docs/superpowers/specs/2026-06-12-nfc-stack-architecture.md`), and the
  **headers** of the layer below yours. You may NOT read or copy from
  `flipperzero/` (GPL reference tree — protocol facts are already distilled
  into the plans).
- **R3.** You may modify only the files your plan's "Files produced" section
  names, plus the root `Kconfig`/`CMakeLists.txt` lines your plan specifies.
  No drive-by edits: never touch other modules, never reformat neighbors,
  never "fix" the substrate (`src/system/`).

## 2. The task loop (TDD, one commit per task)

For each numbered task in the plan, in order:

1. **Test first.** Write the ztest cases the task names, in the module's
   `tests/unit/<module>/` suite. Run them; they must FAIL (red) for the right
   reason before you implement.
2. **Implement** the minimum the task specifies until tests pass (green).
3. **Gate.** Run the gate commands (§3). All green or no commit.
4. **Commit.** One task = one commit. Never batch tasks; never commit red;
   never `--amend` a pushed commit.

- **R4. Commit message format:** `feat(nfc): <wave> task <N> — <short summary>`
  (e.g. `feat(nfc): wave1 task 7 — send_response ISR path`). Use `test(nfc):`
  for test-only commits, `fix(nfc):` for in-wave fixes after review findings.
- **R5.** Mark each task done in your running report as you commit it.
- **R5a (early checkpoint).** The dispatcher reviews your commits continuously,
  starting after your first ~3 tasks, and may interrupt with corrections. A
  correction to a pattern (not just a line) must be back-applied to all prior
  commits of this wave before new tasks continue — replicated mistakes are
  fixed at the root, early, while they are 3 commits deep instead of 20.

## 3. The gate (must pass before every commit)

```bash
# Unit tests for your module on the DK (HIL rig; SERIAL provided in dispatch):
SERIAL=<port> BOARD_SN=<sn> ./tests/scripts/hil_dk.sh --module <your_module>

# If no board is available in your session, the minimum gate is:
./tests/scripts/hil_dk.sh --build-only --module <your_module>

# Firmware build must stay green (stack enabled in the nfc lab):
west build -d build/nfc_demo -b nrf54l15dk/nrf54l15/cpuapp src/labs/nfc_demo \
  --no-sysbuild -- -DBOARD_ROOT=$PWD
```

- **R6.** A red gate is information, not an obstacle: fix your code, or if the
  plan itself is wrong, STOP (§5). Never weaken a test, never broaden a
  tolerance, never `#ifdef` around a failure to get green.

## 4. Code rules (summary — conventions doc is authoritative)

- **R7.** MISRA C:2012 per the plan's deviation notes; Zephyr idioms per
  `NFC_STACK_CONVENTIONS.md` (lifecycle FSM + state guards, `_fn`/`_cb`/
  `user_ctx` callback pattern, `s_stats`/`s_stats_lock` + `STATS_*`, fixed
  `net_buf` pools, no dynamic allocation, no static locals).
- **R8.** Signatures must match the plan **character for character**. The plans
  are capstone-gated; a mismatch is your bug until proven otherwise.
- **R9.** Downward = direct call; upward = registered callback. Never include a
  higher layer's header. All cross-layer wiring lives in `nfc_stack.c` (Wave 4).
- **R10.** Comments explain non-obvious intent only. No narration, no
  changelog comments, no TODO unless the plan says to leave one.

## 5. Stop conditions (report instead of inventing)

STOP work on the current task and record a discrepancy report when:

- the plan is ambiguous or contradicts the conventions/spec;
- a declared signature/constant doesn't exist where the plan says it does;
- a test cannot pass as specified (toolchain/silicon reality differs from plan);
- you need an API, file, or Kconfig symbol the plan doesn't define.

Format: location (plan §), what you expected, what you found, smallest proposed
resolution. Then continue with the next independent task if one exists.
**The fix goes into the plan first, then the code** — never silently diverge.

## 6. Final report (end of dispatch)

1. Task checklist: done / skipped (+ why) per plan task number.
2. Gate evidence: last `hil_dk.sh` summary line(s) and firmware build result.
3. Discrepancy reports (or "none").
4. Exact `git log --oneline` of your commits.
5. Anything the NEXT wave needs to know (one short list; no essays).

## 7. Environment facts

| Item | Value |
|---|---|
| Repo / branch | `/Users/majidfaroud/sigmation_experimental`, branch `nfc-stack` |
| Board (bring-up) | `nrf54l15dk/nrf54l15/cpuapp` (PCA10156) |
| Product board | `sigmationbaord/nrf54l15/cpuapp` via `-DBOARD_ROOT=$PWD` (no NFC antenna DTS yet — do not target) |
| SDK | NCS v3.2.4 (`/opt/nordic/ncs/v3.2.4`), env per `tests/scripts/hil_dk.sh` |
| Test layout | `tests/unit/<module>/` (own `CMakeLists.txt` + `prj.conf` + `testcase.yaml`), `--no-sysbuild`, Twister tag `sigmation.nfc.<module>` |
| native_sim | Linux CI only (blocked on macOS) — use the HIL rig or `qemu_cortex_m33` |
| Kconfig gate | Everything dark unless `CONFIG_NFC_STACK=y` (only the `nfc_demo` lab sets it) |

---

*Changelog: v1 (2026-06-12) — initial protocol, derived from the wave-planning
campaign and capstone gate.*
