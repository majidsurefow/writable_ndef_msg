# Unified Stats API Design

**Date:** 2026-06-01

## Problem

Stats are scattered across ~22 modules with no consistent pattern. Counter types vary (`uint32_t` vs `uint64_t`), thread safety is handled differently in each module (atomics in some, mutexes in others, nothing in some), getters return live pointers or shared view buffers (torn reads on 32-bit), hot paths take multiple lock trips per event, reset timing is inconsistent, and lifecycle fields (`init_count`, `start_count`, etc.) add noise without value.

## Decision

A single **composite recipe** every stats-bearing module follows — validated by 22-module audits, Zephyr kernel patterns (`k_mem_slab_runtime_stats_get`, `usage.c` settle-then-copy), and in-tree reference implementations. Exemptions are limited to modules listed in **Exempt Modules** below.

---

## The Recipe

### 1. The Stats Struct (in `module.h`)

```c
typedef struct {
    uint32_t error_count;       /* mandatory: total errors */
    int32_t  last_error_code;   /* mandatory: most recent errno */

    /* module-specific fields below */
    uint32_t packets_received;
    uint32_t buffers_dropped_no_buffer;
    latency_stats_t alloc_to_enqueue;
    /* ... */
} module_stats_t;

/**
 * @brief Copy module statistics into caller-owned storage.
 *
 * @param out  Destination buffer; must not be NULL.
 *
 * @retval  0       Success.
 * @retval -EINVAL  @p out is NULL.
 *
 * @note Never returns a pointer into module-private `s_stats`.
 * @note One lock covers the full struct copy — safe on 32-bit (no torn reads).
 */
int module_get_stats(module_stats_t *out);
```

Rules:
- `error_count` and `last_error_code` are in every struct, no exceptions
- All other fields are module-specific — see **Stats Naming Conventions**
- Fields are `uint32_t` by default; use `uint64_t` only when overflow risk is documented
- No lifecycle fields: no `init_count`, `start_count`, `stop_count`, `shutdown_count`, `is_running`
- No side counters outside the struct — every observable counter lives in `{module}_stats_t`

### 2. The Macro API (from `stats.h`)

```c
#include <zephyr/spinlock.h>

/* Increment a field by 1 */
#define STATS_INC(lock, stats, field)               \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).field++;                            \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/* Increment a field by N */
#define STATS_ADD(lock, stats, field, val)          \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).field += (val);                     \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/* Decrement a field by 1 (gauges, e.g. inflight_now) */
#define STATS_DEC(lock, stats, field)               \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).field--;                            \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/* Record an error: increments error_count and saves the code */
#define STATS_ERROR(lock, stats, code)              \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).error_count++;                      \
        (stats).last_error_code = (int32_t)(code);  \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/* Batch multiple field updates under one lock — hot paths only */
#define STATS_SCOPE(lock, body)                         \
    do {                                                \
        k_spinlock_key_t _key = k_spin_lock(lock);      \
        body;                                           \
        k_spin_unlock(lock, _key);                      \
    } while (0)

/* Reset all stats — call once in module_init() only (no lock) */
#define STATS_RESET(stats) memset(&(stats), 0, sizeof(stats))

/* Copy-out getter helper — see §3 */
#define STATS_COPY_OUT(lock, stats, out)  /* defined in stats.h */
```

When to use which macro:

| Macro | Use when |
|---|---|
| `STATS_INC` / `STATS_ADD` / `STATS_DEC` | Single field, cold path (lifecycle error, one-off drop, gauge adjust) |
| `STATS_ERROR` | Record errno + bump `error_count` (two fields, one lock — built-in batch) |
| `STATS_SCOPE` | Hot path: one logical event updates multiple fields (counters + `latency_stats_record`) |
| `STATS_RESET` | Once in `module_init()` only — no lock |
| `STATS_COPY_OUT` | Inside `module_get_stats()` — lock + struct assign into caller buffer |

Hot-path pattern: compute inputs (e.g. elapsed time) **before** `STATS_SCOPE`; never call timing helpers inside the lock. Every hot path must route through a module-private helper — reference: `scan_stats_on_callback()` in `ble_scanner.c`.

### 3. Module Implementation (in `module.c`)

```c
static module_stats_t       s_stats;          /* always this name, always static */
static struct k_spinlock    s_stats_lock;     /* NOT K_SPINLOCK_DEFINE — unavailable in NCS v3.2.4 */

/* Reset — exactly once, in module_init(), before any concurrent writer exists */
int module_init(void) {
    STATS_RESET(s_stats);
    /* ... */
}

/* Hot path — mandatory private helper, one STATS_SCOPE per logical event */
static void module_stats_on_enqueue(uint32_t t0, bool success)
{
    uint32_t elapsed_ms = k_cyc_to_ms_floor32(k_cycle_get_32() - t0);

    STATS_SCOPE(&s_stats_lock, {
        if (success) {
            s_stats.buffers_enqueued++;
            latency_stats_record(&s_stats.alloc_to_enqueue, elapsed_ms);
        } else {
            s_stats.enqueue_failed++;
        }
    });
}

/* Getter — copy-out under lock; caller owns buffer */
int module_get_stats(module_stats_t *out)
{
    return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}
```

**Settle-then-copy** (modules with derived or in-flight fields): finish derivation inside the getter before releasing the lock. Reference: `pending_get_stats()` materializes from `inflight_get_stats()` into caller `out` — no static `s_view` buffer returned to callers.

**Do not:**
- Return `const module_stats_t *` pointing at `s_stats`, `s_stats_view`, or `s_stats_snapshot`
- Use `K_SPINLOCK_DEFINE(s_stats_lock)` — use `static struct k_spinlock s_stats_lock`
- Skip the private `module_stats_on_<event>()` helper on hot paths

### 4. Reset Policy

See **Reset Policy (expanded)** below for Zephyr comparison, lifecycle registry, lock rules, and pingpong exemption.

---

## Stats Naming Conventions

### Struct and storage

| Artifact | Pattern | Example |
|---|---|---|
| Stats struct | `{module}_stats_t` | `ble_scanner_stats_t`, `pending_stats_t` |
| Static storage | `s_stats` | always |
| Spinlock | `s_stats_lock` | `static struct k_spinlock s_stats_lock` |
| Getter | `{module}_get_stats()` | `int ble_scanner_get_stats(ble_scanner_stats_t *out)` |
| Hot-path helper | `{module}_stats_on_{event}()` | `static void scan_stats_on_callback(...)` — private |
| Runtime reset | `{module}_reset_stats()` | only if registered in lifecycle registry |

### Field naming rules

- **snake_case**, descriptive; no abbreviations unless domain-standard (`rx`, `tx`, `ack`, `tlv`, `cb`, `hw`)
- **Counters** — suffix with semantic verb: `_attempted`, `_succeeded`, `_failed`, `_dropped`, `_rejected`. Avoid bare `_ok` / `_fail` unless already established in the module
- **Drop counters** — `buffers_dropped_{reason}` pattern (see `reception_stats_t`, `ble_scanner_stats_t`)
- **Latency fields** — `{start}_to_{end}` in ms unless `_us` suffix documents microseconds (e.g. `alloc_to_enqueue`, `forward_from_alloc`, `callback_time_us`)
- **Embedded latency** — always `latency_stats_t` with a descriptive field name; never generic `latency` or `timing`
- **Mandatory error fields** — `error_count`, `last_error_code` in C; never `errors`, `err`, `last_err` in struct fields (those are TELEM JSON keys only)
- **Gauges vs counters** — `inflight_now` (gauge, current level) vs `registered` (counter, monotonic total). TELEM phase deltas: counters subtract; gauges and embedded `latency_stats_t` emit end-state
- **Types** — `uint32_t` default; `uint64_t` only when overflow risk is documented (e.g. `ble_scanner` packet counters on long-running devices)
- **Forbidden** — lifecycle fields (`init_count`, `is_running`, …); side counters outside the struct

### TELEM JSON key mapping

C field names are canonical. TELEM uses shortened JSON keys. Mapping is stable once published.

| C field | TELEM JSON key | Notes |
|---|---|---|
| `error_count` | `err` | all wired modules must emit |
| `last_error_code` | `last_err` | all wired modules must emit |
| `buffers_alloced` | `alloc` | `tx_channel` |
| `buffers_enqueued` | `enqueue` | |
| `alloc_failed_pool_empty` | `alloc_fail` | |
| `enqueue_failed` | `enq_fail` | |
| `advance_drops` | `adv_drops` | |
| `alloc_to_enqueue` | `alloc_to_enq` | nested `{min,max,avg,n}` |
| `callback_time_us` | `cb_us` | nested latency object |
| `buffers_drained` | `drained` | `reception` |
| `buffers_routed_primary` | `routed_primary` | TELEM; canvas uses legacy key — see below |
| `buffers_routed_fallback` | `routed_fallback` | |
| `work_handler_runs` | `wh_runs` | |
| `buffers_dropped_submit_failed` | `drop_submit` | |
| `buffers_dropped_persist_failed` | `drop_persist` | |
| `registered` | `reg` | `pending` |
| `delivered` | `deliv` | |
| `timed_out` | `to` | |
| `inflight_now` | `inflight` | gauge — end-state in phase delta |
| `inflight_high_water` | `inflight_hw` | gauge — end-state in phase delta |
| `register_to_delivered` | `lat_deliv` | nested latency |
| `register_to_timed_out` | `lat_to` | nested latency |
| `session_writes` | `session` | inside `logger` object only — not a top-level subsystem |

**Known fix — reception/canvas key mismatch:** TELEM emits `routed_primary` / `routed_fallback` from C fields `buffers_routed_primary` / `buffers_routed_fallback`. Host canvas (`canvas_render.py`) still reads legacy keys `tlv_sub` / `sr_routed`. Align canvas to TELEM keys (or add TELEM aliases) during TELEM migration — do not rename C fields to match canvas.

### Examples from reference modules

```c
/* ble_scanner_stats_t — drop reasons, uint64_t where documented */
uint64_t packets_dropped_no_buffer;
uint64_t packets_dropped_by_filter[BLE_SCAN_FILTER_COUNT];
latency_stats_t callback_time_us;

/* pending_stats_t — gauge + counter distinction */
uint64_t registered;       /* counter */
uint32_t inflight_now;     /* gauge */
latency_stats_t register_to_delivered;

/* tx_channel_stats_t — latency naming */
latency_stats_t alloc_to_enqueue;

/* tlv_processor_stats_t — semantic verbs */
uint32_t parse_errors;
uint32_t publish_errors;
uint32_t buffers_dropped;
latency_stats_t alloc_to_persist;
```

---

## Thread Safety

Zephyr `k_spinlock` is the standard primitive here:
- ISR-safe and SMP-safe
- Designed for short critical sections
- Does not sleep — safe to call from any context including BLE callbacks

**Rule: one logical event → one lock acquisition.**

- **Cold paths** (init/start/stop failure, single drop counter): `STATS_INC`, `STATS_ADD`, `STATS_DEC`, or `STATS_ERROR`
- **Hot paths** (callback exit, enqueue with latency + counters): mandatory `module_stats_on_<event>()` wrapping one `STATS_SCOPE`

`latency_stats_record()` does not take a lock — call it inside `STATS_SCOPE` when updating a `latency_stats_t` embedded in module stats alongside counters. Do not add locking inside `latency_stats_record()` itself.

Do not use `k_mutex` for stats (not ISR-safe). Do not use raw `atomic_t` fields for module counters (requires `atomic_get()` wrappers on every read, pollutes telemetry and tests).

**Getter reads:** `module_get_stats(out)` acquires `s_stats_lock` once and copies the full struct into caller-owned memory. Each caller uses its own stack-local buffer — no module-global view buffer, no `@caller_sync` dependency.

---

## What NOT to Adopt from Zephyr

| Pattern | Why rejected |
|---|---|
| Legacy `<zephyr/stats/stats.h>` unlocked `STATS_INC(group, var)` | No spinlock; races with ISR writers |
| Net inline unlocked increment macros | Same |
| Live pointer getters (`return &s_stats`, `stats_group_find()`) | Torn reads on 32-bit; alias lifetime issues |
| Side counters outside `{module}_stats_t` | Breaks reset, TELEM, and shell consistency |
| `stats_walk()` without lock | Fine for pre-boot registered groups; wrong for BLE hot-path counters |
| Per-field atomics for module counters | Pollutes TELEM; inconsistent with `STATS_*` recipe |
| Zephyr BT host public stats | Does not exist — app modules own BLE observability |
| Ad-hoc per-subsystem reset with no fleet policy | Breaks `@@PHASE_DELTA@@` monotonicity |

**Adopt from Zephyr kernel:** spinlock-guarded embedded structs, copy-out getters, settle-then-copy for derived fields, optional raw-vs-query split for tooling only.

---

## Reset Policy (expanded)

### Zephyr baseline

Zephyr's stats subsystem (`stats.c`):
- `stats_init()` zeroes a group at registration via `stats_reset()` (plain `memset`)
- `stats_reset(hdr)` is public — any caller can reset any group at any time
- `stats_group_walk()` assumes groups are registered before OS start and are **not locked** during reads
- MCUmgr stat management exposes **list** and **show** only — no SMP reset command in NCS v3.2.4
- Subsystems (e.g. `pm_stats.c`) register ad hoc; no fleet-wide reset policy

### Sigmation policy (stricter, intentional)

| When | Mechanism | Rule |
|---|---|---|
| First init | `STATS_RESET(s_stats)` in `module_init()` | No concurrent writers yet — lock not required |
| After init | No reset on `start()`, `stop()`, or `shutdown()` | Counters reflect device/runtime history |
| Explicit fleet reset | `{module}_reset_stats()` registered in `system_lifecycle_reset_all_stats()` | **Only** lifecycle-registry modules may expose public reset |

**Why stricter than Zephyr:**
1. **Host tooling** — `@@TELEM@@` and `@@PHASE_DELTA@@` assume monotonic counters between snapshots; mid-run resets produce negative deltas
2. **Diagnostic continuity** — drop/error counters survive subsystem restarts
3. **Concurrency** — `STATS_RESET` is multi-word `memset`; without spinlock it races `STATS_INC`/`STATS_SCOPE` (audit XC-7)
4. **Single front door** — `sys stats_reset` → `system_lifecycle_reset_all_stats()` resets a known set in defined order

**Runtime reset lock rule:**

```c
int module_reset_stats(void)
{
    k_spinlock_key_t key = k_spin_lock(&s_stats_lock);
    STATS_RESET(s_stats);
    k_spin_unlock(&s_stats_lock, key);
    return 0;
}
```

`STATS_RESET` without lock is valid **only** in `module_init()` before any ISR/callback can write stats.

**Ordering rule with `mod_claim_init()` (Pattern B modules):** Call `STATS_RESET(s_stats)` as the very first statement in `module_init()`, **before** `mod_claim_init()`. Once `mod_claim_init()` succeeds the state is INITIALIZED, and any concurrent call to `module_get_stats()` (from the shell, TELEM, or another thread) can run `STATS_COPY_OUT` under `s_stats_lock`. If `STATS_RESET` runs after `mod_claim_init()`, the multi-word `memset` races the spinlock-protected copy-out and produces a torn stats snapshot (audit findings PT-003, PT-004).

**Lifecycle reset registry (current):**

```
system_lifecycle_reset_all_stats()
  ├── logger_reset_stats()
  ├── battery_reset_stats()
  ├── heartbeat_app_reset_stats()
  ├── reception_subsystem_reset_stats()
  │     ble_scanner, ble_scan_buffer, ble_state_table,
  │     reception, tlv_processor, art_reset_stats
  └── transmission_reset_stats()
        tx_channel, tx_air, tx_batch, pending (+ inflight),
        transmission_rx, tx_admit, transmission orchestrator errors
```

Modules **not** in this registry (`spam`, pingpong apps, etc.) must not expose public `*_reset_stats()` unless added to the registry with a documented reason.

**Pingpong exemption:** `pp_metrics_reset()` at scenario boundaries resets test-harness aggregates (`@@PP@@` layer), not fleet subsystem counters. Intentional — does not violate lifetime accumulation policy for wired modules.

| Aspect | Zephyr `stats_*` | Sigmation recipe |
|---|---|---|
| Reset scope | Per group, any caller | Init + optional lifecycle only |
| Reset discovery | `stats_group_find(name)` | Fixed lifecycle registry |
| Read safety | Unlocked walk | Copy-out getter or TELEM snapshot |
| BLE stack | No public stats | App-level counters in `ble_scanner`, `tx_air`, etc. |

---

## Exposure Layers

Stats flow through four layers. Each has different consumers and contracts.

```
  Hot path (ISR / callback / WQ)
       │  STATS_INC / STATS_ERROR / STATS_SCOPE
       ▼
  ┌─────────────────────────────────────────────────────────┐
  │ L1  module stats struct  (s_stats + s_stats_lock)      │
  └─────────────────────────────────────────────────────────┘
       │  module_get_stats(&out)  — copy-out under lock
       ▼
  ┌─────────────────────────────────────────────────────────┐
  │ L2  Shell  (<module> stats)  — human inspection        │
  └─────────────────────────────────────────────────────────┘
       │  system_telemetry_dump() / phase_begin|end
       ▼
  ┌─────────────────────────────────────────────────────────┐
  │ L3  @@TELEM@@ / @@PHASE_DELTA@@  — fleet snapshot      │
  └─────────────────────────────────────────────────────────┘
       │  host parse (canvas_render.py, log scrapers)
       ▼
  ┌─────────────────────────────────────────────────────────┐
  │ L4  Host canvas / analysis  — joins TELEM + @@PP@@     │
  └─────────────────────────────────────────────────────────┘

  Parallel app path (pingpong only):
  pp_engine round hooks → pp_metrics → @@PP@@ {"ev":"cell"|"round_lat"|...}
```

| Layer | API / sentinel | Consumer | Contract |
|---|---|---|---|
| L1 Getter | `int foo_get_stats(foo_stats_t *out)` | Shell, TELEM, tests | Caller-owned buffer; `-EINVAL` on NULL |
| L2 Shell | `SHELL_CMD(stats, …)` in `*_shell_cmds.c` | Developer at RTT | All fields including `error_count` / `last_error_code` |
| L3 TELEM | `@@TELEM@@`, `@@PHASE_DELTA@@` | Python pipeline, CI | Single `printk` line; `@caller_sync` |
| L4 Host | `canvas_render.py`, stats API | Dashboards, regression | Parses sentinels; does not call firmware getters |

**TELEM subsystem keys (implemented in `system_telemetry.c`):**

`tx_channel`, `tx_air`, `tx_batch`, `pending`, `tx_admit`, `rx`, `scanner`, `scan_buf`, `state_table`, `tlv`, `reception`, `logger`, `battery`

**TELEM requirements when migrating a module:**
- Emit `"err"` and `"last_err"` for **every** wired module whose struct carries error sentinels (today only 3/13 keys include them — gap to close)
- Monotonic counters → delta in `@@PHASE_DELTA@@` (`end − begin`)
- Gauges (`inflight_now`, `inflight_high_water`) and embedded `latency_stats_t` → end-state at phase close, not delta
- `snap_capture()` copies structs by value; dump reads via getters — both need copy-out safety


**Pingpong / `@@PP@@`:** Subsystem modules follow the C recipe. `pp_metrics` is a separate host-json layer (per-cell round aggregates). Host canvas joins `@@PP@@` with `@@TELEM@@` — no `module_get_stats()` bridge required.

---

## Optional Zephyr Stats / MCUmgr Bridge

Sigmation does not use `stats_register()` today. Exposure is getters + TELEM. An optional bridge enables SMP stat show/list for external tools without replacing the recipe.

**Off by default.** Enable only for factory rigs or mcumgr clients:

```kconfig
CONFIG_STATS=y
CONFIG_STATS_NAMES=y
CONFIG_MCUMGR=y
CONFIG_MCUMGR_GRP_STAT=y
CONFIG_SIGMATION_STATS_MCUMGR=y   # project gate
```

**Adapter pattern — snapshot-under-lock:**

Never point `stats_hdr` directly at live `s_stats`. Zephyr stat reads copy scalars without locking; Sigmation stats mutate under spinlocks from ISRs.

```c
static foo_stats_t s_mcumgr_snap;

static void foo_mcumgr_refresh(void)
{
    foo_get_stats(&s_mcumgr_snap);   /* copy-out under lock */
    STATS_SET(foo_mcumgr, error_count, s_mcumgr_snap.error_count);
    /* scalars only — skip latency_stats_t blobs unless names are stable */
}
```

Rules:
1. Export **scalars only** — map `uint32_t`/`uint64_t`; keep `latency_stats_t` in TELEM/shell as nested JSON
2. Refresh snapshot under `s_stats_lock` before MCUmgr read
3. Do **not** wire Zephyr `stats_reset()` to live `s_stats` — reset via lifecycle only
4. Group naming: `"sig_<module>"` (e.g. `"sig_pending"`)
5. MCUmgr provides **read** only in NCS 3.2.4 — fleet reset remains `system_lifecycle_reset_all_stats()`

Not a replacement for `@@TELEM@@` (structured JSON, role tagging, phase deltas).

---

## Exempt Modules

| Module / layer | Exemption | Rationale |
|---|---|---|
| `latency_stats` | Utility, not a module | Embedded struct + `latency_stats_record()`; no `s_stats`, no getter, no reset |
| `system_telemetry` | Aggregator | No `error_count`; serializes other modules' getters into `@@TELEM@@` |
| `inflight_table` | Library embedded in `pending` | Per-table instance; stats folded into `pending_stats_t` for TELEM |
| `pp_metrics` (+ `pp_lat`, `pp_scenario`) | Host-json test harness | `@@PP@@` emission; `pp_metrics_reset()` at scenario boundaries |
| `board` / `device_identity` | Creed-exempt init | No stats struct |

Do not force migration on exempt modules.

---

## Module Checklist

For each module being migrated:

### Recipe compliance
- [ ] Remove all lifecycle fields (`init_count`, `start_count`, `stop_count`, `shutdown_count`, `is_running`, `total_runtime_ms`, `last_start_time`)
- [ ] Ensure `error_count` and `last_error_code` are present and incremented on every non-fatal failure path
- [ ] Replace all direct `field++` / `atomic_inc` / mutex-guarded increments with `STATS_INC` / `STATS_ADD` / `STATS_ERROR`
- [ ] Hot paths: mandatory private `{module}_stats_on_{event}()` helper; one `STATS_SCOPE` per logical event; `latency_stats_record()` inside scope
- [ ] Replace existing reset logic with `STATS_RESET(s_stats)` in `module_init()` only (no lock)
- [ ] Rename internal stats variable to `s_stats`
- [ ] Add `static struct k_spinlock s_stats_lock` if not present (not `K_SPINLOCK_DEFINE`)
- [ ] Fold any side counters into `{module}_stats_t`

### Getter / read safety
- [ ] Replace `const {module}_stats_t *{module}_get_stats(void)` with `int {module}_get_stats({module}_stats_t *out)`
- [ ] Implement via `STATS_COPY_OUT(&s_stats_lock, s_stats, out)` or settle-then-copy (pending pattern)
- [ ] Remove `s_stats_view` / `s_stats_snapshot` static buffers used only for getters
- [ ] Update all call sites (telemetry, shell, tests) to stack-local `{module}_stats_t snap`
- [ ] Remove `@caller_sync` from getter once copy-out is in place

### Reset
- [ ] If `{module}_reset_stats()` exists: holds `s_stats_lock` around `STATS_RESET`; registered in `system_lifecycle_reset_all_stats()` or removed from public API
- [ ] No reset on `start()` / `stop()` / `shutdown()`

### TELEM integration (if module is in the 13-key set)
- [ ] Added to `snap_capture()` / `system_telemetry_dump()` with stable JSON key name
- [ ] Emits `"err"` and `"last_err"` whenever struct has error sentinels
- [ ] Phase delta: counters subtract; embedded `latency_stats_t` and gauges emit end-state
- [ ] Fix reception/canvas key mismatch if touching reception TELEM

### Shell / observability
- [ ] `config`, `stats`, `state` subcommands in dedicated `*_shell_cmds.c`
- [ ] Silent drop paths have explicit counter fields (not only logs)

### Latency embedding
- [ ] If struct contains `latency_stats_t`: record only inside `STATS_SCOPE`
- [ ] Handle `samples` uint32 wrap (S-10): skip avg recompute or reset aggregate on wrap

### Post-migration verification
- [ ] Shell `sys stats_reset` → all registered modules zero; TELEM line shows zeros
- [ ] Phase bracket: `phase_begin` / run / `phase_end` → `@@PHASE_DELTA@@` deltas match manual TELEM subtraction
- [ ] Host canvas parses new TELEM fields without schema regressions

---

## Module Inventory

23 audited areas with stats touchpoints. Use as migration tracker beyond the original 13-name list.

| # | Module | Stats API | TELEM key | Lifecycle reset | Notes |
|---|---|---|---|---|---|
| 1 | `tx_channel` | COMPLIANT | `tx_channel` | via transmission | copy-out getter; TELEM err/last_err |
| 2 | `tx_air` | COMPLIANT | `tx_air` | via transmission | copy-out; STATS_SCOPE hot-path helpers |
| 3 | `tx_batch` | COMPLIANT | `tx_batch` | via transmission | copy-out; batch dwell latency |
| 4 | `pending` (+ `inflight_table`) | COMPLIANT | `pending` | via transmission | materialized view; inflight library exempt |
| 5 | `tx_admit` | COMPLIANT | `tx_admit` | via transmission | copy-out under admit lock |
| 6 | `transmission_rx` | COMPLIANT | `rx` | via transmission | copy-out; peek_lat in STATS_SCOPE |
| 7 | `transmission` | COMPLIANT | — | yes | facade; error-only stats; TELEM via submodules (no parent block — intentional) |
| 8 | `ble_scanner` | COMPLIANT | `scanner` | reception subtree | hot-path reference |
| 9 | `ble_scan_buffer` | COMPLIANT | `scan_buf` | reception subtree | |
| 10 | `ble_state_table` | COMPLIANT | `state_table` | reception subtree | persist failures → error_count |
| 11 | `reception` | COMPLIANT | `reception` | reception subtree | TELEM tlv_sub/sr_routed aligned |
| 12 | `tlv_processor` | COMPLIANT | `tlv` | reception subtree | submit_errors in STATS_SCOPE |
| 13 | `logger` | COMPLIANT | `logger` | yes | copy-out; commit-path helpers |
| 14 | `battery` | COMPLIANT | `battery` | yes | copy-out getter |
| 15 | `heartbeat_app` | COMPLIANT | — | yes | copy-out; no TELEM |
| 16 | `spam` | COMPLIANT | — | init only | copy-out; init-only STATS_RESET; no lifecycle reset (by design for stress app) |
| 17 | `art_test` | COMPLIANT | — | reception subtree | copy-out; harness stats |
| 18 | `pp_engine` / `pp_faker` | — | — | exempt | app layer |
| 20 | `pp_metrics` | EXEMPT | — | exempt | `@@PP@@` host-json |
| 21 | `runtime_config` | COMPLIANT | — | no | copy-out settings error counters |
| 22 | `scan_recorder` | COMPLIANT | — | reception init | copy-out; `persist_ok`/`persist_fail` in getter; init-only STATS_RESET |

**Reference tier:** `ble_scanner`, `ble_scan_buffer` — COMPLIANT on Stats API + latency.

---

## Modules to Migrate (original list)

Priority order: P0 copy-out getters + hot-path helpers → P1 side counters + TELEM errors → P2 apps + `runtime_config` → P3 optional mcumgr bridge.

1. `spam`
2. `eeprom_fs` (planned — no stats struct yet)
3. `battery`
4. `logger`
5. `heartbeat_app`
6. `ble_scanner` — hot path uses `scan_stats_on_callback()` + `STATS_SCOPE` (reference implementation)
7. `ble_scan_buffer`
8. `ble_state_table`
9. `transmission`
10. `latency_stats` — **exempt** (utility library)
11. `tlv_processor`
12. `art_test`

Additional modules from inventory (not in original list): `tx_channel`, `tx_air`, `tx_batch`, `tx_admit`, `pending`, `transmission_rx`, `reception`.
