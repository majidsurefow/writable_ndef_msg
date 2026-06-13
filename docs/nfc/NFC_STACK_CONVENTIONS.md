# NFC Emulation Stack — Conventions

**Status:** LOCKED — binding for every layer and service in `src/nfc/`.
**Last updated:** 2026-06-12. Architecture-of-record: `docs/superpowers/specs/2026-06-12-nfc-stack-architecture.md` (v3).

This is the single conventions reference every wave agent reads in Step 1. It
adapts the firmware-wide design docs to the NFC stack and pre-resolves the
NFC-specific decisions so no wave re-derives them.

---

## 1. Authority and Precedence

These firmware-wide docs are the governing authority. Where this file is silent,
they win. Where this file makes an NFC-specific choice, follow this file.

| Doc | Governs |
|-----|---------|
| `docs/API_DESIGN_CREED.md` | Module lifecycle, required structs/getters, thread-safety annotations, FSM, coupling patterns, workqueue/memory/PM/observability contracts |
| `docs/CALLBACK_REGISTRATION_GUIDE.md` | Exact callback typedef/setter/storage/guard pattern, `user_ctx` naming |
| `docs/STATS_API_DESIGN.md` | `s_stats` + `s_stats_lock` recipe, copy-out getter, hot-path helper, `STATS_*` macros |
| `docs/NETWORK_BUFFERS.md` | `net_buf` ownership — one owner, explicit transfer, FIXED pools |
| `docs/STACK_SPEC.md` | Layered-stack style; the four coupling patterns; "downward = direct call, upward = A/B/C/D" rule |
| `rules/coding-standards/misra-c-2012.md` + `zephyr-misra-deviations.md` | MISRA C:2012 + pre-approved DEV-ZEP deviations |

---

## 2. Per-Layer Module Contract

Every layer is a "module" per the creed. This table locks the per-layer choices.
The wave plan for each layer defines the concrete fields; this table fixes the
shape.

| Layer | Lifecycle | State storage | Behavioral FSM |
|-------|-----------|---------------|----------------|
| `hal/nfc_transport` | **Full** — init/start/stop/shutdown (restartable for UID rotation) | **Pattern B** — `atomic_t` (ISR + WQ read state) | none |
| `framing/apdu_assembler` | **Minimal** — init/shutdown | **Pattern A** — plain enum (WQ thread only) | none |
| `router/aid_router` | **Minimal** — init/shutdown | **Pattern A** — plain enum (WQ thread only) | none |
| `store/nfc_store` | **Minimal** — init/shutdown | **Pattern A** — plain enum (caller thread) | none |
| `nfc_stack` | **Full** — init/start/stop/shutdown | **Pattern A** lifecycle + `atomic_t` pending-profile | none |
| `services/ndef` | **Minimal** — init/shutdown | **Pattern A** (WQ thread) | none |
| `services/desfire` | **Minimal** — init/shutdown | **Pattern A** lifecycle + per-session auth FSM | **SMF** (auth: IDLE→STEP1→STEP2→AUTHENTICATED) |
| `services/ultralight` | **Minimal** — init/shutdown | **Pattern A** (WQ thread) | none |
| `services/emv` | **Minimal** — init/shutdown | **Pattern A** (WQ thread) | none |
| `services/aliro` | **Minimal** — init/shutdown | **Pattern A** lifecycle + per-session FSM | **SMF** (IDLE→AWAIT_AUTH0→AWAIT_AUTH1→AWAIT_EXCHANGE→DONE/ERROR) |

**Lifecycle naming is fixed by the creed:** `init` / `start` / `stop` /
`shutdown`. There is **no `deinit`**. Minimal-lifecycle modules expose only
`init` / `shutdown` and a state enum of `{UNINITIALIZED, INITIALIZED}`.

**Every module — no exceptions — provides:**

```c
typedef struct { /* ... */ } <module>_config_t;   /* frozen after init() */
typedef struct {                                  /* see §5 for required fields */
    uint32_t error_count;
    int32_t  last_error_code;
    /* module-specific counters */
} <module>_stats_t;
typedef enum { <MODULE>_STATE_UNINITIALIZED, <MODULE>_STATE_INITIALIZED,
               /* + STARTED/STOPPED/ERROR for full lifecycle */ } <module>_state_t;

const <module>_config_t *<module>_get_config(void);   /* never NULL */
int                      <module>_get_stats(<module>_stats_t *out); /* copy-out */
<module>_state_t         <module>_get_state(void);     /* never fails */
```

Getters are backed by file-static structs and are safe to call before `init()`.

---

## 3. Coupling Map

Direction rule (STACK_SPEC): **downward data flow = direct function calls;
upward data flow (events) = a registered callback pattern.** Bottom of the stack
is the HAL (closest to hardware); top is the services.

| Boundary | Direction | Pattern | Mechanism |
|----------|-----------|---------|-----------|
| nrfxlib → HAL | up | Zephyr/nrfxlib API | `nfc_t4t_lib` callback (ISR) |
| HAL → framing (field events + complete APDU) | up | **A** ops struct | `nfc_transport_register_callbacks(ops, user_ctx)` |
| framing → router (parsed APDU) | up | **A** direct call to fixed singleton | `aid_router_dispatch(&apdu)` — `const nfc_apdu_t *` (locked by Wave 3) |
| router → service (SELECT-matched dispatch) | up | **A** service vtable | `aid_router_register(aid, len, svc)` |
| service → HAL (response) | down | direct call | `nfc_transport_send_response(buf, len)` |
| `nfc_stack` → store (save/load active card) | down | direct call | `nfc_store_save/load(tag, svcs, n)` — both return `-EBUSY` while the stack is STARTED (no save/load during a live session); load (provisioning) is the primary path, save is a debug/capture facility |
| store → service (serialize/deserialize) | down | direct call via vtable | `svc->serialize` / `svc->deserialize` |
| store → save/load backend | out | **register-cb** (canonical §4) | `nfc_store_register_save_cb/load_cb` — default stubs (shell / `.h`) |
| `nfc_stack` → hal/framing/router/store | down | direct call | orchestration |

**Wiring rule (STACK_SPEC rule 2):** all cross-layer callback registration
happens in **`nfc_stack.c`** — the lifecycle orchestrator. No layer `#include`s
another layer's header solely to wire callbacks. Register **after** the emitting
module's `init()` and **before** its `start()`.

---

## 4. Callback Pattern

Both callback boundaries (HAL ops struct, service vtable) are **Pattern A**. They
obey the CALLBACK_REGISTRATION_GUIDE rules:

- Callback typedef suffix is **`_fn`**; variables/functions use **`_cb`**.
- The caller-context pointer is **`user_ctx`** — never `ctx`, `user_data`,
  `priv`, or `arg`.
- Registration setters return `int` (0 / negative errno), guard on module state
  (`-ENODEV` if UNINITIALIZED, `-EBUSY` if STARTED), and accept `NULL` to clear.
- The invoker NULL-checks every function pointer before calling.
- The dispatch thread is documented in the header for every callback.

**HAL ops struct** (`hal/nfc_transport.h`):

```c
typedef struct {
    void (*on_field_on)(void *user_ctx);
    void (*on_field_off)(void *user_ctx);
    void (*on_apdu)(struct net_buf *apdu, void *user_ctx); /* complete APDU; see §5. Callee owns the ref. */
} nfc_transport_ops_t;

/* @caller_sync — register after init(), before start() */
int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops,
                                     void *user_ctx);
```

**Service vtable** (`router/service.h`):

```c
typedef struct {
    void (*on_select)(const uint8_t *aid, size_t aid_len, void *user_ctx);
    void (*on_apdu)(const nfc_apdu_t *apdu, void *user_ctx); /* parsed APDU; see spec apdu_types.h */
    void (*on_deselect)(void *user_ctx);
    void (*on_field_off)(void *user_ctx);
    void *user_ctx;
} nfc_service_t;
```

This is the base callback shape. The full locked form (Wave 3 `service.h`) adds the
persistence fields `serialize` / `deserialize` (nullable) and `persist_id` on top —
see `docs/superpowers/plans/wave3-router.md` §1.

`on_apdu` calls `nfc_transport_send_response()` imperatively (synchronous
services) or defers crypto to `nfc_work_q` and sends from the work handler
(Aliro). Both are legal — the response buffer is static and service-owned.

---

## 5. Buffer Model

**Decision: `net_buf` inbound, static buffer outbound.** `net_buf` is used only
where data crosses the ISR→work-queue boundary; the single-owner response path
uses a static service buffer.

### Inbound — fragment → APDU (the one cross-thread transport)

- A single **FIXED** pool: `NET_BUF_POOL_FIXED_DEFINE(nfc_apdu_pool, …)`.
  FIXED is non-blocking and ISR-safe (NETWORK_BUFFERS §Pool Types).
- The HAL ISR callback allocates from the pool with `K_NO_WAIT` on the **first**
  fragment of a chain, appends each `DATA_IND` fragment with `net_buf_add_mem`
  (the mandatory immediate copy — nrfxlib buffer lifetime is undocumented), and
  on the **final** fragment (`MORE` clear) enqueues the complete-APDU `net_buf`
  to a `k_fifo`. The WQ thread drains the fifo.
- **One owner at all times** (NETWORK_BUFFERS): the ISR owns the buffer until it
  is put on the fifo; the WQ owns it after `k_fifo_get`; the WQ unrefs it after
  the router returns. Ownership transfer is explicit; never touch the pointer
  after transfer.
- Pool exhaustion is **normal**: increment a `dropped` stat and return — never
  `__ASSERT`.
- **Sizing:** `CONFIG_NFC_APDU_BUF_SIZE` (default 512) data bytes per buffer,
  `CONFIG_NFC_APDU_POOL_COUNT` (default 4) buffers. An APDU whose assembled
  length would exceed one buffer's tailroom is **oversized** → respond `6700`
  (Wrong Length), drop, and bump `apdu_oversized_count`. This replaces the
  earlier 32767 `NFC_APDU_MAX_LEN` ceiling with the practical pool bound.

### Outbound — response

- Each service owns a **static** response buffer (BSS, single instance — legal
  because there is exactly one response in flight and the HAL borrows it until
  the next callback). This is *not* a `static` local inside a function (forbidden
  by creed §9) — it is a file-static module buffer.
- `nfc_transport_send_response(buf, len)` hands it to
  `nfc_t4t_response_pdu_send()`, which borrows it until the next callback. The
  service must not overwrite it until then.

`net_buf` is **not** used for the response, nor for the in-thread
framing→router→service dispatch (borrowed pointer passes on one thread).

---

## 6. Stats

Every layer follows the STATS_API_DESIGN recipe exactly:

```c
static <module>_stats_t   s_stats;        /* always this name */
static struct k_spinlock  s_stats_lock;   /* NOT K_SPINLOCK_DEFINE — unavailable in NCS v3.2.4 */
```

- `error_count` (`uint32_t`) and `last_error_code` (`int32_t`) in **every** stats
  struct. Every non-fatal failure path: `STATS_ERROR(&s_stats_lock, s_stats, code)`.
- Every silent-drop path has a named counter (e.g. `frag_dropped_no_buffer`,
  `apdu_oversized_count`).
- `STATS_RESET(s_stats)` as the first statement of the init body **after the
  `-EALREADY` lifecycle guard** (a rejected double-`init()` must not wipe live
  stats); under `s_stats_lock` if a runtime `reset_stats()` exists.
- Getter is copy-out: `int <module>_get_stats(<module>_stats_t *out)` via
  `STATS_COPY_OUT(&s_stats_lock, s_stats, out)`. Never return a pointer to
  `s_stats`.
- **Hot path** (HAL fragment RX) routes through a private
  `nfc_transport_stats_on_fragment(...)` helper wrapping one `STATS_SCOPE` —
  compute elapsed time *before* the scope, never inside the lock.

---

## 7. Threading

| Context | Runs | Rules |
|---------|------|-------|
| nrfxlib ISR callback | HAL `on_field_*` / `on_fragment` raw delivery | No alloc except FIXED `net_buf` `K_NO_WAIT`; no sleep; no blocking; copy fragment immediately |
| `nfc_work_q` (dedicated, high priority) | fifo drain → framing assemble-finalize → router dispatch → service → response | No sleep; no dynamic alloc beyond the inbound pool; bounded execution; document worst case |
| Caller thread | `nfc_stack_*()` lifecycle, `nfc_stack_set_profile()`, `nfc_stack_set_uid()` | `@caller_sync` |

- `nfc_work_q` is a **named** work queue owned by the HAL — never the system work
  queue (creed §8). Document its priority and stack size in the HAL README.
- Never call a scheduler API (`k_work_submit_to_queue`, `k_fifo_put` wakeups,
  `k_sem_give`) while holding a `k_spinlock` (creed §8).
- Teardown uses `k_work_cancel_*_sync` / fifo drain from thread context.
- **Every public function carries a thread-safety annotation** in its header:
  `@threadsafe`, `@isr_safe`, or `@caller_sync`. An `@isr_safe` function reads
  only `atomic_t` or spinlock-protected state (creed §3).

---

## 8. Profile Switching

- `nfc_stack_set_profile(profile)` sets an `atomic_t` pending-profile and returns
  immediately (`@threadsafe`). It does **not** tear down an active session.
- The pending profile is applied on the **next field-off**, on `nfc_work_q`:
  `aid_router_clear()` then re-register the new profile's services.
- In-memory only — no persistence; resets to the default profile on reboot.
- Only profiles whose Kconfig is enabled are valid; an unavailable profile
  returns `-ENOTSUP`.

---

## 9. Error Codes

Standard errno only (creed §4) — no invented codes:

| Code | Use |
|------|-----|
| `-EINVAL` | NULL pointer, bad argument |
| `-EALREADY` | `init()` when already INITIALIZED |
| `-EBUSY` | register/config mutation while STARTED |
| `-ENODEV` | operation before `init()` / device not ready |
| `-ENOTSUP` | profile/service not compiled in |
| `-ENOMEM` | pool exhaustion at a non-hot path |
| `-EIO` | hardware / nrfxlib failure |

ISO 7816 status words (`0x9000`, `0x6A82`, `0x6700`, …) are protocol-level
responses sent to the reader — distinct from C return codes. Never conflate them.

---

## 10. Shell

Every module with runtime state registers a shell subcommand exposing `config`,
`stats`, and `state`, in a dedicated `<module>_shell_cmds.c` (creed §11) —
never inside the module's core `.c`.

```c
SHELL_STATIC_SUBCMD_SET_CREATE(nfc_transport_cmds,
    SHELL_CMD(config, NULL, "Print config", cmd_nfc_transport_config),
    SHELL_CMD(stats,  NULL, "Print stats",  cmd_nfc_transport_stats),
    SHELL_CMD(state,  NULL, "Print state",  cmd_nfc_transport_state),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(nfc_transport, &nfc_transport_cmds, "NFC HAL control", NULL);
```

`shell_print` variadic usage is the pre-approved deviation DEV-ZEP-009.

---

## 11. MISRA

All code targets MISRA C:2012 (`rules/coding-standards/misra-c-2012.md`). Each
wave plan's Implementation Notes section must call out the rule-relevant
decisions for that layer and cite the applicable DEV-ZEP deviation where a
Zephyr/nrfxlib API forces one (e.g. `CONTAINER_OF` for `k_fifo`/`net_buf`
recovery → DEV-ZEP-001/005; `K_MSEC`/`K_NO_WAIT` → DEV-ZEP-013;
`LOG_*`/`shell_print` → DEV-ZEP-008/009; `BIT`/`GENMASK` → DEV-ZEP-017/018).

---

## 12. Per-Wave Compliance Checklist

Every wave plan must show, before its Tasks section, that the layer satisfies:

- [ ] Lifecycle matches §2 (full vs minimal; `shutdown` not `deinit`)
- [ ] `config_t` / `stats_t` / `state_t` + three getters defined (§2)
- [ ] State storage Pattern A/B per §2
- [ ] Coupling matches §3; callbacks follow §4 (`_fn`/`_cb`, `user_ctx`, guard, NULL-check)
- [ ] Buffer model per §5 (net_buf inbound / static outbound where applicable)
- [ ] Stats recipe per §6 (`s_stats` + `s_stats_lock`, copy-out getter, hot-path helper)
- [ ] Threading + annotations per §7
- [ ] Error codes per §9; shell per §10
- [ ] MISRA notes + DEV-ZEP citations per §11
