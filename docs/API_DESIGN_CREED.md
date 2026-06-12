# API Design Creed

Standard pattern for all application-level modules/services in this firmware.

Hardware bring-up module (`board.c`) uses `SYS_INIT` directly and is exempt from the full lifecycle below.

---

## 1. Lifecycle: Init → Start → Stop → Shutdown

```
init() → [register_callback()] → start() → [operational] → stop() → [can restart] → shutdown() → [must re-init]
```

**`int <module>_init(<dependencies>)`**
- Dependency injection via parameters — no globals, no hidden singletons
- Initialize state, validate dependencies, allocate one-time resources (`k_malloc` allowed here only)
- DO NOT start threads, timers, or submit work
- Idempotent: already-INITIALIZED → no-op, return `-EALREADY`
- Return `0` on success, negative errno on failure

**`int <module>_register_callback(<module>_cb_t cb, void *user_ctx)`** *(if module emits events)*
- Called after `init()`, before `start()`
- Callback signature must include `void *user_ctx` — callers must not rely on globals
- Document dispatch context (which thread / work queue) in the header

**`int <module>_start(void)`**
- Start threads, timers, submit initial work
- Transition to STARTED
- Idempotent: already-STARTED → no-op, return `0`

**`int <module>_stop(void)`**
- Stop threads, timers, cancel pending work
- Flush and sync — module remains INITIALIZED, `start()` may be called again
- Idempotent: already-STOPPED → no-op, return `0`

**`int <module>_shutdown(void)`**
- Full cleanup; lands in UNINITIALIZED — must re-`init()` to reuse
- Legal from any state; from STARTED performs implicit `stop()` first
- Release any resources allocated in `init()`
- Always idempotent — second call is a safe no-op

**Minimal lifecycle** — modules with no threads, timers, or restartable state may omit `start()` and `stop()`. Mark the header:
```c
/* lifecycle: init / shutdown only */
```
The `*_state_t` enum then only needs `UNINITIALIZED` and `INITIALIZED`.

**`int <module>_reset_stats(void)`**
- Zero all runtime counters without re-init
- Safe to call in any state ≥ INITIALIZED
- Return `0` on success (always succeeds today — no failure paths)

Note: `latency_stats_reset(latency_stats_t *)` is a utility helper that resets an
embedded stats object. It is **not** the module lifecycle API — do not rename it.

### Legal State Transitions

```
UNINITIALIZED → INITIALIZED               (init)
INITIALIZED   → STARTED                   (start)
INITIALIZED   → UNINITIALIZED             (shutdown, never started)
STARTED       → STOPPED                   (stop)
STARTED       → UNINITIALIZED             (shutdown; implicit stop)
STOPPED       → STARTED                   (start, restartable)
STOPPED       → UNINITIALIZED             (shutdown)
ERROR         → UNINITIALIZED             (shutdown; only recovery path)
```

Any other transition returns `-EINVAL`. ERROR is terminal until `shutdown()`.
To change config: `stop() → shutdown() → init(new_cfg) → start()`.

### State Storage Selection

Pick the backing storage by answering one question:

> **Does any API _outside_ the lifecycle path read state from another thread or ISR?**

| Answer | Backend | Storage |
|--------|---------|---------|
| No — lifecycle is the only reader/writer | **Pattern A** — plain enum | `static module_state_t s_state = MODULE_STATE_UNINITIALIZED;` |
| Yes — `@threadsafe` or `@isr_safe` functions check state | **Pattern B** — atomic | `static atomic_t s_state;` *(holds `int` values of `module_state_t`)* |

The lifecycle functions themselves remain `@caller_sync` in both patterns. Atomics exist solely to make the hot-path ready-check safe — not to serialize lifecycle calls.

#### Pattern A — plain enum (no concurrent state reads)

```c
/* In .c */
static module_state_t s_state = MODULE_STATE_UNINITIALIZED;
```

Transitions are plain assignments inside `@caller_sync` lifecycle functions.

#### Pattern B — atomic_t (hot-path or ISR state reads)

Use the helpers in `src/system/module_lifecycle.h`:

```c
/* In .c */
static atomic_t s_state; /* zero-init == MODULE_STATE_UNINITIALIZED */
```

**Init claim — with mandatory rollback on failure:**
```c
if (!mod_claim_init(&s_state, MODULE_STATE_UNINITIALIZED, MODULE_STATE_INITIALIZED)) {
    return -EALREADY;
}
rc = /* setup */;
if (rc != 0) {
    mod_init_abort(&s_state, MODULE_STATE_INITIALIZED, MODULE_STATE_UNINITIALIZED);
    return rc;
}
```

**Shutdown claim — idempotent:**
```c
if (!mod_claim_shutdown(&s_state, MODULE_STATE_INITIALIZED, MODULE_STATE_UNINITIALIZED)) {
    return; /* already shut down */
}
/* cleanup */
```

**Hot-path / `@threadsafe` / `@isr_safe` ready-check:**
```c
if (mod_get_state(&s_state) != MODULE_STATE_INITIALIZED) {
    return -ENODEV;
}
```

**Never** use `atomic_get` + later `atomic_set` for init — that is a TOCTOU race. Always use `mod_claim_init` (CAS).

Both patterns MUST expose `get_state()` in the public header. Pattern B maps the atomic value back to the enum:
```c
module_state_t module_get_state(void) {
    return (module_state_t)atomic_get(&s_state);
}
```

---

## 2. Required Structures

**`<module>_config_t`** — configuration parameters, frozen after `init()`
**`<module>_stats_t`** — runtime statistics (see §7 for required fields)
**`<module>_state_t`** — state enum, values MUST be prefixed:
- `<MODULE>_STATE_UNINITIALIZED`
- `<MODULE>_STATE_INITIALIZED`
- `<MODULE>_STATE_STARTED` *(omit if minimal lifecycle)*
- `<MODULE>_STATE_STOPPED` *(omit if minimal lifecycle)*
- `<MODULE>_STATE_ERROR`

Minimal-lifecycle modules (init/shutdown only) only need `UNINITIALIZED` and `INITIALIZED`.

## Required Getters

**`const <module>_config_t* <module>_get_config(void)`**
**`const <module>_stats_t*  <module>_get_stats(void)`**
**`<module>_state_t          <module>_get_state(void)`**

Getters MUST always return a valid (non-NULL) pointer. Back them with a file-static struct, zero-initialized at load time. Safe to call from any context including before `init()`.

---

## 3. Thread-Safety Classification

Every public function MUST carry one of these labels in its header:

- `@threadsafe`  — safe to call concurrently from multiple threads (takes internal lock)
- `@isr_safe`    — safe from ISR (lock-free or atomic only)
- `@caller_sync` — caller must ensure no concurrent calls

### `@isr_safe` Backing Requirements

A function annotated `@isr_safe` MUST read only:
- Individual `atomic_t` values (via `atomic_get` / `mod_get_state`)
- Fields protected by a `k_spinlock` (spinlock held during the read)
- Single machine-word values (≤ 4 bytes, naturally aligned) where the atomicity is explicitly documented as hardware-guaranteed

Reading a struct with more than one word field without a spinlock is **never** `@isr_safe`, even if the struct members are individually word-sized. The annotation must be backed by actual synchronization — not just by the code happening to work on the current CPU.

---

## 4. Error Codes

Use standard errno from `<errno.h>`: `-EINVAL`, `-EBUSY`, `-ENOTCONN`, `-EAGAIN`, `-ENOMEM`, `-EIO`.
Do not invent module-specific negative codes.

---

## 5. Config Mutation

Config is **frozen after `init()`**. If a field legitimately needs runtime tuning, expose a dedicated `<module>_set_<field>()` setter. Never expose the whole config struct for mutation.

---

## 6. Behavioral FSM Pattern

Lifecycle states (UNINITIALIZED → INITIALIZED → STARTED → STOPPED → ERROR) are managed by the hand-rolled lifecycle above. SMF is not warranted for this five-state linear machine.

**Use Zephyr SMF (`smf.h`) for behavioral FSMs** when a module has:
- ≥ 3 distinct states with entry/exit side effects, OR
- Hierarchical states (parent handles common transitions such as error recovery)

Examples in this codebase: per-channel TX IDLE/BUSY, proximity zone FSM.

### SMF Rules

- `struct smf_ctx` MUST be the **first member** of the FSM context struct
- Entry/exit functions manage hardware or resource state — the run function handles events only
- All transitions via `smf_set_state()` — never write the state field directly
- State enum values prefixed `<MODULE>_SMF_STATE_*` (distinct from the lifecycle state enum)
- Kconfig: `CONFIG_SMF=y`; add `CONFIG_SMF_ANCESTOR_SUPPORT=y` for hierarchical machines

```c
struct proximity_fsm_obj {
    struct smf_ctx ctx;        /* MUST be first */
    int16_t        rssi_ema;
    uint32_t       last_seen_ms;
};

enum proximity_smf_state {
    PROX_SMF_STATE_UNKNOWN,
    PROX_SMF_STATE_NEAR,
    PROX_SMF_STATE_FAR,
};

static const struct smf_state prox_states[] = {
    [PROX_SMF_STATE_UNKNOWN] = SMF_CREATE_STATE(prox_unknown_entry, prox_unknown_run, NULL),
    [PROX_SMF_STATE_NEAR]    = SMF_CREATE_STATE(prox_near_entry,    prox_near_run,    prox_near_exit),
    [PROX_SMF_STATE_FAR]     = SMF_CREATE_STATE(prox_far_entry,     prox_far_run,     NULL),
};
```

### Exemptions

**`ble_proximity_fsm`** (`src/ble/reception/ble_state_table/row_rssi/proximity_fsm/`) is exempt from SMF:

- Debounce and zone-band logic are **pure functions** with no module-static state.
- Committed and pending zone state lives in caller-owned `ble_state_row_t` fields (`zone`, `pending_zone`, `pending_count`) — not in an SMF context struct.
- No entry/exit side effects (no hardware or resource management on zone transitions).
- Runs synchronously on `rx_pipeline_wq` inside `ble_state_row_rssi_proximity_update()` — one sample in, updated row fields out.

Do not use this exemption as precedent for FSMs that arm peripherals, emit events from entry handlers, or hold private transition state outside the row struct.

---

## 7. Inter-Module Communication and Callback Patterns

Every boundary between modules uses exactly one of the four patterns below. The pattern
is chosen based on coupling characteristics — not preference. See `NETWORKING.md` for the
per-boundary coupling map.

### Pattern A — Ops struct (vtable)

**When:** Tight coupling, per-object lifetime, 3+ related callbacks all firing against
the same object. The lower layer owns the calling site entirely.

```c
/* Lower layer defines the ops struct */
struct my_module_ops {
    void (*on_event_a)(struct my_obj *obj, int status);
    void (*on_event_b)(struct my_obj *obj, const uint8_t *data, size_t len);
    struct net_buf *(*alloc_buf)(struct my_obj *obj);
};

int my_module_register_ops(const struct my_module_ops *ops);  /* before start() */
```

**Rules:**
- `ops` pointer must remain valid for the lifetime of the module
- All ops functions run on the documented dispatch thread
- NULL function pointers in the ops struct mean "no handler" — callers must NULL-check

### Pattern B — Register function with embedded list node

**When:** Optional, independently removable callbacks. Multiple modules may subscribe
to the same event. One or two callback types per event.

```c
typedef void (*my_event_fn)(int status, void *user_ctx);

struct my_event_cb {
    sys_snode_t  node;          /* embedded — caller allocates this struct */
    my_event_fn  fn;
    void        *user_ctx;
};

int  my_module_register_event_cb(struct my_event_cb *cb,
                                  my_event_fn fn, void *user_ctx);
void my_module_unregister_event_cb(struct my_event_cb *cb);
```

**Invocation must be under the same spinlock used during registration:**
```c
/* Caller allocates the node; no heap required */
static struct my_event_cb s_event_cb;
my_module_register_event_cb(&s_event_cb, my_handler, my_ctx);
```

### Pattern C — Static linker section (compile-time)

**When:** System-wide, always-present subscribers. The set is known at compile time
and does not change at runtime. Zero overhead; no list traversal at registration.

```c
/* In the module's header */
struct my_handler {
    uint16_t         type;
    my_handler_fn    fn;
};

#define MY_HANDLER_DEFINE(_name, _type, _fn)                        \
    const STRUCT_SECTION_ITERABLE(my_handler, _name) = {            \
        .type = (_type),                                             \
        .fn   = (_fn),                                               \
    }

/* In any translation unit — zero registration call required */
MY_HANDLER_DEFINE(my_record_handler, RECORD_TYPE_ACCESS, ac_on_record);

/* Runtime dispatch — walks the section */
STRUCT_SECTION_FOREACH(my_handler, h) {
    if (h->type == received_type) { h->fn(args); }
}
```

### Pattern D — ZBus (pub/sub)

**When:** Loose coupling. Publisher does not know its consumers. Multiple independent
modules react to the same event independently.

```c
/* Channel defined once in a shared header */
ZBUS_CHAN_DEFINE(battery_chan, struct battery_msg, NULL, NULL,
                 ZBUS_OBSERVERS(heartbeat_sub), ZBUS_CHAN_DEFAULTS);

/* Publisher — from a work handler, not from BT RX */
void battery_work_handler(struct k_work *w) {
    struct battery_msg msg = { .pct = battery_get_percent() };
    zbus_chan_pub(&battery_chan, &msg, K_NO_WAIT);
}
```

**Rules for all patterns:**
- Callbacks registered via `register_*()` or statically — never passed to `init()`
- Every callback signature includes `void *user_ctx` — callers never rely on globals
- Dispatch thread is documented in the header for every callback
- Modules depend only on shared channel/handler definitions — never `#include` another
  module's header solely to route data
- **Never publish to ZBus from ISR or BT RX context** — defer to a work queue first
- Callbacks must not block: no mutex with timeout, no `k_sleep`, no `k_sem_take`

### user_ctx naming

All callback registration functions use the parameter name **`user_ctx`** — not
`user_data`, not `priv`, not `arg`. Callers may pass NULL if no context is needed.

```c
/* Correct */
int module_register_cb(module_event_fn fn, void *user_ctx);

/* Wrong */
int module_register_cb(module_event_fn fn, void *user_data);  /* non-standard name */
```

---

## 8. Workqueue Contract

### Selecting the Right Primitive

| Primitive | When to use |
|---|---|
| `k_work` | One-shot deferred work — the default choice |
| `k_work_delayable` | Only when a timeout path actually exists; not a substitute for `k_work` with `K_NO_WAIT` |
| System workqueue (`k_sys_work_q`) | Never from application code — always use a named custom work queue |

### Work Handler Rules

- MUST NOT sleep (`k_sleep`, `k_usleep`)
- MUST NOT take a semaphore or mutex with a non-zero timeout in a hot-path handler
- MUST be bounded: document worst-case execution time in the module README
- MUST NOT allocate memory dynamically (see §9)

### Teardown: Always Use `k_work_cancel_delayable_sync`

In any shutdown or teardown path where a work item holds a reference to the resource being freed or deleted, use `k_work_cancel_delayable_sync()`. The non-sync variant (`k_work_cancel_delayable()`) only cancels a pending (not yet running) work item — if the item is already executing, it returns immediately without waiting, and the handler will continue to run against freed state.

```c
/* WRONG — handler may still be running after this returns */
k_work_cancel_delayable(&set->work);
bt_le_ext_adv_delete(set->adv);

/* CORRECT — guaranteed handler is not running when this returns */
struct k_work_sync sync;
k_work_cancel_delayable_sync(&set->work, &sync);
bt_le_ext_adv_delete(set->adv);
```

The sync variant must be called from thread context (not ISR, not spinlock-held). All shutdown functions are already `@caller_sync` thread-context functions.

### No Kernel Scheduling APIs Inside Spinlocks

Never call `k_work_submit_to_queue()`, `k_work_schedule_for_queue()`, `k_sem_give()`, or any Zephyr kernel API that touches the scheduler while holding a `k_spinlock`. These APIs internally acquire kernel-level locks. Calling them from inside a spinlock violates Zephyr's lock ordering, is forbidden on SMP builds, and can produce silent deadlocks or kernel assertion failures.

```c
/* WRONG */
k_spinlock_key_t key = k_spin_lock(&s_lock);
fill_slot(s_packs, idx, data);
k_work_schedule_for_queue(wq, &work, K_MSEC(timeout)); /* scheduler API inside spinlock */
k_spin_unlock(&s_lock, key);

/* CORRECT */
k_spinlock_key_t key = k_spin_lock(&s_lock);
fill_slot(s_packs, idx, data);
k_spin_unlock(&s_lock, key);
k_work_schedule_for_queue(wq, &work, K_MSEC(timeout)); /* outside spinlock */
```

### Work Queue Priority (this firmware)

| Queue | Priority | Owner |
|---|---|---|
| `tx_pipeline_wq` | 4 | `src/system/workqueues/tx_pipeline_wq.c` |
| `rx_pipeline_wq` | 6 | `src/system/workqueues/rx_pipeline_wq.c` |

New work queues: document priority, stack size, and owner module in the README before merging.
`tx_pipeline_wq` preempts `rx_pipeline_wq` by design — preserve this ordering.

---

## 9. Memory Model

| Context | Rule |
|---|---|
| ISR / BT RX thread | Zero allocation — no exceptions |
| Work handlers (hot path) | Zero allocation — use pre-allocated pools |
| `init()` | `k_malloc` / `k_heap_alloc` allowed — one-time setup only |
| `start()` / `stop()` / `shutdown()` | No allocation; no free of hot-path resources |

**Data passing between contexts:** Use `net_buf` from static pools for zero-copy transfer.
`net_buf_alloc()` failure is a normal condition — increment `stats.dropped`, return without asserting.
Never `__ASSERT` on an allocation failure in a hot path.

**Static footprint documentation:** Every module header MUST include a memory comment:
```c
/*
 * Memory: 1x ble_scanner_ctx_t (48 B, static)
 *         net_buf pool: 16 x 256 B = 4096 B (CONFIG_BLE_SCAN_POOL_COUNT)
 */
```

### No `static` Local Buffers

Never declare a buffer `static` inside a function that may be called concurrently or from multiple contexts. A `static` local variable is a BSS singleton — it is shared across all concurrent invocations exactly like a global, negating any apparent "locality."

**Wrong:**
```c
void encode(struct net_buf *buf) {
    static uint8_t tmp[256]; /* shared across all callers */
    ...
}
```

**Correct:**
```c
void encode(struct net_buf *buf) {
    uint8_t tmp[256]; /* per-call stack allocation */
    ...
}
```

Stack cost is always acceptable for buffers ≤ 512 B in work queue handlers (both pipeline work queues are ≥ 2 KiB). For larger buffers, use a `K_MEM_SLAB_DEFINE` allocated in `init()`.

---

## 10. Power Management Contract

**Modules that own a Zephyr device node** MUST implement `pm_device_action` callbacks:

| Action | Required behaviour |
|---|---|
| `PM_DEVICE_ACTION_SUSPEND` | Flush pending work, release peripheral, set state to STOPPED |
| `PM_DEVICE_ACTION_RESUME` | Re-arm peripheral, restore operational state |
| `PM_DEVICE_ACTION_TURN_OFF` | Full power-down (if SoC power domain supports it) |
| `PM_DEVICE_ACTION_TURN_ON` | Full power-up and re-init of peripheral |

**Modules that do not own hardware** are PM-transparent — no hooks required.

PM callbacks MUST NOT block. They may be invoked from a context where sleeping is unsafe.

```kconfig
CONFIG_PM=y
CONFIG_PM_DEVICE=y
CONFIG_PM_DEVICE_RUNTIME=y
```

---

## 11. Observability Contract

### Stats Struct Requirements

Every `<module>_stats_t` MUST carry `uint32_t error_count` and `int32_t last_error_code`
(see `STATS_API_DESIGN.md` for the full naming convention):

- Any path that can **silently drop data** MUST have a named drop counter (e.g. `buffers_dropped_no_buffer`)
- Any path that can **fail non-fatally** MUST increment `error_count` and set `last_error_code`
- `<module>_reset_stats()` MUST zero every counter — never omit one

### Shell Commands

Every module with runtime state MUST register a shell subcommand that prints `config`, `stats`, and `state`.
Shell commands MUST live in a dedicated `<module>_shell_cmds.c` file — never inside the module's core `.c` file.

```c
SHELL_STATIC_SUBCMD_SET_CREATE(ble_scanner_cmds,
    SHELL_CMD(config, NULL, "Print config", cmd_ble_scanner_config),
    SHELL_CMD(stats,  NULL, "Print stats",  cmd_ble_scanner_stats),
    SHELL_CMD(state,  NULL, "Print state",  cmd_ble_scanner_state),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(ble_scanner, &ble_scanner_cmds, "BLE scanner control", NULL);
```

### Telemetry Sentinels

Output consumed by the Python analysis pipeline MUST use `printk` (never `LOG_*`) to guarantee atomic single-line output:

```c
printk("@@TELEM@@ {\"dropped\":%u,\"rssi_ema\":%d}\n", stats.dropped, ctx.rssi_ema);
```

- Sentinel tag format: `@@<TAG>@@` followed by a single JSON object on one line
- Never mix sentinel and non-sentinel content in a single `printk` call
- Tags currently in use: `@@PP@@`, `@@TELEM@@`, `@@PHASE_DELTA@@`, `@@THREAD_STATS@@`

### Development Build Requirements

Enable in every development build — not optional:

```kconfig
CONFIG_THREAD_ANALYZER=y
CONFIG_THREAD_ANALYZER_AUTO=y
CONFIG_STACK_SENTINEL=y
CONFIG_THREAD_STACK_INFO=y
```

Use Thread Analyzer output to establish the stack floor before finalising `K_THREAD_STACK_DEFINE` sizes for production. Document the measured peak in the module README.

### `STATS_RESET` Must Hold the Stats Spinlock

`STATS_RESET` performs a `memset` on the stats struct. When called from `*_reset_stats()` (which may be invoked from any context including shell commands), the module's stats spinlock MUST be held:

```c
int module_reset_stats(void)
{
    K_SPINLOCK(&s_stats_lock) {
        STATS_RESET(s_stats);
    }
    return 0;
}
```

The "call only from init" note in `stats.h` applies to calling `STATS_RESET` without a lock. From any live path (including shell `reset_stats` commands), always hold the lock first.

---

## 12. Settings Contract

### Settings Load Handler Always Returns 0

`settings_load_cb` handlers registered with Zephyr's Settings subsystem MUST always return `0`. A non-zero return causes `settings_load_subtree()` to stop calling the handler for all subsequent keys — one corrupt key silently prevents the entire stored configuration from loading.

```c
/* WRONG */
static int my_cfg_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    rc = parse_and_apply(key, len, read_cb, cb_arg);
    return rc; /* if rc != 0, all remaining keys are dropped */
}

/* CORRECT */
static int my_cfg_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    rc = parse_and_apply(key, len, read_cb, cb_arg);
    if (rc != 0) {
        LOG_ERR("Failed to load key '%s': %d", key, rc);
        s_stats.settings_load_errors++;
    }
    return 0; /* always return 0 — settings subsystem must continue with next key */
}
```

---

## 13. Object Lifetime and Reference Counting

Modules that manage a **pool of reusable objects** (inflight entries, pending slots,
device table rows) must guarantee that every callback fires while the object is still
structurally valid — never after the slot is returned to the pool.

This is the pattern used by Zephyr's `bt_conn` and `net_context`. Violating it causes
use-after-free bugs that are silent in testing and fatal under load.

### The rule

> An object is recycled only after its final callback has returned.
> A callback must never fire against a recycled or reused slot.

### Implementation — reference counting

Add `atomic_t ref` as the **last field** in the pooled object struct (so `memset` of
earlier fields does not corrupt it):

```c
struct my_slot {
    /* ... all other fields ... */
    atomic_t ref;  /* MUST be last */
};
```

Provide ref/unref functions. The unref recycles the slot when the count reaches zero:

```c
static void slot_recycle(struct my_slot *s);  /* internal */

struct my_slot *my_slot_ref(struct my_slot *s)
{
    atomic_val_t old;
    do {
        old = atomic_get(&s->ref);
        if (old == 0) { return NULL; }  /* already recycled — refuse */
    } while (!atomic_cas(&s->ref, old, old + 1));
    return s;
}

void my_slot_unref(struct my_slot *s)
{
    if (atomic_dec(&s->ref) == 1) {
        slot_recycle(s);
    }
}
```

**When work is queued against a slot**, take a ref before queuing and release it after
the callback returns:

```c
/* Before submitting work */
if (!my_slot_ref(slot)) { return; }     /* slot already gone */
k_work_submit_to_queue(&my_wq, &slot->work);

/* Inside the work handler */
void my_work_handler(struct k_work *w)
{
    struct my_slot *slot = CONTAINER_OF(w, struct my_slot, work);
    /* ... slot is valid here ... */
    if (slot->event_cb) {
        slot->event_cb(slot->handle, slot->event, slot->user_ctx);
    }
    my_slot_unref(slot);                /* release the ref taken at queue time */
}
```

### When ref counting is not needed

If all callbacks for an object fire **synchronously** from the same thread before the
slot is recycled, ref counting is not required. The simpler invariant is:

> Call the terminal callback inline. Recycle the slot after the callback returns.
> Never queue the callback and recycle the slot in the same work handler.

```c
/* Correct — inline delivery before recycle */
k_spinlock_key_t key = k_spin_lock(&s->lock);
event_fn  cb    = s->event_cb;
void     *ctx   = s->user_ctx;
handle_t  h     = s->handle;
s->generation++;
slot_recycle(s);
k_spin_unlock(&s->lock, key);

if (cb) { cb(h, EVENT_CLOSED, NULL, 0, ctx); }  /* safe: slot already recycled */
```

This is safe because `cb` was captured before the recycle. The slot may be reused
immediately after `slot_recycle()`, but `cb`, `ctx`, and `h` are local copies on the
stack.

---

## Summary of Rules

- All dependencies injected via `init()` parameters — no hidden globals
- Callbacks registered via `register_callback()`, not passed to `init()`
- Return `0` on success, negative errno on failure
- Config/stats/state structures and getters are mandatory; getters never return NULL
- Lifecycle calls are idempotent: repeated `init()` returns `-EALREADY`; `shutdown()` is always a safe no-op
- State storage is `atomic_t` only when hot paths read state concurrently; otherwise plain enum. Both expose `*_state_t` + `get_state()`
- Pattern B init uses CAS (`mod_claim_init`) with mandatory rollback on setup failure — never `atomic_get` + `atomic_set`
- Behavioral FSMs with ≥ 3 states or hierarchy MUST use `smf.h`
- Inter-layer coupling uses one of four patterns (A: ops struct, B: register fn, C: static section, D: ZBus) — see §7
- Callback parameter for caller context is always named `user_ctx` — never `user_data`, `priv`, or `arg`
- Pooled objects with deferred callbacks use `atomic_t ref` at end of struct; slot recycled only when ref hits 0
- Async inter-module data flows through Zbus channels — not direct header includes
- Never publish to Zbus from ISR or BT RX context
- Work handlers must not sleep, must not allocate, must be bounded in execution time
- No dynamic allocation outside `init()`; hot-path failures increment `dropped` — never assert
- Hardware-owning modules implement `pm_device_action` callbacks
- Every silent-failure path has a `dropped` or `errors` counter in `stats_t`
- Every module with runtime state has a shell subcommand for `config` / `stats` / `state`

---

## Example

```c
/* Memory: 1x logger_ctx_t (static) */

typedef struct {
    uint8_t  flash_area_id;
    uint32_t async_queue_size;
    uint32_t async_max_record_size;
} logger_config_t;

typedef struct {
    uint32_t writes_total;
    uint32_t writes_dropped_no_storage;
    uint32_t fcb_append_errors;
} logger_stats_t;

typedef enum {
    LOGGER_STATE_UNINITIALIZED,
    LOGGER_STATE_INITIALIZED,
    LOGGER_STATE_STARTED,
    LOGGER_STATE_STOPPED,
    LOGGER_STATE_ERROR,
} logger_state_t;

int logger_init(const logger_config_t *cfg);   /* @caller_sync */
int logger_start(void);                        /* @caller_sync */
int logger_stop(void);                         /* @caller_sync */
int logger_shutdown(void);                     /* @caller_sync */
int logger_reset_stats(void);                  /* @caller_sync */

int logger_write(record_type_t type,
                 const void *data, uint16_t length); /* @threadsafe */

const logger_config_t *logger_get_config(void); /* @isr_safe */
int                    logger_get_stats(logger_stats_t *out); /* @threadsafe — copy-out under spinlock */
logger_state_t         logger_get_state(void);  /* @isr_safe */
```
