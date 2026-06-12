# Callback Registration Guide

Every module that needs to notify an external consumer follows this guide exactly.
No variations, no legacy patterns.

---

## 1. Canonical Pattern

### Header (`module.h`)

```c
/* Step 1 — typedef: always <module>_<purpose>_fn */
typedef void (*module_purpose_fn)(/* event args */, void *user_ctx);

/* Step 2 — setter declaration */
/// @caller_sync — call only from INITIALIZED or STOPPED state
int module_register_purpose_cb(module_purpose_fn handler, void *user_ctx);
```

### Implementation (`module.c`)

```c
/* Step 3 — internal storage: two paired statics */
static module_purpose_fn  s_purpose_cb;
static void              *s_purpose_user_ctx;

/* Step 4 — setter body */
int module_register_purpose_cb(module_purpose_fn handler, void *user_ctx)
{
    if (s_state == MODULE_STATE_UNINITIALIZED) {
        return -ENODEV;
    }
    if (s_state == MODULE_STATE_STARTED) {
        return -EBUSY;
    }
    if (s_state == MODULE_STATE_ERROR) {
        return -EIO;
    }

    s_purpose_cb       = handler;
    s_purpose_user_ctx = user_ctx;
    return 0;
}

/* Step 5 — invocation site */
if (s_purpose_cb) {
    s_purpose_cb(event_args, s_purpose_user_ctx);
}
```

---

## 2. Rules

### Naming

| Thing | Pattern | Example |
|-------|---------|---------|
| Callback typedef | `<module>_<purpose>_fn` | `reception_route_fn` |
| Setter function | `<module>_register_<purpose>_cb` | `reception_register_route_cb` |
| Internal handler static | `s_<purpose>_cb` | `s_route_cb` |
| Internal ctx static | `s_<purpose>_user_ctx` | `s_route_user_ctx` |

The `_fn` suffix is for the **type** (typedef). The `_cb` suffix is for **variables and function names**.

### Guard — allowed states

| Module state | Return |
|-------------|--------|
| `UNINITIALIZED` | `-ENODEV` |
| `INITIALIZED` | `0` — allowed |
| `STARTED` | `-EBUSY` |
| `STOPPED` | `0` — allowed |
| `ERROR` | `-EIO` |

Registration is only permitted when the module is **not actively running**.

### NULL handler

Passing `NULL` as `handler` is valid. It clears the registration (unregister).  
The module must NULL-check before every invocation:

```c
if (s_purpose_cb) {
    s_purpose_cb(args, s_purpose_user_ctx);
}
```

### user_ctx

`user_ctx` is an opaque pointer owned by the caller. The module stores it and passes it
back on every invocation without reading or modifying it. It may be NULL.

### Thread safety

All setters are `@caller_sync`. The guard enforces the module is not running, so no
concurrent callback invocation can race the pointer write.

### Return value

Always `int`. Always `0` on success. Always negative errno on failure.
Never `void`. Never positive non-zero.

---

## 3. Migration Table

All existing setters must be renamed and updated to this pattern.

| Old function | New function | Typedef change |
|-------------|-------------|----------------|
| `reception_set_route_fn(fn)` | `reception_register_route_cb(handler, ctx)` | `reception_route_fn` — no change |
| `ble_scanner_set_pre_alloc_cb(cb)` | `ble_scanner_register_pre_alloc_cb(handler, ctx)` | `ble_scanner_pre_alloc_fn` — no change |
| `ble_scan_buffer_set_on_enqueue_cb(cb)` | `ble_scan_buffer_register_on_enqueue_cb(handler, ctx)` | `ble_scan_buffer_on_enqueue_fn` — no change |
| `ble_scan_buffer_set_prefilter_cb(cb)` | `ble_scan_buffer_register_prefilter_cb(handler, ctx)` | `ble_scan_buffer_prefilter_fn` — no change |
| `ble_state_table_set_proximity_change_callback(cb)` | `ble_state_table_register_proximity_change_cb(handler, ctx)` | `ble_state_table_proximity_change_cb_t` → `ble_state_table_proximity_change_fn` |
| `ble_state_row_persist_set_write_fn(fn)` | `ble_state_row_persist_register_write_cb(handler, ctx)` | `ble_state_row_persist_write_fn` — no change |
| `tlv_processor_set_persist_fn(fn)` | `tlv_processor_register_persist_cb(handler, ctx)` | `tlv_processor_persist_fn` — no change |
| `tx_air_set_on_send_cb(cb)` | `tx_air_register_on_send_cb(handler, ctx)` | `tx_air_on_send_fn` — no change |
| `tx_air_set_on_window_done_cb(cb)` | `tx_air_register_on_window_done_cb(handler, ctx)` | `tx_air_on_window_done_fn` — no change |
| `transmission_set_reliable_admit_cb(cb)` | `transmission_register_reliable_admit_cb(handler, ctx)` | `transmission_reliable_admit_fn` — no change |
| `pending_set_reliable_outcome_cb(cb)` | `pending_register_reliable_outcome_cb(handler, ctx)` | `pending_reliable_outcome_fn` — no change |

---

## 4. Anti-Patterns

```c
/* WRONG — void return, no error reporting */
void module_set_x_cb(module_x_fn cb);

/* WRONG — no guard, caller can set mid-operation */
int module_register_x_cb(module_x_fn handler, void *user_ctx)
{
    s_cb = handler;
    return 0;
}

/* WRONG — _cb_t typedef suffix */
typedef void (*module_x_cb_t)(...);

/* WRONG — no NULL check before invocation */
s_cb(args, s_user_ctx);

/* WRONG — file-level @threadsafe comment instead of per-function tag */
// @threadsafe
void module_set_x_cb(...);  /* still missing tag on declaration line */
```

---

## 5. Caller Side (system_lifecycle.c)

All registrations happen after `module_init()` and before `module_start()`:

```c
/* Correct wiring sequence */
rc = module_init(&cfg);
if (rc) { ... }

rc = module_register_purpose_cb(my_handler, my_ctx);
if (rc) { ... }   /* must check — -EBUSY means you wired too late */

rc = module_start();
if (rc) { ... }
```

Always check the return value. A non-zero return means the registration was rejected
and the module will run without a handler — silent loss.
