# NFC HIL & Polish Plan (post–Step A, non-Kconfig)

**Branch:** `nfc-stack` · **Scope:** everything in the remaining NFC backlog **except** the Kconfig
refactor (owned by a parallel agent — see [`NFC_KCONFIG_ARCHITECTURE.md`](NFC_KCONFIG_ARCHITECTURE.md)).
**Companion:** [`2026-06-14-nfc-sequential-execution.md`](2026-06-14-nfc-sequential-execution.md) (Gates 2–5 source of truth).

> This is a **planning** doc. Do not implement code from here except trivial doc typos that block reading.
> HIL steps are runbooks for a human at the bench; software-polish and doc-polish are agent-executable.

---

## 1. Executive summary & verdict

**Verdict: software is effectively ship-ready on QEMU; the product cannot ship until HIL Gates 2–5 pass on real silicon.**

Evidence gathered from the tree (not assumptions):

- **Step A is closed.** A1–A4 all landed (`688c370`, `cfd30b0`, `2bd1e77`→`646c5ab`, `2c9fab1`).
- **PN7160 listen `on_apdu` path is implemented** (`cfd30b0`) — `src/nfc/hal/nfc_transport_pn7160.c`
  now allocates from `nfc_apdu_pool`, assembles fragments, and calls `s_ops.on_apdu`
  (lines ~109–186). This contradicts `NFC_HAL_GUIDE.md` §6 which still says "drops only" (doc drift, §3).
- **Unit coverage is large and green-by-design**: `tests/unit/nfc_reader/` alone has **52** `ZTEST(`
  cases across 11 files; the protocol suites (`nfc_ndef`, `nfc_ultralight`, `nfc_classic`, `nfc_felica`,
  `nfc_slix`, `nfc_desfire`, `nfc_emv`, `nfc_aliro`) all exist with model + poller (+ listener) tests.
- **CI already runs 10 unit dirs** in `.github/workflows/twister.yaml` (`twister-unit` job),
  contradicting `CI_TESTING.md` which still calls the NFC stack suites "Local only" (doc drift, §3).
- **No silicon evidence exists for Gates 2–5.** Every "PASS" in the sequential plan for Gates 2–5
  is QEMU/build-only or marked HIL-pending. That is the ship blocker.

**One-line:** the code and the test harness are ahead of the docs; the docs claim *less* than what
exists, and the hardware sign-off is the only real gap.

---

## 2. HIL runbook — Gates 2–5

### Shared hardware prerequisites

| Item | Detail |
|------|--------|
| Reader board | nRF54L15DK + **PN7160** click/shield on I2C (`boards/overlays/pn7160_i2c.overlay`) or SPI (`pn7160_spi.overlay`) |
| Emulator board (Gate 5) | nRF54L15DK NFCT antenna (on-board) for `nfc_t4t_lib` PICC |
| Tags | 1× NTAG/Type-4 NDEF tag (Gate 2/3), 1× known UID tag for diff (Gate 4) |
| Toolchain | NCS v3.2.4; `source "$REPO/scripts/env/ncs-env.sh"`; `cd "$NCS_ROOT"` |
| Env | `export REPO=/Users/majidfaroud/writable_ndef_msg` |

Single-flight rule (from `NFC_HAL_GUIDE.md` §3): never poll and listen on the same controller
concurrently. RW+CE on one PN7160 is **DEFERRED** (see `overlay-pn7160-listen.conf` header).

### Gate 2 — PN7160 reader: read / clone a real tag

**Goal:** prove poll session + NDEF poller walk + `.card` store on hardware.

Flash:
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf"
west flash
```

Shell sequence (over `SHELL_BACKEND_SERIAL`):
```
uart:~$ nfc scan
uart:~$ nfc read tag1          # alias: nfc reader clone tag1
uart:~$ nfc store list
uart:~$ nfc store load tag1
```

**Expected logs:** `nfc scan` prints UID / protocol / interface for the physical tag;
`nfc read tag1` returns a valid `.card` hex blob and a non-zero stored length;
`nfc store list` shows `tag1`.

**Pass criteria:**
- `discover_wait` returns a valid `nfc_transport_tag_info_t` (UID len > 0).
- `nfc read` produces a `.card` whose CC/NDEF round-trips through `nfc store load`.
- No `-EBUSY`/timeout on a present tag; session held open across transceives (A1 behavior).

### Gate 3 — PN7160 card emulation: SELECT + READ BINARY

**Goal:** prove the listen `on_apdu` → `apdu_assembler` → `aid_router` → NDEF listener path on silicon.

Flash (reader-stack base + listen layer):
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf;$REPO/overlay-pn7160-listen.conf"
west flash
```

Shell sequence:
```
uart:~$ nfc store load tag1     # or load an NDEF golden first
uart:~$ nfc emulate tag1
```

Then drive it with an **external reader** (phone NFC Tools / second PN7160):
SELECT NDEF Tag Application AID → SELECT CC → READ BINARY → SELECT NDEF file → READ BINARY.

**Expected logs:** field-on event; `apdu_assembled_count` increments; SELECT returns `90 00`,
READ BINARY returns CC then NDEF payload with valid status words.

**Pass criteria:**
- `nfc_transport_get_stats`: `apdu_assembled_count > 0`, `apdu_dropped_no_consumer == 0`.
- External reader reads back the NDEF content matching the stored `.card`.

### Gate 4 — emulate → verify → loop (sequential, RW+CE deferred)

**Goal:** the full applet loop on a single PN7160, switching sessions (no concurrent RW+CE).

Same image as Gate 3. Sequence:
```
uart:~$ nfc read tag1           # session 1: poll
uart:~$ nfc emulate tag1        # session 2: listen (stack stop between)
uart:~$ nfc verify tag1         # session 3: poll, diff vs stored .card
uart:~$ nfc loop tag1           # orchestrated read→emulate→verify
```

**Expected logs:** `nfc verify` reports PASS (model + UID match); `nfc loop` prints a single
HIL sign-off PASS line.

**Pass criteria:**
- `nfc verify tag1` → **PASS** against the blob from `nfc read`.
- `nfc loop tag1` → **PASS** end-to-end with a `stack stop` between emulate and verify
  (confirm no `-EBUSY` from store while listen STARTED — single-flight enforced).
- RW+CE concurrency is **not** tested (deferred by design).

### Gate 5 — NFCT emulate + PN7160 cross-verify

**Goal:** same `.card` emulated on NFCT (`nfc_t4t_lib` PICC), verified by the PN7160 reader.

Two boards. **Emulator board** (NFCT):
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-nfct-stack.conf"
west flash
# uart:~$ nfc store load tag1 ; nfc emulate tag1
```
**Reader board** (PN7160, Gate 2 image): hold over the emulator antenna →
```
uart:~$ nfc verify tag1
```

**Expected logs (per `overlay-nfct-stack.conf` header checklist):** NFCT field-on, no crash;
PN7160 SELECT app AID → valid SW/payload; `apdu_assembled_count > 0`,
`apdu_dropped_no_consumer == 0`.

**Pass criteria:**
- `nfc verify tag1` on PN7160 → **PASS** against the NFCT-emulated card.
- No concurrent poll+listen on either controller.

---

## 3. Doc drift fix checklist (file · line · change)

All are **stale-status** fixes; each is agent-executable without touching code.

| # | File | Line(s) | Current (wrong) | Change to |
|---|------|---------|-----------------|-----------|
| D1 | `docs/nfc/NFC_APPLETS_AND_TESTING.md` | 67–72 | 12× `pending F1` in the Flipper parity table | Tier A/B ztests **exist** (`nfc_ultralight` has model+poller); set Tier A/B/C to reflect reality — Ultralight model/poller **done**, mark only **Tier C (NDEF-T4 listen adapter)** + Tier E+ as `pending F1` |
| D2 | `tests/fixtures/nfc/flipper/README.md` | 55–60 | 12× `pending F1` (same table) | Same revision as D1; keep in sync |
| D3 | `tests/fixtures/nfc/flipper/README.md` | 66 | Malformed row `nfc_nfca_signal_long.nfc` (2 cols, not 5) + header says "Manifest (13 files)" while body listed 12 | Fix the row to 5 columns (`tests/fixtures/hal/` · `pending (HAL)` · `n/a` · `n/a`); reconcile the 13-vs-12 count (MfClassic golden makes 13) |
| D4 | `tests/unit/nfc_reader/README.md` | 3, 7–42 | "13 ztests" (7+3+3) | **52** `ZTEST(` cases across 11 files; rewrite the per-file table to include `test_store_{classic,felica,slix,desfire,emv,aliro}_roundtrip.c`, `test_store_import.c`, `test_poller_registry.c`; keep `test_reader_clone_slot` in TODO |
| D5 | `docs/nfc/CI_TESTING.md` | 14 | NFC stack suites "**Local only** … not in merge-gate CI yet" | `twister.yaml` `twister-unit` runs **10 dirs** in CI; update the row to "Active in CI" and list the dirs (see §4) |
| D6 | `docs/nfc/NFC_HAL_GUIDE.md` | 296–301 (§6) | "PN7160 listen — incomplete (Gate 3) … drops only" | `on_apdu`/pool path **landed** (`cfd30b0`); narrow the remaining-gap bullet to **`set_uid` live rotation** + synthetic field on/off only |
| D7 | `docs/nfc/NFC_HAL_GUIDE.md` | new (end of §6) | No overlay matrix | Add the overlay matrix table from §4 (which overlay → roles → gate) |

> Note: D1/D2 require judgement — confirm whether each protocol's **listener** (Tier C) is wired
> before downgrading "pending". Model/poller (`test_model.c`, `test_poller.c`) clearly exist; listener
> adapters (`nfc_ultralight` via NDEF-T4) may legitimately remain pending.

---

## 4. CI polish checklist

| # | Item | Current state | Action |
|---|------|---------------|--------|
| C1 | `nfc_apdu_asm` not in merge-gate CI | `twister.yaml` `UNIT_DIRS` (lines 149–160) lists 10 dirs but **omits** `tests/unit/nfc_apdu_asm`; its `testcase.yaml` has tags `ci_unit, nfc` | Add `tests/unit/nfc_apdu_asm` to the `UNIT_DIRS` array |
| C2 | `CI_TESTING.md` ↔ `twister.yaml` truth | Doc says local-only (D5) | After D5, add a "CI unit matrix" subsection listing the 10 (→11 with C1) dirs verbatim |
| C3 | Overlay matrix as single source | Scattered across overlay headers | Publish this table in `NFC_HAL_GUIDE.md` (D7) and reference from `CI_TESTING.md` |

**CI unit matrix (current `twister-unit` job):**
```
modules/nfc_pn7160/tests/unit/pn7160_tml
tests/unit/nfc_reader
tests/unit/nfc_ultralight
tests/unit/nfc_classic
tests/unit/nfc_felica
tests/unit/nfc_slix
tests/unit/nfc_desfire
tests/unit/nfc_emv
tests/unit/nfc_aliro
tests/unit/nfc_ndef
# + tests/unit/nfc_apdu_asm   (C1 — to be added)
```

**Overlay matrix (for D7 / C3):**

| Overlay | Roles | Backend | Gate | Use |
|---------|-------|---------|------|-----|
| `overlay-pn7160-stack.conf` | reader | PN7160 | 2 | `nfc scan/read/verify` poll path |
| `overlay-pn7160-listen.conf` | + listen | PN7160 | 3–4 | layered on stack; CE `nfc emulate` (RW+CE deferred) |
| `overlay-nfct-stack.conf` | listen | NRFX `nfc_t4t_lib` | 5 | NFCT PICC emulate, multi-protocol |
| `overlay-pn7160.conf` / `-hal.conf` / `-spi.conf` | reader | PN7160 | bring-up | driver/HAL smoke + SPI variant |
| `boards/overlays/pn7160_unit_test.overlay` | — | emul | unit | QEMU DTS for `--no-sysbuild` builds |

---

## 5. P1 software polish (specs, no implementation)

| # | Item | Spec |
|---|------|------|
| P1-a | `test_reader_clone_slot` ztest | New case in `tests/unit/nfc_reader/`: mock poller walk → `nfc_store_save("tag1")` → assert stored `.card` envelope matches the poller model and the slot is listable. Uses existing `nfc_session_mock` + store golden. Removes the last TODO in the reader README. |
| P1-b | HAL signal fixtures `nfc_nfca_signal_*` | Wire `tests/fixtures/hal/nfc_nfca_signal_{short,long}` into a HAL framing ztest (Tier "pending (HAL)"). Scope = ATQA/SAK/anticollision framing assertions only; **not** Tier A/B/C protocol. Keep out of `.card.bin` (n/a). |
| P1-c | `tag_info` ATQA/SAK/ATS gap | `nfc_transport_tag_info_t` (`src/nfc/hal/nfc_transport.h:90–96`) carries only uid/protocol/interface/mode_tech. Add `atqa`, `sak`, `ats[]`/`ats_len` (+ ATQB for Type-B) populated from PN7160 `RF_INTF_ACTIVATED_NTF`. Spec the struct extension + backend fill + a poller-detect ztest. Cross-listed as HAL gap in the sequential plan ("Remaining HAL gaps"). |

These are independent of HIL and safe to do in parallel (see §7).

---

## 6. P3 deferred — list only, no action

- RW+CE concurrency on a single PN7160 (NXP combined-discovery sequencing).
- `tech_mask` honoring in PN7160 `discover_start` (currently ignored) — Backlog/FeliCa.
- FeliCa / NFC-F discovery table — Backlog F4.
- `nfc_t2t_lib` native Type-2 listener (read-only mimic) — backlog; T4 adapter covers v1.
- FF store v2 (`.nfc` on disk) — `NFC_APPLETS_AND_TESTING.md` step 4.
- Per-protocol expansion F1–F5 (ultralight listener, classic, desfire/emv/aliro raw lane, felica/iso15693).
- 255 B READ BINARY chunking polish (owned by `protocols/ndef`).

---

## 7. Recommended execution order

**Two tracks run in parallel — software/doc polish does not block on hardware.**

**Track A — bench (serial, human-gated):** Gate 2 → Gate 3 → Gate 4 → Gate 5.
Each gate is a hard prerequisite for the next (you cannot verify CE without a reader, etc.).

**Track B — agent (parallel with Track A, no hardware):**
1. **C1** (add `nfc_apdu_asm` to CI) — smallest, highest leverage; one-line array edit.
2. **D4 + D1/D2/D3** (test-count and parity-table truth) — pure doc, no risk.
3. **D5/D6/D7 + C2/C3** (CI + HAL guide alignment) — depends on D-set wording.
4. **P1-a** (`test_reader_clone_slot`) — closes the reader TODO; QEMU-verifiable.
5. **P1-b / P1-c** — larger; P1-c touches a public struct (coordinate with Kconfig agent only at the
   header level, no Kconfig symbols).

Sequencing rationale: do the zero-risk CI + doc truth-up first (they make the repo self-describing
and unblock reviewers), then the QEMU-verifiable test (P1-a), then the struct-extending work (P1-c)
which benefits from Gate 2/3 bench observations of real `RF_INTF_ACTIVATED_NTF` fields.

---

## 8. Success criteria

**"Software ship-ready" (no hardware required):**
- All CI unit dirs green, including `nfc_apdu_asm` (C1).
- Docs match the tree: no stale "pending F1" where tests exist (D1/D2), reader count = actual
  (D4), CI doc = `twister.yaml` (D5), HAL guide listen status = code (D6), overlay matrix published (D7).
- `test_reader_clone_slot` implemented (P1-a) → reader README TODO empty.
- No new public-API doc-coverage failures (`doxygen-coverage-delta.yml`).

**"Product ship-ready" (adds hardware):**
- Gates 2–5 all PASS on silicon per §2 pass criteria, with captured logs/stats
  (`apdu_assembled_count > 0`, `apdu_dropped_no_consumer == 0`, `nfc verify` PASS, `nfc loop` PASS).
- Cross-backend Gate 5 PASS (NFCT emulate ↔ PN7160 verify).
- HIL sign-off recorded against the Gate 4/5 rows in the sequential execution plan.

---

## 9. Out of scope (this plan)

- **Kconfig refactor** — owned by parallel agent (`NFC_KCONFIG_ARCHITECTURE.md`).
- Gate 2–5 **code** changes — owned by `2026-06-14-nfc-sequential-execution.md`.
- New protocol modules (F1–F5) — backlog.
