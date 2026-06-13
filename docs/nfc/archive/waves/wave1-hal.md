# Wave 1: HAL (nfc_transport) Implementation Plan

**Status:** LOCKED — 2026-06-12 (harmonized v2)

> **Architecture context (v3 reframe):** This plan is the NFCT/card first-slice of
> the capability-driven, dual-role, multi-backend architecture described in
> [`docs/nfc/archive/specs/2026-06-12-nfc-stack-architecture.md`](../specs/2026-06-12-nfc-stack-architecture.md)
> (v3, §9 row "Wave 1 HAL"). The existing listen contract, NFCT function signatures,
> and ISR/WQ implementation details below are **unchanged and correct**. This reframe
> adds the capability model (§1.8), role Kconfig symbols (§1.7), event-model note
> (§6), and reserved poller seam (§A) on top — none of which alter the card-mode
> NFCT implementation.

**Layer:** hal/nfc_transport
**Files produced:**
- `src/nfc/hal/nfc_transport.h` (vendor-clean public interface — no nrfxlib includes, types, or constants; spec v2 §6.1)
- `src/nfc/hal/nfc_transport_nrfx.c` (nRFX backend — wraps nrfxlib `nfc_t4t_lib`; selected by `NFC_HAL_BACKEND_NRFX`)
- `src/nfc/hal/nfc_transport_shell_cmds.c`
- `src/nfc/hal/Kconfig`
- `src/nfc/hal/CMakeLists.txt`
- `tests/unit/nfc_transport/` (ztest for the portable/pure helpers — see §8; Twister tag `sigmation.nfc.transport`)

**Depends on:** none (bottom of the stack; the nRFX backend wraps nrfxlib `nfc_t4t_lib`).

**Consumed by (locked here for Wave 2+):** framing (`apdu_assembler`) via the ops struct, services via `nfc_transport_send_response()`, `nfc_stack` via the lifecycle + `set_uid`.

**Sources consulted (Step 1 context sweep):**
- `docs/NFC_STACK_CONVENTIONS.md` (binding — read fully)
- `docs/NFC_WAVE_PLANNING_GUIDE.md` (process + template)
- `docs/nfc/archive/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) §1a, §4, §5, §6.1, §6.2, §8, §9, §13 (architecture/threading/buffers/backend choice/non-goals)
- `docs/API_DESIGN_CREED.md` (lifecycle, Pattern B atomic state, Pattern A ops, workqueue/memory/observability)
- `docs/CALLBACK_REGISTRATION_GUIDE.md` (register/guard/NULL-clear/`user_ctx` rules)
- `docs/STATS_API_DESIGN.md` + `src/stats.h` (`s_stats`/`s_stats_lock`, `STATS_*`, copy-out getter, hot-path helper)
- `docs/NETWORK_BUFFERS.md` (FIXED pool, one-owner, ISR `K_NO_WAIT`, unref discipline)
- `docs/STACK_SPEC.md` (downward=direct call, upward=A/B/C/D; register after init/before start in `nfc_stack.c`)
- nrfxlib: `/opt/nordic/ncs/v3.2.4/nrfxlib/nfc/include/nfc_t4t_lib.h`, `nfc_platform.h`, `nrf_nfc_errno.h`
- `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` (DEV-ZEP records)
- Zephyr v3.2.4 kernel headers (`k_work_q`, `k_fifo`, `net_buf`, `k_spinlock`, `atomic`)

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (prototype being replaced — patterns must not carry forward).

---

## 1. Types and Constants

All public types live in `nfc_transport.h`. Vocabulary is locked here; Wave 2 builds on it verbatim.

**Vendor-clean header rule (spec v2 §6.1):** `nfc_transport.h` contains no nrfxlib (or other vendor) includes, types, or constants. Every limit the contract needs is a transport-owned `#define`; backend files (`nfc_transport_nrfx.c`) map them to vendor values and `BUILD_ASSERT` the equivalence.

```c
/* Transport-owned response length limit. The nRFX backend BUILD_ASSERTs that
 * this equals NFC_T4T_MAX_PAYLOAD_SIZE; future backends assert their own
 * equivalent. Public code uses only this constant. */
#define NFC_TRANSPORT_MAX_RESPONSE_LEN  0xFFF0U
```

### 1.1 UID

The HAL is the lowest layer that touches `NFC_T4T_PARAM_NFCID1`, so it **owns** the UID type. `nfc_stack` includes this header for `nfc_stack_start(uid)` / `nfc_stack_set_uid(uid)`.

```c
/* NFCID1, MSB first. Valid lengths: 4 (single), 7 (double), 10 (triple). */
typedef struct {
    uint8_t bytes[10];
    uint8_t len;        /* 4, 7, or 10 */
} nfc_uid_t;

#define NFC_UID_LEN_SINGLE   4U
#define NFC_UID_LEN_DOUBLE   7U
#define NFC_UID_LEN_TRIPLE  10U
```

### 1.2 Config (frozen after `init()`)

```c
typedef struct {
    uint8_t fwi;   /* ISO-DEP Frame Wait Integer (ISO 14443-4) applied by the
                    * backend before emulation starts. 0..8. Default 8
                    * => ~5 s reader wait window, leaving headroom for Aliro
                    * crypto on WQ. (The nRFX backend maps this to
                    * NFC_T4T_PARAM_FWI — backend detail, see §6.) */
} nfc_transport_config_t;

#define NFC_TRANSPORT_DEFAULT_FWI  8U   /* ISO-DEP maximum (~5 s) */
```

### 1.3 Stats

```c
typedef struct {
    uint32_t field_on_count;          /* NFC_T4T_EVENT_FIELD_ON seen (ISR)        */
    uint32_t field_off_count;         /* NFC_T4T_EVENT_FIELD_OFF seen (ISR)       */
    uint32_t fragment_rx_count;       /* DATA_IND fragments received (ISR)        */
    uint32_t apdu_assembled_count;    /* complete C-APDUs enqueued to nfc_work_q  */
    uint32_t response_tx_count;       /* nfc_t4t_response_pdu_send() OK           */
    uint32_t uid_rotation_count;      /* set_uid() live rotations OK              */
    uint32_t frag_dropped_no_buffer;  /* pool empty on first fragment (drop)      */
    uint32_t apdu_oversized_count;    /* assembled len > buffer tailroom -> 6700  */
    uint32_t apdu_dropped_no_consumer;/* APDU ready but on_apdu unregistered      */
    uint32_t error_count;             /* mandatory (STATS_ERROR)                  */
    int32_t  last_error_code;         /* mandatory (STATS_ERROR)                  */
} nfc_transport_stats_t;
```

> **DECISION (stats):** The spec v1 §6.1 stats struct listed only `field_on/off`, `fragment_rx`, `response_tx`, `uid_rotation`, `error_count`, `last_error_code`. CONVENTIONS §6 *requires* a named counter for **every** drop path, so the struct was extended with `apdu_assembled_count`, `frag_dropped_no_buffer`, `apdu_oversized_count`, `apdu_dropped_no_consumer`. Spec v2 §6.1 defers the exact field list to this plan (§1.3). No `latency_stats_t` field is added — see §6 "Hot-path helper" DECISION.

### 1.4 State (Pattern B — `atomic_t` backing, see §4)

```c
typedef enum {
    NFC_TRANSPORT_STATE_UNINITIALIZED = 0,  /* zero-init default */
    NFC_TRANSPORT_STATE_INITIALIZED,        /* spec "IDLE" after init           */
    NFC_TRANSPORT_STATE_STARTED,            /* spec "RUNNING" — emulation active */
    NFC_TRANSPORT_STATE_STOPPED,            /* spec "IDLE" after stop (restartable) */
    NFC_TRANSPORT_STATE_ERROR,              /* terminal until shutdown()         */
} nfc_transport_state_t;
```

### 1.5 Upward ops struct (Pattern A) — **LOCKED for Wave 2**

```c
/* Dispatch thread for ALL three callbacks: nfc_work_q (NOT the ISR, NOT the
 * system WQ). The HAL assembles fragments in the t4t callback context and
 * re-dispatches complete events on nfc_work_q. */
typedef struct {
    void (*on_field_on)(void *user_ctx);
    void (*on_field_off)(void *user_ctx);
    void (*on_apdu)(struct net_buf *apdu, void *user_ctx); /* callee OWNS the ref */
} nfc_transport_ops_t;
```

> **DECISION (ops shape):** CONVENTIONS §4 and the original spec v1 §6.1 disagreed — v1 embedded a `void *user_ctx` member inside `nfc_transport_ops_t`; the conventions form has **no** member and passes `user_ctx` separately to `nfc_transport_register_callbacks(ops, user_ctx)`. The conventions doc is binding and governs the *how*, so the **conventions form is locked**: ops struct carries function pointers only; `user_ctx` is a separate registration argument stored alongside. Spec v2 §6.1 now states this form. Wave 2 must use this form.
>
> **Callee owns the ref (`on_apdu`):** matches CONVENTIONS §4 comment and §5. The framing layer (Wave 2) must `net_buf_unref()` the APDU after `aid_router_dispatch()` returns. The HAL does not unref after invoking `on_apdu`.
>
> No `_fn` typedefs are introduced for the ops members — Pattern A canonically uses inline function pointers (creed §7 Pattern A example). The `_fn` typedef rule applies to standalone register-callback typedefs, not vtable members.

### 1.6 Internal-only constants (in `nfc_transport_nrfx.c`, not exported)

```c
/* HAL must not include framing's apdu_types.h (framing is ABOVE the HAL).
 * The one status word the HAL emits itself is duplicated locally on purpose. */
static const uint8_t s_sw_wrong_length[2] = { 0x67U, 0x00U }; /* ISO 7816 6700 */
```

> **DECISION (no upward include):** The HAL is the bottom layer and must not depend on `framing/apdu_types.h` (would invert the dependency). The `6700` bytes the HAL sends for oversized APDUs are hardcoded locally. This 2-byte duplication is intentional and layering-correct.

### 1.7 Kconfig symbols

Introduced by this layer (`src/nfc/hal/Kconfig`):

| Symbol | Type | Default | Purpose |
|---|---|---|---|
| `NFC_HAL_WORKQ_STACK_SIZE` | int | `2048` | `nfc_work_q` thread stack (bytes). Sized for Aliro crypto on-WQ (~ECDH+HKDF) + service response build. Finalize from Thread Analyzer peak (creed §11). |
| `NFC_HAL_WORKQ_PRIORITY` | int | `5` | `nfc_work_q` cooperative/preemptive priority. High priority, dedicated queue (never system WQ). See §6 sizing rationale. |

**Backend choice (spec v2 §6.1/§9):** `src/nfc/hal/Kconfig` also defines the transport backend choice. The vendor dependency lives on the backend symbol, not on `NFC_STACK` — the stack itself is backend-neutral:

```kconfig
choice NFC_HAL_BACKEND
    prompt "NFC HAL transport backend"
    default NFC_HAL_BACKEND_NRFX
    depends on NFC_STACK

config NFC_HAL_BACKEND_NRFX
    bool "nRF NFCT via nrfxlib nfc_t4t_lib"
    depends on NFC_T4T_NRFXLIB

config NFC_HAL_BACKEND_RFAL
    bool "ST25R3916 via ST RFAL (reserved — backend file not yet present)"
    # Planned SECOND backend (spec v3 §6). Driver already in-repo at
    # aliro/platform/nfc/nfc_transport_rfal.*. Selecting this entry here
    # reserves the symbol name and enables the reader-capable hidden guard.
    # Do NOT select in production until nfc_transport_rfal.c is implemented.
    select NFC_HAL_BACKEND_HAS_READER

config NFC_HAL_BACKEND_PN7160
    bool "NXP PN7160 (future — spec v3 §6)"
    select NFC_HAL_BACKEND_HAS_READER

endchoice
```

`CMakeLists.txt` selects the backend source: `nfc_transport_nrfx.c` when `CONFIG_NFC_HAL_BACKEND_NRFX`.

Consumed here, **defined at the stack level** per spec v2 §9 (depend on `NFC_STACK`): `NFC_APDU_BUF_SIZE` (512), `NFC_APDU_POOL_COUNT` (4). The `nfc_apdu_pool` lives in the HAL but its sizing Kconfig is the shared stack contract.

**Role symbols (spec v3 §5 — defined at the stack level, consumed by the HAL):**

| Symbol | Type | Default | Depends on | Purpose |
|---|---|---|---|---|
| `NFC_ROLE_CARD` | bool | `y` | `NFC_STACK` | Build the listen/emulate role. Always on for NFCT. |
| `NFC_ROLE_READER` | bool | `n` | `NFC_STACK && NFC_HAL_BACKEND_HAS_READER` | Build the poll/copy role. Requires a reader-capable backend. |

`NFC_HAL_BACKEND_HAS_READER` is a hidden bool `select`-ed by any backend that
supports the reader role (ST25R3916/RFAL backend: `y`; NFCT backend: never set).

**BUILD_ASSERT — capability guard (spec v3 §2, §5):**

Each backend `BUILD_ASSERT`s that the Kconfig-selected roles are a subset of its
advertised capabilities. For the NFCT backend (in `nfc_transport_nrfx.c`):

```c
/* Selecting CONFIG_NFC_ROLE_READER on the NFCT backend is a compile-time error —
 * the NFCT controller cannot poll. Use NFC_HAL_BACKEND_RFAL or NFC_HAL_BACKEND_PN7160. */
BUILD_ASSERT(!IS_ENABLED(CONFIG_NFC_ROLE_READER),
             "CONFIG_NFC_ROLE_READER selected but NFC_HAL_BACKEND_NRFX does not "
             "support the reader role. Choose a reader-capable backend "
             "(NFC_HAL_BACKEND_RFAL or NFC_HAL_BACKEND_PN7160) or disable "
             "CONFIG_NFC_ROLE_READER.");
```

A reader-capable backend provides `select NFC_HAL_BACKEND_HAS_READER` in its
Kconfig entry and its own BUILD_ASSERT verifying the specific technologies.

> **DECISION (workq priority/stack numbers):** `5` and `2048` are first-cut values (between `tx_pipeline_wq`=4 and `rx_pipeline_wq`=6 from creed §8). NFC and BLE-TX are independent subsystems; FWI=8 (~5 s) gives ample latency headroom so NFC need not preempt TX. Final numbers to be pinned from Thread Analyzer before production — flagged for human review.

### 1.8 Capability descriptor (`nfc_transport.h` — backend-provided, spec v3 §4.1)

The capability model locks which roles and technologies each backend can serve.
Nothing above the HAL hard-codes a technology or role set. Declared in
`nfc_transport.h` (vendor-clean; no backend-specific types).

```c
/* Role bitmask. Each backend sets exactly the roles its controller supports. */
typedef uint8_t nfc_role_t;
#define NFC_ROLE_CARD    BIT(0)   /* listen / card emulation */
#define NFC_ROLE_READER  BIT(1)   /* poll / transceive */

/* Technology bitmask. Forward-looking enumeration; a backend sets only what
 * its controller can physically do. */
typedef uint32_t nfc_tech_t;
#define NFC_TECH_ISO_DEP_A       BIT(0)  /* ISO 14443-4A (ISO-DEP / T4T)          */
#define NFC_TECH_ISO14443_3A_RAW BIT(1)  /* ISO 14443-3A raw (Ultralight, MIFARE) */
#define NFC_TECH_ISO14443_3B_RAW BIT(2)  /* ISO 14443-3B raw                      */
#define NFC_TECH_TYPE2           BIT(3)  /* NFC Forum Type 2 Tag                  */
#define NFC_TECH_TYPE3_FELICA    BIT(4)  /* NFC Forum Type 3 / FeliCa             */
#define NFC_TECH_ISO15693        BIT(5)  /* ISO 15693 (NFC-V)                     */
#define NFC_TECH_ISO_DEP_B       BIT(6)  /* ISO 14443-4B (ISO-DEP over Type-B)    */
/* BIT(7)..BIT(31): reserved for future technologies. */

/* Capability descriptor — statically allocated and owned by the backend. */
typedef struct {
    uint8_t  roles;        /* OR of NFC_ROLE_* bits */
    uint32_t technologies; /* OR of NFC_TECH_* bits */
} nfc_transport_caps_t;

/* @isr_safe — returns a pointer to the backend's static descriptor; never NULL.
 * Callers must not modify the returned struct. Declared here; implemented per-backend. */
const nfc_transport_caps_t *nfc_transport_get_capabilities(void);
```

**NFCT backend declaration** (in `nfc_transport_nrfx.c`):

```c
/* NFCT is a card-only Type-4A controller. It cannot poll. */
static const nfc_transport_caps_t s_nrfx_caps = {
    .roles        = NFC_ROLE_CARD,
    .technologies = NFC_TECH_ISO_DEP_A,
};

const nfc_transport_caps_t *nfc_transport_get_capabilities(void)
{
    return &s_nrfx_caps;
}
```

> **DECISION (static capability descriptor):** The descriptor is a `static const`
> struct, not a runtime controller query. The backend choice is already a
> compile-time Kconfig decision; the BUILD_ASSERT in §1.7 enforces consistency
> statically. Dynamic discovery (e.g. PN7160 firmware-variable tech) can extend
> this contract later without breaking callers.

---

## 2. Public API

> **Vendor-clean shared contract (spec v3 §4.1):** `nfc_transport.h` is the
> portable interface shared by all backends — NFCT now; ST25R3916/RFAL next;
> PN7160 third. It contains no vendor includes, types, or constants. Backend files
> (`nfc_transport_nrfx.c`, future `nfc_transport_rfal.c`) implement every function
> in this header. `nfc_transport_submit_work` and the half-duplex contract are part
> of this shared interface. The NFCT backend does not implement the reserved poller
> sub-API (§A); those stubs return `-ENOTSUP`.

All declarations in `nfc_transport.h`. Memory comment (creed §9) at top of header:

```c
/*
 * Memory: nfc_apdu_pool  = CONFIG_NFC_APDU_POOL_COUNT x CONFIG_NFC_APDU_BUF_SIZE
 *                          (default 4 x 512 = 2048 B, FIXED, static)
 *         nfc_work_q stk = CONFIG_NFC_HAL_WORKQ_STACK_SIZE (default 2048 B)
 *         file-static state/stats/config/ops (~80 B)
 * lifecycle: init / register_callbacks / start / stop / shutdown (full, restartable)
 */
```

```c
/* @caller_sync — UNINITIALIZED -> INITIALIZED. cfg NULL => built-in defaults. */
int nfc_transport_init(const nfc_transport_config_t *cfg);

/* @caller_sync — register AFTER init(), BEFORE start(). NULL ops clears.
 * Dispatch thread for all ops = nfc_work_q. */
int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops, void *user_ctx);

/* @caller_sync — INITIALIZED|STOPPED -> STARTED. Sets NFCID1 from uid, starts
 * nfc_work_q, then nfc_t4t_emulation_start(). uid NULL => library default NFCID1. */
int nfc_transport_start(const nfc_uid_t *uid);

/* @caller_sync — STARTED -> STOPPED. Stops emulation, drains WQ/fifo, stops WQ.
 * Restartable via start(). */
int nfc_transport_stop(void);

/* @caller_sync — any state -> UNINITIALIZED (implicit stop if STARTED).
 * Calls nfc_t4t_done(). Idempotent. Replaces "deinit". */
int nfc_transport_shutdown(void);

/* @threadsafe — live UID rotation while STARTED: backend emulation stop ->
 * set NFCID1 -> backend emulation restart. Does NOT tear down the work queue.
 * Serialized internally. */
int nfc_transport_set_uid(const nfc_uid_t *uid);

/* @threadsafe — hand a service-owned response PDU to the transport.
 * buf is BORROWED by the transport until the next transport event; caller must
 * keep it valid and must not overwrite it until then. The transport does not
 * copy. len must be <= NFC_TRANSPORT_MAX_RESPONSE_LEN. */
int nfc_transport_send_response(const uint8_t *buf, size_t len);

/* @threadsafe — submit a caller-owned k_work item to nfc_work_q. For services
 * that defer response generation (e.g. Aliro crypto): the work handler runs on
 * nfc_work_q after the current item and calls nfc_transport_send_response().
 * -ENODEV unless STARTED. */
int nfc_transport_submit_work(struct k_work *work);

/* @isr_safe — never NULL (file-static backing). */
const nfc_transport_config_t *nfc_transport_get_config(void);

/* @threadsafe — copy-out under s_stats_lock; -EINVAL if out is NULL. */
int nfc_transport_get_stats(nfc_transport_stats_t *out);

/* @isr_safe — atomic read; never fails. */
nfc_transport_state_t nfc_transport_get_state(void);

/* @isr_safe — returns backend's static capability descriptor; never NULL.
 * See §1.8 for nfc_transport_caps_t / nfc_role_t / nfc_tech_t definitions. */
const nfc_transport_caps_t *nfc_transport_get_capabilities(void);
```

---

## 3. Contracts

### `nfc_transport_init(cfg)`
- **Pre:** state == UNINITIALIZED. Callable concurrently-safe only as `@caller_sync`.
- **Post (0):** `-EALREADY` CAS guard evaluated first; `STATS_RESET(s_stats)` is the **first statement after the guard** (a rejected double-`init()` must not wipe live stats — CONVENTIONS §6); `s_config` populated (from `cfg`, or defaults if NULL); `nfc_apdu_pool` ready (link-time); `k_fifo`/`k_work` objects initialized; backend event callback registered and FWI staged (nRFX backend: `nfc_t4t_setup(cb, NULL)`, raw PICC mode locked at next emulation start — §6); state == INITIALIZED. **No thread started, no emulation, no work submitted** (creed §1).
- **Errors:**
  - `-EALREADY` — state already INITIALIZED/STARTED/STOPPED (CAS from UNINITIALIZED failed; stats untouched).
  - `-EIO` — backend setup or FWI staging returned nonzero (vendor error mapped — §5.1); state rolled back to UNINITIALIZED; `STATS_ERROR`.

### `nfc_transport_register_callbacks(ops, user_ctx)`
- **Pre:** state ∈ {INITIALIZED, STOPPED} (registration only when not running).
- **Post (0):** `s_ops = ops` (pointer stored; must remain valid for module lifetime per Pattern A), `s_user_ctx = user_ctx`. `ops == NULL` clears both (unregister).
- **Errors:** `-ENODEV` (UNINITIALIZED) · `-EBUSY` (STARTED) · `-EIO` (ERROR). (CALLBACK_REGISTRATION_GUIDE guard table.)

### `nfc_transport_start(uid)`
- **Pre:** state ∈ {INITIALIZED, STOPPED}.
- **Post (0):** if `uid != NULL`, NFCID1 set via `parameter_set(NFC_T4T_PARAM_NFCID1, uid->bytes, uid->len)` and stored in `s_active_uid`; `nfc_work_q` thread started (named "nfc_work_q"); `nfc_t4t_emulation_start()` OK; state == STARTED; ISR callbacks now flow.
- **Idempotent:** state == STARTED → return 0, no-op (creed §1).
- **Errors:**
  - `-ENODEV` — state == UNINITIALIZED.
  - `-EINVAL` — `uid != NULL` and `uid->len ∉ {4,7,10}`.
  - `-EIO` — `parameter_set` or `emulation_start` failed; WQ stopped, state reverted to prior; `STATS_ERROR`.
  - `-EIO` — state == ERROR.

### `nfc_transport_stop(void)`
- **Pre:** any state (idempotent no-op unless STARTED).
- **Post (0):** `s_uid_mutex` acquired **before** emulation stop and released only after the WQ teardown steps that must not interleave with `set_uid` — this serializes stop against a concurrent UID rotation so a mid-callback rotation cannot re-enable emulation after stop. Under the mutex: backend emulation stop (callbacks cease); in-flight assembly buffer discarded (irq-locked); `s_field_on/off_work` + `s_apdu_work` cancelled via `k_work_cancel_sync`; `nfc_apdu_pool` fifo drained (remaining bufs unref'd); `nfc_work_q` drained then stopped; state == STOPPED; mutex released. `start()` may be called again. A concurrent `set_uid` either completes fully before stop proceeds, or observes the stopped state and fails cleanly with `-ENODEV`.
- **Idempotent:** state ∈ {UNINITIALIZED, INITIALIZED, STOPPED} → return 0.
- **Errors:** `-EIO` (ERROR, or backend emulation-stop error + `STATS_ERROR`). All non-STARTED states return 0 (idempotent); call `shutdown()` to recover from ERROR.

### `nfc_transport_shutdown(void)`
- **Pre:** any state.
- **Post (0):** if STARTED, implicit `stop()` first; `nfc_t4t_done()` (callback ref released — re-`init()` required to reuse); state == UNINITIALIZED.
- **Idempotent:** state == UNINITIALIZED → return 0 (safe no-op).
- **Errors:** none returned for normal paths; lib error from `nfc_t4t_done()` → `STATS_ERROR` but still lands UNINITIALIZED and returns 0 (shutdown must always succeed; creed §1).

### `nfc_transport_set_uid(uid)`
- **Pre:** state == STARTED. Thread context only (uses backend emulation toggles; never called from ISR). Internally serialized by `s_uid_mutex`.
- **Post (0):** under `s_uid_mutex`: backend emulation stop → discard in-flight assembly (irq-locked) → set NFCID1 → backend emulation restart; `s_active_uid` updated; `uid_rotation_count++`.
- **Errors:**
  - `-EINVAL` — `uid == NULL` or `uid->len ∉ {4,7,10}`.
  - `-ENODEV` — state != STARTED (including: a concurrent `stop()` won the mutex race and the module is now STOPPED).
  - `-EIO` — any backend stop/set-NFCID1/restart failure; `STATS_ERROR`; on a failed restart the module is forced to ERROR state.
- **Threadsafe note:** backend emulation stop guarantees no new backend callbacks after it returns; `s_uid_mutex` serializes concurrent `set_uid` callers **and** `nfc_transport_stop()` (which acquires the same mutex — §3 stop contract). `@threadsafe` w.r.t. other threads and w.r.t. `stop()`; the caller must not race `init/start/shutdown` (those are `@caller_sync`). The WQ thread keeps running across rotation.

> **DECISION (set_uid semantics):** Spec v2 marks `set_uid` `@threadsafe` and describes "stop → set NFCID1 → start". This is the **backend emulation** toggle (nRFX: `nfc_t4t_emulation_*`), *not* the module lifecycle stop/start — the module stays STARTED and the work queue is not torn down. `set_uid` is rejected with `-ENODEV` unless STARTED; to change the UID while stopped, pass the new UID to the next `start(uid)`.

### `nfc_transport_send_response(buf, len)`
- **Pre:** state == STARTED. Typically called from `nfc_work_q` (service `on_apdu`), but `@threadsafe`.
- **Post (0):** response handed to the backend OK (nRFX: `nfc_t4t_response_pdu_send(buf, len)` — §6); `response_tx_count++`. `buf` is borrowed by the transport until the next transport event — **caller keeps it valid**.
- **Errors:**
  - `-EINVAL` — `buf == NULL`, `len == 0`, or `len > NFC_TRANSPORT_MAX_RESPONSE_LEN` (0xFFF0; the nRFX backend BUILD_ASSERTs this equals `NFC_T4T_MAX_PAYLOAD_SIZE`).
  - `-ENODEV` — state != STARTED.
  - `-EIO` — backend returned nonzero (vendor error mapped — §5.1); `STATS_ERROR`.
- **Deferred-response safety (ISO-DEP half-duplex guarantee):** ISO 14443-4 is half-duplex — the reader cannot transmit a new C-APDU until it has received the R-APDU for the previous one. Therefore no new `DATA_IND` can reach `nfc_work_q` while a response is pending, and a service may legally defer `send_response()` to a subsequently queued work item (via `nfc_transport_submit_work()` — e.g. Aliro crypto) without violating the borrow contract. A field-off during the deferred window cancels the exchange; the work handler must check it still has a live session before sending.

### `nfc_transport_submit_work(work)`
- **Pre:** state == STARTED. `work` is caller-owned and initialized (`k_work_init`).
- **Post (0):** `k_work_submit_to_queue(&s_work_q, work)` returned >= 0; the handler will run on `nfc_work_q` after the currently executing item (same-queue self-submit from a service callback is the intended pattern).
- **Errors:** `-EINVAL` (`work == NULL`) · `-ENODEV` (state != STARTED) · negative `k_work_submit_to_queue` errors passed through.

### Getters
- `get_config` — returns `&s_config`, never NULL, valid before `init()` (`@isr_safe`).
- `get_stats` — `STATS_COPY_OUT(&s_stats_lock, s_stats, out)`; `-EINVAL` if `out == NULL`; else 0 (`@threadsafe`).
- `get_state` — `(nfc_transport_state_t)atomic_get(&s_state)`; never fails (`@isr_safe`).
- `get_capabilities` — returns the backend's `static const nfc_transport_caps_t *`; never NULL; `@isr_safe`. (Same contract as `get_config`: returns a static pointer; callers must not modify the struct. Declared in `nfc_transport.h`; implemented per-backend. See §1.8.)

---

## 4. Internal State

### 4.1 File-static state (`nfc_transport_nrfx.c`)

```c
/* Lifecycle — Pattern B (read from ISR/WQ + written by @caller_sync lifecycle) */
static atomic_t s_state;                       /* nfc_transport_state_t values; 0 == UNINIT */

/* Required module statics */
static nfc_transport_config_t s_config = { .fwi = NFC_TRANSPORT_DEFAULT_FWI };
static nfc_transport_stats_t  s_stats;
static struct k_spinlock      s_stats_lock;    /* NOT K_SPINLOCK_DEFINE (NCS 3.2.4) */

/* Upward callbacks (Pattern A) */
static const nfc_transport_ops_t *s_ops;       /* NULL until registered */
static void                      *s_user_ctx;

/* UID rotation */
static nfc_uid_t        s_active_uid;           /* last applied NFCID1 */
static struct k_mutex   s_uid_mutex;            /* serializes set_uid AND stop (thread ctx) */

/* Work queue (dedicated, high priority) */
static K_THREAD_STACK_DEFINE(s_workq_stack, CONFIG_NFC_HAL_WORKQ_STACK_SIZE);
static struct k_work_q  s_work_q;
static struct k_work    s_field_on_work;
static struct k_work    s_field_off_work;
static struct k_work    s_apdu_work;            /* drains the fifo */

/* Inbound fragment->APDU transport (ISR -> WQ) */
NET_BUF_POOL_FIXED_DEFINE(nfc_apdu_pool, CONFIG_NFC_APDU_POOL_COUNT,
                          CONFIG_NFC_APDU_BUF_SIZE, 0, NULL); /* user_data_size 0 */
static struct k_fifo    s_apdu_fifo;

/* ISR-only assembly state (single t4t-callback context; no lock needed between
 * fragments, irq_lock only where a thread (stop/set_uid) must discard it) */
static struct net_buf  *s_cur_buf;              /* APDU under assembly, or NULL */
static enum { ASM_OK, ASM_DROP_NOBUF, ASM_DROP_OVERSIZE } s_asm_drop; /* current chain */
```

### 4.2 Lifecycle state machine (Pattern B)

| From | Event | To | nrfxlib / kernel action |
|---|---|---|---|
| UNINITIALIZED | `init()` | INITIALIZED | CAS; `STATS_RESET` (first statement after the guard); `nfc_t4t_setup`; `parameter_set(FWI)`; `k_fifo_init`; `k_work_init` x3; `k_mutex_init` |
| INITIALIZED | `init()` | INITIALIZED | reject `-EALREADY` |
| INITIALIZED/STOPPED | `start(uid)` | STARTED | `parameter_set(NFCID1)`; `k_work_queue_start`; `nfc_t4t_emulation_start` |
| STARTED | `start()` | STARTED | no-op, return 0 |
| STARTED | `stop()` | STOPPED | `k_mutex_lock(s_uid_mutex)`; `nfc_t4t_emulation_stop`; discard `s_cur_buf`; `k_work_cancel_sync` x3; drain fifo; `k_work_queue_drain(...,true)`; `k_work_queue_stop`; `k_mutex_unlock` (serializes against `set_uid`) |
| INITIALIZED/STOPPED | `stop()` | (unchanged) | no-op, return 0 |
| STARTED | `set_uid()` | STARTED | `emulation_stop` → discard `s_cur_buf` → `parameter_set(NFCID1)` → `emulation_start` |
| any | `shutdown()` | UNINITIALIZED | implicit `stop()` if STARTED; `nfc_t4t_done`; CAS → UNINIT |
| STARTED (rotation restart fails) | — | ERROR | terminal until `shutdown()` |

Init/shutdown use CAS (`atomic_cas`) — never `atomic_get`+`atomic_set` (creed Pattern B, TOCTOU rule).

> **DECISION (no shared lifecycle helper):** The creed references `src/system/module_lifecycle.h` (`mod_claim_init`, `mod_init_abort`, `mod_claim_shutdown`, `mod_get_state`) but that file does **not exist** in this repo (only `src/stats.h` and `src/nfc_emulation.h` are present). The HAL implements Pattern B inline with `atomic_cas`/`atomic_get` and small file-static helpers (`state_claim`, `state_set`, `state_get`) so the layer is self-contained. If the shared header lands later, migrate to it.

### 4.3 ISR vs thread ownership

| State | Written by | Read by | Sync |
|---|---|---|---|
| `s_state` | lifecycle (thread, CAS) | ISR cb, WQ, getters | `atomic_t` |
| `s_stats.*` | ISR cb, WQ, lifecycle | `get_stats` (thread) | `s_stats_lock` |
| `s_config` | `init()` (thread) | getter, ISR | frozen after init |
| `s_ops`/`s_user_ctx` | `register_callbacks` (thread, not running) | WQ handlers | no race (only set when not STARTED; only read when STARTED) |
| `s_active_uid` | `start`/`set_uid` (thread) | `set_uid` | `s_uid_mutex` (also held by `stop()` across emulation-stop + WQ teardown, serializing stop against rotation) |
| `s_cur_buf`,`s_asm_drop` | t4t cb (ISR) | t4t cb (ISR); discarded by `stop`/`set_uid`/field events | single ISR context; `irq_lock` only for thread-side discard; `discard_cur_buf` resets **both** `s_cur_buf` and `s_asm_drop` |
| `s_apdu_fifo` | ISR put / WQ get | ISR + WQ | `k_fifo` (ISR-safe) |

### 4.4 t4t callback (ISR/library context) — the assembly state machine

```c
static void nfc_isr_cb(void *context, nfc_t4t_event_t event,
                       const uint8_t *data, size_t data_length, uint32_t flags)
{
    ARG_UNUSED(context);
    switch (event) {
    case NFC_T4T_EVENT_FIELD_ON:
        discard_cur_buf();                 /* defensive: stale partial */
        STATS_INC(&s_stats_lock, s_stats, field_on_count);
        (void)k_work_submit_to_queue(&s_work_q, &s_field_on_work);
        break;
    case NFC_T4T_EVENT_FIELD_OFF:
        discard_cur_buf();                 /* drop in-progress APDU */
        STATS_INC(&s_stats_lock, s_stats, field_off_count);
        (void)k_work_submit_to_queue(&s_work_q, &s_field_off_work);
        break;
    case NFC_T4T_EVENT_DATA_IND:
        nfc_hal_on_fragment(data, data_length,
                            (flags & NFC_T4T_DI_FLAG_MORE) != 0U);
        break;
    case NFC_T4T_EVENT_DATA_TRANSMITTED: /* response flushed; nothing to do */
    case NFC_T4T_EVENT_NDEF_READ:        /* not possible in raw PICC mode */
    case NFC_T4T_EVENT_NDEF_UPDATED:
    case NFC_T4T_EVENT_NONE:
    default:
        break;                            /* MISRA 16.4: explicit default */
    }
}
```

`nfc_hal_on_fragment(data, len, more)` (ISR), batching counters via the §6 hot-path helper:
1. **First fragment of a chain** (`s_cur_buf == NULL && s_asm_drop == ASM_OK`):
   - `s_cur_buf = net_buf_alloc(&nfc_apdu_pool, K_NO_WAIT)`. If NULL → `s_asm_drop = ASM_DROP_NOBUF`, bump `frag_dropped_no_buffer`, continue to step 3 (consume rest of chain, no response).
2. **Append** (if `s_asm_drop == ASM_OK`): if `len > net_buf_tailroom(s_cur_buf)` → `s_asm_drop = ASM_DROP_OVERSIZE`, `net_buf_unref(s_cur_buf)`, `s_cur_buf = NULL`. Else `net_buf_add_mem(s_cur_buf, data, len)` (immediate copy — nrfxlib lifetime undocumented). Bump `fragment_rx_count`.
3. **Final fragment** (`!more`):
   - `ASM_OK`: `k_fifo_put(&s_apdu_fifo, s_cur_buf)`; bump `apdu_assembled_count`; `s_cur_buf = NULL`; `k_work_submit_to_queue(&s_work_q, &s_apdu_work)`.
   - `ASM_DROP_OVERSIZE`: send 6700 inline — `(void)nfc_t4t_response_pdu_send(s_sw_wrong_length, 2U)`; bump `apdu_oversized_count`.
   - `ASM_DROP_NOBUF`: no response (reader retries / FWI timeout); counter already bumped.
   - Reset `s_asm_drop = ASM_OK`.

> **DECISION (oversized 6700 sent inline from the callback):** `nfc_t4t_response_pdu_send` is the *intended* reply mechanism from within the DATA_IND callback (per header: "The Application then has to reply with a call to nfc_t4t_response_pdu_send"). The 6700 buffer is file-static `const`, so its borrow-until-next-callback lifetime is trivially satisfied. No deferral to the WQ is needed.
>
> **DECISION (no response on pool exhaustion):** CONVENTIONS §5 says pool exhaustion is normal — "bump dropped stat and return." The HAL sends nothing; the reader retransmits the I-block (lib re-delivers DATA_IND, alloc retried) or times out at FWI. Documented consequence: up to one FWI window (~5 s) of reader stall under sustained pool starvation.

### 4.5 WQ handlers (`nfc_work_q` thread)

```c
static void field_on_handler(struct k_work *w)  { if (s_ops && s_ops->on_field_on)  s_ops->on_field_on(s_user_ctx); }
static void field_off_handler(struct k_work *w) { if (s_ops && s_ops->on_field_off) s_ops->on_field_off(s_user_ctx); }
static void apdu_handler(struct k_work *w) {
    struct net_buf *buf;
    while ((buf = k_fifo_get(&s_apdu_fifo, K_NO_WAIT)) != NULL) {
        if (s_ops && s_ops->on_apdu) {
            s_ops->on_apdu(buf, s_user_ctx);     /* callee owns + unrefs */
        } else {
            net_buf_unref(buf);                  /* no consumer */
            STATS_INC(&s_stats_lock, s_stats, apdu_dropped_no_consumer);
        }
    }
}
```

**Ordering guarantee:** all three work items target the same queue; Zephyr processes distinct items in submission order. `field_on` is submitted before any DATA_IND of a session; `apdu_work` (submitted at each final fragment) is processed before a later-submitted `field_off`; `apdu_handler` drains the whole fifo per run so coalesced re-submits lose nothing. Pathological reverse-order field toggles within one unprocessed batch are not reachable at FWI timescales (single reader session bounded by field on/off).

---

## 5. Integration Points

### 5.1 Down — nrfxlib / Zephyr called by the HAL

| Call | Where | Notes |
|---|---|---|
| `nfc_t4t_setup(nfc_isr_cb, NULL)` | `init` | registers cb; raw PICC mode (no payload set) locked at first `emulation_start` |
| `nfc_t4t_parameter_set(NFC_T4T_PARAM_FWI, &fwi, 1)` | `init` | FWI from `s_config.fwi` (default 8) before any start |
| `nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, uid->bytes, uid->len)` | `start`, `set_uid` | 4/7/10-byte NFCID1 |
| `nfc_t4t_emulation_start()` | `start`, `set_uid` | begins event flow |
| `nfc_t4t_emulation_stop()` | `stop`, `set_uid` | callbacks cease before this returns |
| `nfc_t4t_response_pdu_send(buf, len)` | `send_response`, oversized-6700 | borrows buf until next callback |
| `nfc_t4t_done()` | `shutdown` | releases callback; re-init required |
| `net_buf_alloc(&nfc_apdu_pool, K_NO_WAIT)` / `net_buf_add_mem` / `net_buf_unref` | ISR + WQ | FIXED pool, one-owner |
| `k_fifo_init/put/get` | ISR + WQ | DEV-ZEP-005 |
| `k_work_init/submit_to_queue/cancel_sync`, `k_work_queue_start/drain/stop` | lifecycle + ISR | dedicated `nfc_work_q` |
| `atomic_cas/atomic_get`, `k_spin_lock/unlock` (via `STATS_*`), `k_mutex_*`, `irq_lock/irq_unlock` | various | state, stats, uid serialization, assembly discard |

nrfxlib returns `-NRF_E*` (`nrf_nfc_errno.h`). Mapping: **any nonzero lib return → `-EIO`** + `STATS_ERROR(code)` (our own arg validation catches `-EINVAL` cases earlier).

### 5.2 Up — consumers of the HAL

| Consumer | Mechanism | Lock-for-Wave |
|---|---|---|
| framing (`apdu_assembler`, Wave 2) | registers `nfc_transport_ops_t` via `nfc_transport_register_callbacks(ops, user_ctx)`; receives `on_field_on/off` + `on_apdu(struct net_buf*, user_ctx)` on `nfc_work_q`; **must `net_buf_unref` the APDU** after dispatch | **§1.5 ops form is the locked contract** |
| services (Wave 5) | call `nfc_transport_send_response(buf, len)` from `on_apdu` (sync) or from `nfc_work_q` after deferred crypto (Aliro); service owns the static response buffer | borrow-until-next-callback |
| `nfc_stack` (Wave 4) | orchestrates `init → register_callbacks → start(uid) → stop → shutdown`; calls `set_uid` on rotation. **All cross-layer wiring lives in `nfc_stack.c`** (STACK_SPEC rule 2): registers framing ops after `nfc_transport_init()` and before `nfc_transport_start()` | lifecycle order |

### 5.3 Shell (`nfc_transport_shell_cmds.c`, CONVENTIONS §10)

```c
SHELL_STATIC_SUBCMD_SET_CREATE(nfc_transport_cmds,
    SHELL_CMD(config, NULL, "Print config (fwi)",            cmd_nfc_transport_config),
    SHELL_CMD(stats,  NULL, "Print stats (all counters)",    cmd_nfc_transport_stats),
    SHELL_CMD(state,  NULL, "Print lifecycle state",         cmd_nfc_transport_state),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(nfc_transport, &nfc_transport_cmds, "NFC HAL control", NULL);
```

`stats` prints every field including `error_count`/`last_error_code` and all drop counters (via `nfc_transport_get_stats(&snap)` into a stack-local). `state` maps the enum to a string with a `default` arm. Output uses `shell_print` (DEV-ZEP-009).

---

## 6. Implementation Notes

### Non-goals (spec v2 §13)

Reader/initiator mode is **out of scope for this slice** — not implemented in Wave 1. The NFCT backend is card-only and the BUILD_ASSERT in §1.7 enforces this at compile time.

> **Spec v3 framing update:** Spec v2 §13 described the reader role as arriving via
> a "sibling `nfc_reader_*` transport." Spec v3 supersedes this: the reader role
> lives in the **same `nfc_transport` HAL** as a reserved poller sub-API (§A),
> gated by `CONFIG_NFC_ROLE_READER` and advertised through the capability
> descriptor (§1.8). No separate sibling transport is created. The card-mode NFCT
> implementation is unchanged by this reframing.

### Event model — upward API is event-driven; backends bridge their driver (spec v3 §4.1)

Both the listen sub-API (built now) and the reserved poller sub-API (§A) present an
**event-driven contract** at the HAL boundary: the caller registers callbacks and
receives events (`on_field_on/off`, `on_apdu`; future `on_tag_detected/removed`).
The caller never polls the HAL directly.

Each backend bridges its own driver model to this contract internally:

- **NFCT backend (this plan):** natively ISR-driven. `nfc_isr_cb` fires directly
  from the NFCT hardware interrupt, enqueues work items to `nfc_work_q`, and
  delivers events upward on that queue. No additional worker thread is needed
  between the ISR and the HAL event boundary.
- **RFAL-class backends (ST25R3916 — reserved; driver already in-repo):**
  worker-driven. The backend thread calls `NfcTransportRfal::Execute()` (see
  `aliro/platform/nfc/nfc_transport_rfal.h`) which drives the RFAL iteration
  (`rfalNfcWorker`). State changes are delivered via `RfalNotifyCallback(rfalNfcState
  state)`, which the backend translates into the same HAL events. This
  poll-loop-to-event bridging is **fully contained in the backend** — nothing
  above the HAL (`framing`, `aid_router`, protocol modules, `nfc_stack`) sees it.

The stack above the HAL is identical for both backends; only the backend `.c` file
changes.

### nRFX backend implementation notes (`nfc_transport_nrfx.c` only — vendor specifics never appear in `nfc_transport.h`)
- **Callback context:** the t4t callback runs in the library/HAL_NFC callback context. The spec and conventions lock it as **ISR context** (`nfc_platform_cb_request` may call directly from the NFCT IRQ). Treat it as ISR: no sleep, no blocking, FIXED-pool `K_NO_WAIT` allocation only, immediate copy.
- **Complete APDU vs fragments — VERIFIED from `nfc_t4t_lib.h`:** `NFC_T4T_EVENT_DATA_IND` delivers **APDU *fragments*** (PCB-stripped I-block payloads). All fragments with `NFC_T4T_DI_FLAG_MORE` set belong to one C-APDU; the first DATA_IND with `MORE` clear completes it. Short APDUs arrive as a single DATA_IND with `MORE` clear. **Therefore the HAL must assemble** (it cannot assume one DATA_IND == one APDU). The library handles ISO-DEP framing/PCB/WTX/CRC/response-defragmentation internally — the HAL never touches those.
- **Response borrow lifetime:** the response PDU passed to `nfc_t4t_response_pdu_send` "must be kept valid until the next callback" (e.g. next DATA_IND or FIELD_OFF). Hence: services keep their static response buffer valid (no copy in the HAL), and the HAL's `s_sw_wrong_length` is file-static `const`.
- **Inbound data lifetime undocumented:** the `data` pointer in DATA_IND must be copied immediately (`net_buf_add_mem`) — never stored.
- **Raw PICC mode:** achieved by `nfc_t4t_setup()` with **no** `ndef_*payload_set` call before `emulation_start`. Never call `nfc_t4t_ndef_rwpayload_set/staticpayload_set` (they switch to NDEF mode and bypass DATA_IND).
- **FWI:** `NFC_T4T_PARAM_FWI` (max bounded by `NFC_T4T_PARAM_FWI_MAX`, default 8). Set FWI=8 (`NFC_T4T_FWI_MAX_VAL_NFC`) before the first `emulation_start` for the widest (~5 s) reader wait window — headroom for Aliro crypto on `nfc_work_q`.
- **UID rotation:** `emulation_stop()` → `parameter_set(NFC_T4T_PARAM_NFCID1, ...)` → `emulation_start()`. `emulation_stop` guarantees no further callbacks before it returns; discard any partial assembly buffer under `irq_lock` during the gap.
- **Stop vs. rotation:** `nfc_transport_stop()` takes `s_uid_mutex` before `nfc_t4t_emulation_stop()` and holds it across the WQ teardown — serializing against `set_uid` so a mid-callback rotation cannot re-enable emulation after stop. A rotation that loses the race observes the STOPPED state under the mutex and fails with `-ENODEV`.
- **Response length limit:** the backend `BUILD_ASSERT`s `NFC_TRANSPORT_MAX_RESPONSE_LEN == NFC_T4T_MAX_PAYLOAD_SIZE` so the vendor-clean public constant can never drift from the library limit.
- **Error namespace:** lib uses `-NRF_E*` (positive macros in `nrf_nfc_errno.h`, returned negated). Never propagate these upward — map to standard errno (`-EIO`).

### Work queue sizing rationale
- Dedicated **named** `nfc_work_q` (CONVENTIONS §7 / creed §8) — never the system WQ. Drains the APDU fifo, runs framing→router→service, and (for Aliro) the deferred crypto + response. Stack default 2048 B sized for crypto + response build; priority default 5 (high, dedicated). Both Kconfig-tunable; pin from Thread Analyzer (creed §11). Document the measured peak in the HAL README before production.
- Teardown uses `k_work_cancel_sync` (not the async variant) from thread context (creed §8) so no handler runs against drained state, then `k_work_queue_drain(..., true)` + `k_work_queue_stop` to allow restart.
- **Never** call `k_work_submit_to_queue`/`k_fifo_put` while holding `s_stats_lock` (creed §8): the ISR does `STATS_*` first (lock released), then `k_fifo_put`/submit.

### Stats / hot path (CONVENTIONS §6, STATS_API_DESIGN)
- `static nfc_transport_stats_t s_stats; static struct k_spinlock s_stats_lock;` — exact names; **not** `K_SPINLOCK_DEFINE`.
- `STATS_RESET(s_stats)` is the **first statement after the `-EALREADY` CAS guard** in `init()` — a rejected double-`init()` must not wipe live stats (CONVENTIONS §6).
- Mandatory `error_count`/`last_error_code` via `STATS_ERROR` on every non-fatal failure.
- **Hot-path helper:** `nfc_hal_on_fragment` routes its counter updates through one private helper `nfc_transport_stats_on_fragment(rx, dropped_nobuf, oversized, assembled)` wrapping a single `STATS_SCOPE` per logical fragment event.
  - > **DECISION (no latency field):** CONVENTIONS §6 describes the hot-path helper "with elapsed time computed before the scope," implying an embedded `latency_stats_t`. The spec's HAL stats struct has no latency field **and the `latency_stats` utility is not present in this repo** (only `src/stats.h` exists). The fragment helper therefore batches counters only; a `latency_stats_t fragment_rx_time` field can be added later if/when the utility lands. Flagged for human review.
- `get_stats` is copy-out (`STATS_COPY_OUT`) — never returns a pointer into `s_stats`.

### MISRA C:2012 decisions + DEV-ZEP citations
- `K_NO_WAIT` (ISR alloc), `K_FOREVER` n/a, `K_MSEC` n/a here → **DEV-ZEP-013** (timeout compound literals).
- `k_fifo_put`/`k_fifo_get` intrusive void* linkage → **DEV-ZEP-005**.
- `CONTAINER_OF` is **not** needed (work handlers act on file-static state, not per-object recovery) — if a handler ever recovers a parent struct, cite **DEV-ZEP-001**.
- `K_THREAD_STACK_DEFINE` / `k_work_queue_start` thread entry → **DEV-ZEP-007** (function-pointer conversion).
- `LOG_*` (module logging) → **DEV-ZEP-008**; `shell_print` in shell cmds → **DEV-ZEP-009**.
- `BIT()` for `NFC_T4T_DI_FLAG_MORE` is unnecessary (the enum value `0x01` is used directly); if bit math is introduced cite **DEV-ZEP-017**.
- `ARG_UNUSED` in the t4t callback (`context`) and work handlers (`w`) and shell handlers → **DEV-ZEP-016**.
- `K_SPINLOCK` scoped macro is **not** used — `STATS_*` macros use explicit `k_spin_lock/unlock` pairs; no DEV-ZEP-014 needed.
- General MISRA: fixed-width types + `U` suffix on unsigned literals (Rule 7.2); all locals initialized at declaration (Rule 9.1); every `switch` has `default` (Rule 16.4); controlling expressions Boolean (Rule 14.4, e.g. `(flags & NFC_T4T_DI_FLAG_MORE) != 0U`); all lib/kernel return values checked or explicitly `(void)`-cast (Rule 17.7); no dynamic allocation in the running phase (only the FIXED net_buf pool, ISR `K_NO_WAIT`).

---

## 7. Conventions Compliance (CONVENTIONS §12)

- [x] **Lifecycle matches §2** — Full (init/start/stop/shutdown), restartable for UID rotation; `shutdown` not `deinit` (§2, spec v2 §6.1).
- [x] **`config_t`/`stats_t`/`state_t` + three getters** — §1.2/1.3/1.4/2; `get_config` never NULL, `get_stats` copy-out, `get_state` never fails.
- [x] **State storage Pattern B** — `atomic_t s_state` read from ISR + WQ; CAS init/shutdown, inline (no shared helper header — §4.2 DECISION).
- [x] **Coupling §3 / callbacks §4** — Up to framing via Pattern A ops struct (`register_callbacks(ops, user_ctx)`); `user_ctx` name; guard `-ENODEV`/`-EBUSY`/`-EIO`; NULL clears; every invoker NULL-checks; dispatch thread (`nfc_work_q`) documented.
- [x] **Buffer model §5** — net_buf inbound (single FIXED `nfc_apdu_pool`, ISR `K_NO_WAIT`, immediate `net_buf_add_mem`, fifo handoff, one owner, drop+stat on exhaustion, 6700+drop on oversize); static outbound (service-owned, borrowed by nrfxlib).
- [x] **Stats recipe §6** — `s_stats`+`s_stats_lock`, `STATS_RESET` as the first statement after the `-EALREADY` guard in init, copy-out getter, named drop counters, single-`STATS_SCOPE` hot-path fragment helper.
- [x] **Threading + annotations §7** — `@caller_sync` lifecycle, `@threadsafe` (`set_uid`, `send_response`, `get_stats`), `@isr_safe` (`get_config`, `get_state`); ISR rules honored; dedicated named `nfc_work_q`.
- [x] **Error codes §9; shell §10** — standard errno only; ISO 7816 6700 is a protocol response, not a return code; `nfc_transport` shell with `config`/`stats`/`state` in dedicated `nfc_transport_shell_cmds.c`.
- [x] **MISRA notes + DEV-ZEP §11** — §6 cites DEV-ZEP-005/007/008/009/013/016.

---

## 8. Tasks

> Granularity 2–5 min/step. TDD applies to the **portable/pure helpers** (UID validation, errno mapping, oversize decision) — the nrfxlib-coupled paths cannot run on `native_sim` (closed `libnfc_t4t.a` is nRF-only), so they are covered by on-target integration checks on the DK (`nrf54l15dk/nrf54l15/cpuapp`), not ztest. **Platform note:** `native_sim` is Linux-CI-only in this repo (macOS blocked for arch/posix in this repo's tooling); local unit tests for the pure helpers target `qemu_cortex_m33` or hardware Ztest on the DK. Test path convention: `tests/unit/nfc_transport/` (standalone `CMakeLists.txt` + `prj.conf` + `testcase.yaml`, built `--no-sysbuild`). Commit after each numbered group.

- [ ] **1. Scaffolding.** Create `src/nfc/hal/CMakeLists.txt` (add the backend source when its symbol is set, plus shell cmds) and `src/nfc/hal/Kconfig` (`NFC_HAL_WORKQ_STACK_SIZE` default 2048, `NFC_HAL_WORKQ_PRIORITY` default 5, plus the `NFC_HAL_BACKEND` choice from §1.7 — `NFC_HAL_BACKEND_NRFX` default, `depends on NFC_T4T_NRFXLIB`). Stub:
  ```cmake
  if(CONFIG_NFC_STACK)
    target_sources(app PRIVATE nfc_transport_shell_cmds.c)
    if(CONFIG_NFC_HAL_BACKEND_NRFX)
      target_sources(app PRIVATE nfc_transport_nrfx.c)
    endif()
  endif()
  ```
  **Build-order bootstrap:** Wave-1-time builds use a provisional minimal `src/nfc/Kconfig` stub defining `NFC_STACK` + `NFC_APDU_BUF_SIZE`/`NFC_APDU_POOL_COUNT` (replaced by Wave 4's full version), wired per spec v2 §9: the application top-level `Kconfig` `rsource`s `src/nfc/Kconfig` (which `rsource`s `hal/Kconfig`) and the top-level `CMakeLists.txt` does `add_subdirectory(src/nfc)`.
- [ ] **2. Header types.** Write `nfc_transport.h` (vendor-clean — no nrfxlib includes/types/constants): memory comment, `nfc_uid_t` + length macros, `nfc_transport_config_t` + `NFC_TRANSPORT_DEFAULT_FWI` (ISO-DEP FWI), `NFC_TRANSPORT_MAX_RESPONSE_LEN`, `nfc_transport_stats_t`, `nfc_transport_state_t`, `nfc_transport_ops_t`, all 12 public prototypes (incl. `nfc_transport_submit_work`) with `@`-annotations and Doxygen (note `on_apdu` callee-owns-ref, response borrow lifetime, half-duplex deferred-response note). **Commit.**
- [ ] **2a. Capability descriptor (header + backend stub — Wave 1 interface only).** Add to `nfc_transport.h`: `nfc_role_t` bitmask (`NFC_ROLE_CARD/READER`), `nfc_tech_t` bitmask (all technologies from §1.8), `nfc_transport_caps_t`, and `nfc_transport_get_capabilities()` declaration. Add to `nfc_transport_nrfx.c`: static `s_nrfx_caps = {NFC_ROLE_CARD, NFC_TECH_ISO_DEP_A}` and its `nfc_transport_get_capabilities()` implementation. Add the `CONFIG_NFC_ROLE_READER` BUILD_ASSERT (§1.7) to the backend file. Add `NFC_ROLE_CARD` / `NFC_ROLE_READER` to `src/nfc/hal/Kconfig` per §1.7. No reader-role code paths wired. **Commit after task 2.**
- [ ] **2b. Reserved poller sub-API header (interface only — Wave 1).** Add the `#if defined(CONFIG_NFC_ROLE_READER)` block from §A to `nfc_transport.h`: `nfc_transport_poller_ops_t`, `nfc_transport_poll_start/stop`, `nfc_transport_transceive`. Add the NFCT `-ENOTSUP` stubs to `nfc_transport_nrfx.c`. Verify the header compiles with `CONFIG_NFC_ROLE_READER=n` (default) and that the BUILD_ASSERT fires when `=y` against the NRFX backend. **Interface-only for this slice; no implementation.** **Commit after task 2a.**
- [ ] **3. (TDD) UID validation.** Add static `static bool nfc_uid_len_valid(uint8_t len)` → `len==4||7||10`. Ztest `test_uid_len_valid` (4/7/10 pass; 0/5/8/11 fail) first. **Commit.**
- [ ] **4. (TDD) errno mapping.** Add `static int nrf_to_errno(int rc)` → `rc==0?0:-EIO`. Ztest `test_nrf_to_errno` (0→0; -22/-45→-EIO). **Commit.**
- [ ] **5. Statics + state helpers.** In `nfc_transport_nrfx.c`: all §4.1 statics; `state_get/state_claim(from,to)/state_set(v)` over `atomic_cas/atomic_get`; `NET_BUF_POOL_FIXED_DEFINE(nfc_apdu_pool,...)`. `LOG_MODULE_REGISTER(nfc_transport, ...)`.
- [ ] **6. Getters.** `get_config` (`&s_config`), `get_state` (`state_get`), `get_stats` (`STATS_COPY_OUT`). **Commit.**
- [ ] **7. init/shutdown.** `init`: `state_claim(UNINIT,INIT)` (`-EALREADY`) → `STATS_RESET` (**first statement after the guard** — CONVENTIONS §6) → `k_fifo_init`, `k_work_init` x3, `k_mutex_init` → `nfc_t4t_setup(nfc_isr_cb,NULL)` → `parameter_set(FWI)` → on any lib error roll back to UNINIT + `nfc_t4t_done` + `STATS_ERROR` + `-EIO`. `shutdown`: idempotent; implicit stop if STARTED; `nfc_t4t_done`; `state_set(UNINIT)`; always return 0. **Commit.**
- [ ] **8. register_callbacks.** Guard table (`-ENODEV`/`-EBUSY`/`-EIO`); store/clear `s_ops`,`s_user_ctx`. **Commit.**
- [ ] **9. WQ handlers + assembly helper.** `field_on_handler`, `field_off_handler`, `apdu_handler` (drain loop, callee-owns or unref+`apdu_dropped_no_consumer`); `discard_cur_buf()` (irq-locked unref of `s_cur_buf` **and reset `s_asm_drop = ASM_OK`** so a mid-chain field loss / stop / rotation cannot bleed a `DROP_*` flag into the next session); `nfc_hal_on_fragment()` per §4.4; private `nfc_transport_stats_on_fragment()` single `STATS_SCOPE`. **Commit.**
- [ ] **10. (TDD) oversize decision.** Factor the oversize test into pure `static bool frag_fits(size_t cur_len, size_t add, size_t cap)`. Ztest boundary cases (fits exactly, one over). Use it in `nfc_hal_on_fragment`. **Commit.**
- [ ] **11. t4t callback.** `nfc_isr_cb` per §4.4 (`ARG_UNUSED(context)`, `switch` with `default`, MORE-flag Boolean test, inline 6700 on oversize, no-response on no-buffer). **Commit.**
- [ ] **12. start/stop.** `start`: state guard + idempotent; `nfc_uid_len_valid` (`-EINVAL`); `parameter_set(NFCID1)`; `k_work_queue_start(&s_work_q, s_workq_stack, K_THREAD_STACK_SIZEOF(...), CONFIG_NFC_HAL_WORKQ_PRIORITY, &(struct k_work_queue_config){.name="nfc_work_q"})`; `emulation_start`; rollback on error. `stop`: idempotent; `k_mutex_lock(&s_uid_mutex, K_FOREVER)` **before** `emulation_stop` (serializes against `set_uid` — a mid-callback rotation cannot re-enable emulation after stop); `emulation_stop`; `discard_cur_buf`; `k_work_cancel_sync` x3; drain `s_apdu_fifo` (unref); `k_work_queue_drain(...,true)`; `k_work_queue_stop`; `k_mutex_unlock`. **Commit.**
- [ ] **13. set_uid.** `-EINVAL`/`-ENODEV` guards; `k_mutex_lock(&s_uid_mutex, K_FOREVER)`; `emulation_stop` → `discard_cur_buf` (irq-locked) → `parameter_set(NFCID1)` → `emulation_start`; on restart failure force `state_set(ERROR)` + `-EIO`; `uid_rotation_count++`; unlock. **Commit.**
- [ ] **14. send_response.** `-EINVAL` (NULL/len 0/len>`NFC_TRANSPORT_MAX_RESPONSE_LEN`), `-ENODEV` (not STARTED); `BUILD_ASSERT(NFC_TRANSPORT_MAX_RESPONSE_LEN == NFC_T4T_MAX_PAYLOAD_SIZE)` in the backend; `nfc_t4t_response_pdu_send`; map `-EIO`+`STATS_ERROR`; `response_tx_count++`. Also `nfc_transport_submit_work()`: `-EINVAL`/`-ENODEV` guards then `k_work_submit_to_queue(&s_work_q, work)`. **Commit.**
- [ ] **15. Shell.** `nfc_transport_shell_cmds.c`: `config`/`stats`/`state` via getters into stack-local snapshots; `state` switch has `default`; `SHELL_CMD_REGISTER(nfc_transport, ...)`. **Commit.**
- [ ] **16. Build + lint.** Build with `CONFIG_NFC_STACK=y` (NDEF profile minimal; `NFC_HAL_BACKEND_NRFX` from the choice default) against NCS v3.2.4 targeting `nrf54l15dk/nrf54l15/cpuapp` (primary bring-up board — has NFC antenna on DK):
  ```
  west build -b nrf54l15dk/nrf54l15/cpuapp -- -DCONFIG_NFC_STACK=y
  ```
  nRF52840 DK (`nrf52840dk/nrf52840`) is a valid secondary target. Using the provisional `src/nfc/Kconfig` stub + top-level Kconfig/CMake hooks from task 1 (the stub is replaced by Wave 4's full `src/nfc/Kconfig`); run cppcheck MISRA with the project `misra-suppressions.txt`; confirm the DEV-ZEP suppressions cover all flagged kernel-macro uses. Document the Thread Analyzer `nfc_work_q` stack peak in the HAL README. **Commit.**

---

## A. Reserved Poller Sub-API (DESIGNED — NOT IMPLEMENTED IN WAVE 1)

> **Status:** Interface defined per spec v3 §4.1 + §7. **Not implemented in this
> Wave 1 slice.** Gated entirely under `CONFIG_NFC_ROLE_READER`. The NFCT backend
> provides `-ENOTSUP` stubs (prevented from being activated by the BUILD_ASSERT in
> §1.7). The **ST25R3916/RFAL backend** (`aliro/platform/nfc/nfc_transport_rfal.h`
> — already in-repo) is the intended first implementer. Do NOT wire implementation
> code for these functions in Wave 1.

The poller sub-interface lives in `nfc_transport.h` alongside the listen sub-API.
Declaring it here locks the seam so the second backend (RFAL) can implement it
without reworking the header contract.

```c
/* ---- Poller sub-API — gated: compiled only when CONFIG_NFC_ROLE_READER ---- */
#if defined(CONFIG_NFC_ROLE_READER)

/* Upward events for the poller path. Registered via
 * nfc_transport_register_poller_callbacks() below. */
typedef struct {
    /* @nfc_work_q — tag entered the field and was identified. */
    void (*on_tag_detected)(nfc_tech_t tech, void *user_ctx);
    /* @nfc_work_q — tag left the field. */
    void (*on_tag_removed)(void *user_ctx);
} nfc_transport_poller_ops_t;

/* @caller_sync — begin polling for tags matching tech_mask.
 * Returns -ENOTSUP on backends that do not implement the reader role (NFCT). */
int nfc_transport_poll_start(nfc_tech_t tech_mask);

/* @caller_sync — stop polling.
 * Returns -ENOTSUP on NFCT backend. */
int nfc_transport_poll_stop(void);

/* @threadsafe — send tx_buf[0..txlen) to the active tag; receive reply into
 * rx_buf (capacity rxcap bytes); actual received byte count written to *rxlen.
 * Returns -ENOTSUP on NFCT backend. */
int nfc_transport_transceive(const uint8_t *tx_buf, size_t txlen,
                             uint8_t *rx_buf, size_t rxcap, size_t *rxlen);

/* @caller_sync — register upward poller event callbacks (Pattern A).
 * Must be called AFTER nfc_transport_init() and BEFORE nfc_transport_poll_start().
 * NULL ops clears (unregisters). RESERVED — not implemented on NFCT backend
 * (returns -ENOTSUP). The RFAL backend is the intended first implementer. */
int nfc_transport_register_poller_callbacks(const nfc_transport_poller_ops_t *ops,
                                            void *user_ctx);

#endif /* CONFIG_NFC_ROLE_READER */
```

**NFCT `-ENOTSUP` stubs** (in `nfc_transport_nrfx.c`; dead code in practice due to
BUILD_ASSERT in §1.7, but present for defensive compilation):

```c
#if defined(CONFIG_NFC_ROLE_READER)
int nfc_transport_poll_start(nfc_tech_t tech_mask)
{
    ARG_UNUSED(tech_mask);
    return -ENOTSUP;
}
int nfc_transport_poll_stop(void) { return -ENOTSUP; }
int nfc_transport_transceive(const uint8_t *tx_buf, size_t txlen,
                             uint8_t *rx_buf, size_t rxcap, size_t *rxlen)
{
    ARG_UNUSED(tx_buf); ARG_UNUSED(txlen);
    ARG_UNUSED(rx_buf); ARG_UNUSED(rxcap); ARG_UNUSED(rxlen);
    return -ENOTSUP;
}
int nfc_transport_register_poller_callbacks(const nfc_transport_poller_ops_t *ops,
                                            void *user_ctx)
{
    ARG_UNUSED(ops); ARG_UNUSED(user_ctx);
    return -ENOTSUP;
}
#endif /* CONFIG_NFC_ROLE_READER */
```

The RFAL transport already in-repo (`NfcTransportRfal::Execute()` /
`RfalNotifyCallback`) is the natural implementation vehicle; see the §6 event model
note and spec v3 §6 backend matrix.

---

## B. Devicetree & Kconfig for the HAL Layer (spec v3 §12)

> **Strategy locked in spec v3 §12.1.** This subsection records the Wave 1
> (NFCT) concrete binding and defers external-controller bindings to their waves.

### B.1 Kconfig vs Devicetree split for this layer

- **Kconfig** (this file, `src/nfc/hal/Kconfig`): backend choice (`NFC_HAL_BACKEND_*`),
  role symbols (`NFC_ROLE_CARD/READER`), work-queue sizes, buffer sizes.
- **Devicetree**: hardware topology — which peripheral, which bus, which pins/IRQ.
  The library code is board-independent; board overlays supply the hardware binding.

### B.2 NFCT backend binding (now)

The NFCT peripheral is on-die; no custom DT binding YAML needed. Antenna pin handling is **SoC-specific**:

- **nRF52840:** UICR-based antenna mux (`AIN[26/27]`); managed internally by nrfxlib. Board overlay:

```devicetree
/* prj.conf companion; board overlay enables NFCT (nRF52840) */
&nfct {
    status = "okay";
    /* NFC antenna pin mux (UICR AIN[26/27]) managed by nrfxlib — no irq-gpios. */
};
```

- **nRF54L15 DK (`nrf54l15dk/nrf54l15/cpuapp`):** DK provides NFC antenna; pin config comes from the SoC DTS and NCS v3.2.4 board files. No UICR step needed. Same `&nfct { status = "okay"; }` overlay pattern.
- **nRF54L15 product board (`sigmationbaord/nrf54l15/cpuapp`):** ⚠️ **Prerequisite:** no NFC antenna in the current board DTS. NFC bring-up on product board is blocked until antenna wiring is added. DK is the development target.

`NFC_HAL_BACKEND_NRFX` Kconfig symbol gates the `nfc_transport_nrfx.c` compilation.
`nfc_t4t_lib.h` is the only vendor header included in `nfc_transport_nrfx.c`.

### B.3 External controller bindings (DEFERRED)

Full DT binding YAML for ST25R3916 (`st,st25r3916`) and PN7160 (`nxp,pn7160`) is
**intentionally not authored here** — see spec v3 §12.3. The strategy (compatible
string, `reg`, `irq-gpios`, optional `reset-gpios`; `DEVICE_DT_INST_DEFINE` driver
pattern) is locked in spec v3 §12.3. Authors the binding when each backend is
implemented, using the NFCT wave as the prior-art reference.

---

### DECISION flags raised (for human review)
1. **Stats struct extended** beyond spec v1 §6.1 (spec v2 defers the field list to §1.3 here) to add `apdu_assembled_count`, `frag_dropped_no_buffer`, `apdu_oversized_count`, `apdu_dropped_no_consumer` (CONVENTIONS §6 mandates named drop counters). — §1.3
2. **Ops struct shape:** conventions form (no `user_ctx` member; passed separately) locked over the spec form (member). — §1.5
3. **No `latency_stats_t`** in HAL stats despite CONVENTIONS §6 wording — utility not present in repo; counters-only hot-path helper. — §1.3 / §6
4. **No shared `module_lifecycle.h`** (file absent in repo) — Pattern B implemented inline with `atomic_cas`. — §4.2
5. **`set_uid` semantics** = nrfxlib emulation toggle only (module stays STARTED, WQ not torn down); `-ENODEV` unless STARTED. — §3
6. **Oversized 6700 sent inline** from the t4t callback (intended reply context); **no response on pool exhaustion** (FWI-timeout consequence documented). — §4.4
7. **HAL hardcodes the 6700 bytes locally**, does not include framing `apdu_types.h` (preserves layering). — §1.6
8. **`nfc_work_q` priority 5 / stack 2048 B** first-cut, to be pinned from Thread Analyzer. — §1.7
9. **`nfc_uid_t` owned by the HAL header** (lowest layer touching NFCID1); `nfc_stack` includes it. — §1.1
