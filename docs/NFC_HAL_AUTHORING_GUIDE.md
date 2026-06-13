# NFC HAL Backend Authoring Guide

**Status:** Authoring guide (non-normative). The normative contract is
[`docs/superpowers/plans/wave1-hal.md`](superpowers/plans/wave1-hal.md) (LOCKED)
and the architecture-of-record
[`docs/nfc/archive/specs/2026-06-12-nfc-stack-architecture.md`](nfc/archive/specs/2026-06-12-nfc-stack-architecture.md)
(v3). Where this guide and the locked contract disagree, **the contract wins** —
open a spec change, do not diverge in code. Every API name used here is taken
verbatim from `nfc_transport.h` as locked in wave1 §1–§2 and §A. This guide
invents no new API; where a backend author needs something the contract does not
provide, it is listed in [§10 Open items for spec owners](#10-open-items-for-spec-owners).

**Audience:** an engineer holding a controller eval board (ST25R3916, PN7160, or
any future part) who must add a `nfc_transport` backend without reverse-engineering
the stack.

**Worked references used throughout:**
- NFCT (in-tree first slice): `src/nfc/hal/nfc_transport_nrfx.c` — native ISR backend.
- RFAL (aliro reference): `aliro/platform/nfc/nfc_transport_rfal.{h,cpp}` — worker-driven backend.
- Legacy callback shape: `src/main.c` (`nfc_t4t_lib` ISR callback pattern).

---

## Table of Contents

1. [What a HAL backend is](#1-what-a-hal-backend-is)
2. [The event-bridging requirement](#2-the-event-bridging-requirement)
3. [Capability declaration](#3-capability-declaration)
4. [Kconfig integration](#4-kconfig-integration)
5. [Devicetree integration](#5-devicetree-integration)
6. [Lifecycle + concurrency obligations](#6-lifecycle--concurrency-obligations)
7. [Reader (poller) sub-API](#7-reader-poller-sub-api)
8. [Porting checklist](#8-porting-checklist)
9. [Worked example index](#9-worked-example-index)
10. [Open items for spec owners](#10-open-items-for-spec-owners)

---

## 1. What a HAL backend is

`nfc_transport` is the single hardware-specific layer of the stack
(architecture §4.1). Everything above it — `framing` (APDU assembly), `aid_router`,
the protocol modules, and `nfc_stack` — is backend-neutral and never changes when a
new controller is added. A **backend** is one `.c` file (e.g.
`nfc_transport_nrfx.c`, future `nfc_transport_rfal.c`) that implements every
function declared in the **vendor-clean** header `nfc_transport.h`.

> **Vendor-clean header rule (wave1 §1, §2):** `nfc_transport.h` contains no
> vendor includes, types, or constants. Each limit the contract needs is a
> transport-owned `#define`; the backend maps it to the vendor value and
> `BUILD_ASSERT`s the equivalence. Example from the NFCT backend:
> `BUILD_ASSERT(NFC_TRANSPORT_MAX_RESPONSE_LEN == NFC_T4T_MAX_PAYLOAD_SIZE)`.

### 1.1 What the backend must provide (the listen sub-API, built now)

Implement the full public surface from wave1 §2. Card-mode-relevant functions:

| Function | Annotation | Backend obligation |
|---|---|---|
| `nfc_transport_init(cfg)` | `@caller_sync` | Claim UNINITIALIZED→INITIALIZED via CAS; `STATS_RESET` first after the guard; register the controller event callback; stage config. No threads, no field, no work. |
| `nfc_transport_register_callbacks(ops, user_ctx)` | `@caller_sync` | Store `s_ops`/`s_user_ctx`; NULL clears. Guard table `-ENODEV`/`-EBUSY`/`-EIO`. |
| `nfc_transport_start(uid)` | `@caller_sync` | Apply NFCID1; start dispatch; begin listening. INITIALIZED/STOPPED→STARTED. |
| `nfc_transport_stop()` | `@caller_sync` | Stop listening, drain + stop the work queue, discard in-flight assembly. STARTED→STOPPED (restartable). |
| `nfc_transport_shutdown()` | `@caller_sync` | Implicit stop if STARTED; release the controller; →UNINITIALIZED. Always succeeds. |
| `nfc_transport_set_uid(uid)` | `@threadsafe` | Live NFCID1 rotation while STARTED (card role). Serialized internally. |
| `nfc_transport_send_response(buf, len)` | `@threadsafe` | Hand a service-owned response PDU to the controller. **Borrowed** until the next transport event — do not copy, do not free. |
| `nfc_transport_submit_work(work)` | `@threadsafe` | Submit a caller-owned `k_work` to `nfc_work_q` (deferred-response path). |
| `nfc_transport_get_config()` | `@isr_safe` | Return `&s_config`, never NULL. |
| `nfc_transport_get_stats(out)` | `@threadsafe` | Copy-out under `s_stats_lock`; `-EINVAL` if `out == NULL`. |
| `nfc_transport_get_state()` | `@isr_safe` | `atomic_get(&s_state)`; never fails. |
| `nfc_transport_get_capabilities()` | `@isr_safe` | Return the backend's `static const nfc_transport_caps_t *`; never NULL. See §3. |

### 1.2 What the backend receives (framing ops callbacks)

The upward boundary is **Pattern A** (ops struct), per CONVENTIONS §4 and wave1 §1.5.
The struct carries function pointers only; `user_ctx` is a *separate* registration
argument stored alongside (`s_ops` + `s_user_ctx`):

```c
typedef struct {
    void (*on_field_on)(void *user_ctx);
    void (*on_field_off)(void *user_ctx);
    void (*on_apdu)(struct net_buf *apdu, void *user_ctx); /* callee OWNS the ref */
} nfc_transport_ops_t;
```

The backend **invokes** these on `nfc_work_q` (never the ISR). The `on_apdu`
callee owns the `net_buf` ref and unrefs it after dispatch — the backend does
**not** unref after invoking `on_apdu` (NETWORK_BUFFERS one-owner rule;
wave1 §1.5). Every invocation NULL-checks both the ops pointer and the member
(CALLBACK_REGISTRATION_GUIDE).

### 1.3 What the backend must never do

These are hard invariants. Violating any one is a layering or memory-safety bug
that is silent in testing and fatal under load:

- **Never call upward synchronously from ISR/driver-callback context.** Field and
  APDU events are re-dispatched on `nfc_work_q`; the ISR only allocates (FIXED
  pool, `K_NO_WAIT`), copies, and submits work. (CONVENTIONS §7; wave1 §4.4.)
- **Never allocate from a non-FIXED pool, or with any timeout other than
  `K_NO_WAIT`, in ISR/callback context.** Only `NET_BUF_POOL_FIXED_DEFINE` is
  ISR-safe (NETWORK_BUFFERS "Pool Types"). Pool exhaustion is *normal*: bump a
  named drop counter and return — never `__ASSERT`.
- **Never block in a callback or work handler** — no `k_sleep`, no mutex/sem with
  a non-zero timeout in the hot path (creed §8).
- **Never call a scheduler API while holding a `k_spinlock`** (`k_work_submit_to_queue`,
  `k_fifo_put` wakeups, `k_sem_give`). Do `STATS_*` first (lock released), then
  submit/put (creed §8).
- **Never leak vendor types/errors upward.** Map any nonzero vendor return to a
  standard errno (card path: nonzero → `-EIO` + `STATS_ERROR`). Vendor enums never
  cross `nfc_transport.h` (wave1 §5.1, CONVENTIONS §9).
- **Never `#include` a layer above the HAL.** The HAL is the bottom; the one status
  word it emits itself (`6700`) is hardcoded locally, not pulled from framing
  (wave1 §1.6).
- **Never expose a live pointer to `s_stats`** — getters copy-out (STATS_API_DESIGN).

---

## 2. The event-bridging requirement

The upward HAL contract is **event-driven for both roles** (architecture §4.1,
wave1 §6): the consumer registers callbacks and receives events; it never polls the
HAL. Each backend bridges its own driver model to this contract *internally* —
nothing above the HAL sees the difference. There are two canonical driver models.

### 2.1 Model A — native ISR (NFCT)

The controller raises a hardware interrupt; the vendor library delivers a callback
in ISR context. The backend assembles fragments in that context and re-dispatches
complete events on `nfc_work_q`.

```
  reader field/I-block        NFCT IRQ            nfc_work_q (thread)        consumer (framing)
        |                       |                       |                          |
        |--- RF field on ------>|                       |                          |
        |                  nfc_isr_cb(FIELD_ON)         |                          |
        |                       |-- k_work_submit ----->|                          |
        |                       |                  field_on_handler --> on_field_on(user_ctx)
        |                       |                       |                          |
        |--- C-APDU frag 1 ---->|                       |                          |
        |               nfc_isr_cb(DATA_IND, MORE=1)    |                          |
        |               net_buf_alloc(K_NO_WAIT)        |                          |
        |               net_buf_add_mem (copy now)      |                          |
        |--- C-APDU frag N ---->|                       |                          |
        |               nfc_isr_cb(DATA_IND, MORE=0)    |                          |
        |               k_fifo_put(apdu); k_work_submit |                          |
        |                       |--------------------->  apdu_handler              |
        |                       |                   k_fifo_get -> on_apdu(buf,ctx) -->|  (callee unrefs)
        |                       |                       |                  nfc_transport_send_response(buf,len)
        |<-- R-APDU ------------|<--- response_pdu_send -|                          |
```

Reference: `src/nfc/hal/nfc_transport_nrfx.c` (`nfc_isr_cb`, wave1 §4.4) and the
legacy `nfc_callback` in `src/main.c`. No extra worker thread exists between the
ISR and the `nfc_work_q` event boundary.

### 2.2 Model B — worker-driven (RFAL / ST25R3916)

The controller has no upward callback; the driver is *iterated* by a worker that
calls a `…Worker()` step function. State changes arrive via a driver notification
callback that runs in the worker's context. The backend translates each state
transition into the same HAL events and **still re-dispatches them on `nfc_work_q`**
before invoking the registered ops (so the upward boundary is identical to Model A).

```
  backend worker (k_work)        RFAL driver               nfc_work_q (thread)      consumer
        |                           |                            |                     |
   Execute():                       |                            |                     |
     rfalNfcWorker() --------------> step state machine          |                     |
     rfalNfcGetState()              |                            |                     |
     reschedule(interval) <---------+                            |                     |
        |                           |                            |                     |
        |   notifyCb(STATE_ACTIVATED) (worker ctx)               |                     |
        |   -> translate to "tag/field" event                    |                     |
        |   -> k_work_submit(nfc_work_q) -------------------->  handler --> on_* (user_ctx)
        |                           |                            |                     |
        |   notifyCb(DATAEXCHANGE_DONE): CaptureRxData()         |                     |
        |   -> copy rx into a FIXED net_buf (K_NO_WAIT)          |                     |
        |   -> k_fifo_put + k_work_submit -------------------->  apdu_handler --> on_apdu(buf,ctx)
```

Reference: `aliro/platform/nfc/nfc_transport_rfal.cpp`. Note the real-world
mechanics you will reuse:
- `Execute()` runs one `rfalNfcWorker()` iteration and **self-reschedules** a
  delayable work item at `CONFIG_RFAL_NFC_WORKER_INTERVAL_MS` while not idle
  (`ncs_pal_submit_nfc_work()` kickstarts it; the controller IRQ also pokes it).
- `RfalNotifyCallback(rfalNfcState)` is the single state-fan-out point — the
  natural place to emit HAL events.
- A guard (`atomic_get(&mStarted)`) prevents the boot IRQ from driving the worker
  before init — replicate this with the transport FSM (§6).

> **Bridging the aliro reference onto the locked contract:** the aliro class is a
> singleton with its own `Init/Start/Stop/Send/Terminate` and talks straight to
> `AliroStack`. A conforming backend must instead present the C functions from §1.1
> and raise the `nfc_transport_ops_t` (and, for reader, `nfc_transport_poller_ops_t`
> — §7) events. Keep the RFAL worker/notify mechanics; replace the direct
> `AliroStack::Instance()` calls with HAL event dispatch on `nfc_work_q`.

### 2.3 Contract guarantees each model must uphold

Regardless of model, the backend must honor these (wave1 §3, §6):

- **Half-duplex ISO-DEP, single outstanding exchange.** ISO 14443-4 is half-duplex:
  the reader cannot send a new C-APDU until it has the R-APDU for the previous one.
  Therefore no new APDU event can reach `nfc_work_q` while a response is pending,
  which is what makes deferred `send_response()` (e.g. Aliro crypto on
  `nfc_work_q`) legal. The backend must not deliver a second `on_apdu` before the
  prior response is consumed.
- **Field on/off (or tag detected/removed) eventing is mandatory and ordered.**
  `on_field_on` is delivered before any APDU of a session; `on_field_off` after.
  A field-off cancels any in-flight assembly and any deferred response window — the
  backend discards the partial buffer and a deferred work handler must verify the
  session is still live before sending.
- **Response borrow lifetime.** The buffer handed to `send_response()` is borrowed
  by the controller until the next transport event; the backend must not copy it,
  and the service keeps it valid until then.
- **Immediate inbound copy.** Vendor RX buffer lifetime is undocumented; copy on
  receipt (`net_buf_add_mem` / `memcpy`), never store the vendor pointer.

---

## 3. Capability declaration

A backend advertises exactly the roles and technologies its controller can
physically service (architecture §2, §4.1; wave1 §1.8). Nothing above the HAL
hard-codes a technology or role; the stack reads the descriptor at init.

```c
typedef uint8_t  nfc_role_t;     /* NFC_ROLE_CARD | NFC_ROLE_READER (bitmask) */
typedef uint32_t nfc_tech_t;     /* NFC_TECH_* bitmask, see wave1 §1.8 */

typedef struct {
    uint8_t  roles;        /* OR of NFC_ROLE_* bits */
    uint32_t technologies; /* OR of NFC_TECH_* bits */
} nfc_transport_caps_t;

/* @isr_safe — returns the backend's static descriptor; never NULL. */
const nfc_transport_caps_t *nfc_transport_get_capabilities(void);
```

**Rule: never advertise a technology or role you cannot fully service.** The
descriptor is a `static const` struct (not a runtime query) because the backend
choice is already a compile-time Kconfig decision and the BUILD_ASSERT (§4) enforces
consistency statically.

Worked declarations:

```c
/* NFCT — card-only Type-4A; cannot poll (nfc_transport_nrfx.c). */
static const nfc_transport_caps_t s_nrfx_caps = {
    .roles        = NFC_ROLE_CARD,
    .technologies = NFC_TECH_ISO_DEP_A,
};

/* ST25R3916/RFAL — illustrative; pin the exact tech set when implemented
 * (architecture §6 says "card + reader, multi-technology"). */
static const nfc_transport_caps_t s_rfal_caps = {
    .roles        = NFC_ROLE_CARD | NFC_ROLE_READER,
    .technologies = NFC_TECH_ISO_DEP_A | NFC_TECH_ISO14443_3A_RAW |
                    NFC_TECH_ISO_DEP_B | NFC_TECH_TYPE2 |
                    NFC_TECH_TYPE3_FELICA | NFC_TECH_ISO15693,
};
```

**How the stack uses caps at init:** the per-protocol registry assembles only the
listener/poller units that are both compiled in *and* whose technology the backend
advertises (architecture §5, "table-with-NULL"). An enabled role/technology the
backend does not advertise is reported as "unsupported here" rather than crashing.
The compile-time BUILD_ASSERT (§4) catches the gross case (e.g. `ROLE_READER` on a
card-only backend) before the registry is ever consulted.

---

## 4. Kconfig integration

Kconfig is **policy**; it selects the backend and the roles to build. The backend
choice lives in `src/nfc/hal/Kconfig` (wave1 §1.7). The vendor dependency lives on
the backend symbol, never on `NFC_STACK` — the stack stays backend-neutral.

### 4.1 Add a backend choice entry

```kconfig
choice NFC_HAL_BACKEND
    prompt "NFC HAL transport backend"
    default NFC_HAL_BACKEND_NRFX
    depends on NFC_STACK

config NFC_HAL_BACKEND_NRFX
    bool "nRF NFCT via nrfxlib nfc_t4t_lib"
    depends on NFC_T4T_NRFXLIB
    # card-only: does NOT select NFC_HAL_BACKEND_HAS_READER

config NFC_HAL_BACKEND_RFAL
    bool "ST25R3916 via ST RFAL"
    select NFC_HAL_BACKEND_HAS_READER       # reader-capable

config NFC_HAL_BACKEND_PN7160
    bool "NXP PN7160 (NCI)"
    select NFC_HAL_BACKEND_HAS_READER       # reader-capable
endchoice
```

A reader-capable backend `select`s the hidden bool `NFC_HAL_BACKEND_HAS_READER`.
The role symbols are defined at the stack level and consumed by the HAL
(wave1 §1.7):

```kconfig
config NFC_ROLE_CARD
    bool "Build the card/listen role"
    depends on NFC_STACK
    default y

config NFC_ROLE_READER
    bool "Build the reader/poll role"
    depends on NFC_STACK && NFC_HAL_BACKEND_HAS_READER
    default n
```

`NFC_ROLE_READER` cannot even be offered unless the chosen backend selected
`NFC_HAL_BACKEND_HAS_READER`.

### 4.2 BUILD_ASSERT guarding unsupported roles

Every backend `BUILD_ASSERT`s that the Kconfig-selected roles are a subset of its
advertised capabilities, with an explanatory message. Card-only backend:

```c
/* nfc_transport_nrfx.c */
BUILD_ASSERT(!IS_ENABLED(CONFIG_NFC_ROLE_READER),
             "CONFIG_NFC_ROLE_READER selected but NFC_HAL_BACKEND_NRFX does not "
             "support the reader role. Choose a reader-capable backend "
             "(NFC_HAL_BACKEND_RFAL or NFC_HAL_BACKEND_PN7160) or disable "
             "CONFIG_NFC_ROLE_READER.");
```

A reader-capable backend instead asserts the specific technologies it claims, e.g.:

```c
/* nfc_transport_rfal.c (illustrative) */
BUILD_ASSERT(!IS_ENABLED(CONFIG_NFC_ROLE_READER) ||
             (s_rfal_caps.roles & NFC_ROLE_READER) != 0U,
             "NFC_ROLE_READER requires the RFAL backend to advertise NFC_ROLE_READER.");
```

### 4.3 Where backend-private Kconfig lives

Backend-private tunables (poll cadence, discovery limits, log level) live in a
backend-private Kconfig sourced only when that backend is selected — **not** in the
vendor-clean public surface. The aliro RFAL backend already demonstrates this:
`aliro/platform/nfc/Kconfig` defines `RFAL_NFC_WORKER_INTERVAL_MS`,
`RFAL_DISCOVERY_DEV_LIMIT`, `RFAL_DISCOVERY_TOTAL_DURATION_MS`, compliance-mode
choice, bitrate, etc. A new backend mirrors this: gate the source with the backend
symbol so the options vanish for other backends.

### 4.4 CMake source selection

`src/nfc/hal/CMakeLists.txt` picks exactly one backend source plus the shared
shell file (wave1 task 1):

```cmake
if(CONFIG_NFC_STACK)
  target_sources(app PRIVATE nfc_transport_shell_cmds.c)
  if(CONFIG_NFC_HAL_BACKEND_NRFX)
    target_sources(app PRIVATE nfc_transport_nrfx.c)
  elseif(CONFIG_NFC_HAL_BACKEND_RFAL)
    target_sources(app PRIVATE nfc_transport_rfal.c)
  endif()
endif()
```

---

## 5. Devicetree integration

> **Note — strategy, not yet ratified in the locked contract.** The wave1 plan and
> the v3 architecture cover NFCT (on-die) only and say nothing about devicetree for
> external controllers. The strategy below is the recommended approach and is
> consistent with HWMv2 and Zephyr driver idioms, but it must be ratified by the
> spec owners before the second backend is implemented. Tracked in
> [§10](#10-open-items-for-spec-owners) (G6). The binding YAML here is **illustrative**;
> the final binding is locked when the backend is implemented.

**Principle: Kconfig = policy, DT = topology.** Kconfig says *which* backend and
*which roles*; devicetree says *where the hardware is* (bus, pins, IRQ). One board
overlay selects hardware; library code is unchanged.

### 5.1 On-die peripheral (NFCT)

NFCT has no bus and no external wiring beyond the antenna. The node is minimal —
the peripheral plus antenna pins — and there is no custom binding to author. The
backend talks to nrfxlib directly; DT only enables the on-die peripheral and the
antenna pin routing for the board.

### 5.2 External controller (ST25R3916 / PN7160)

An external controller is a **bus device**, so model the backend as a Zephyr device
driver consuming a custom binding via `DT_INST_*` / `DEVICE_DT_GET`:

- `compatible` string identifies the part.
- `reg` is the chip-select / I²C address on the SPI/I²C bus node.
- `irq-gpios` is the controller interrupt line (drives the worker — §2.2).
- optional `reset-gpios` / `enable-gpios` for hardware reset / power.

Illustrative SPI binding sketch for an ST25R3916:

```yaml
# dts/bindings/nfc/st,st25r3916.yaml  (ILLUSTRATIVE — final binding locked at impl time)
description: ST ST25R3916 NFC reader/card controller on SPI

compatible: "st,st25r3916"

include: [spi-device.yaml]

properties:
  irq-gpios:
    type: phandle-array
    required: true
    description: Controller interrupt output (active high), drives the worker.
  reset-gpios:
    type: phandle-array
    required: false
    description: Hardware reset, active low.
  enable-gpios:
    type: phandle-array
    required: false
    description: Power-enable / LDO control, active high.
```

Board overlay node (illustrative):

```dts
&spi1 {
    status = "okay";
    nfc_reader: st25r3916@0 {
        compatible = "st,st25r3916";
        reg = <0>;
        spi-max-frequency = <6000000>;
        irq-gpios   = <&gpio0 11 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpio0 12 GPIO_ACTIVE_LOW>;
    };
};
```

Backend driver skeleton consuming it (illustrative):

```c
#define DT_DRV_COMPAT st_st25r3916

static const struct gpio_dt_spec s_irq =
    GPIO_DT_SPEC_INST_GET(0, irq_gpios);
static const struct spi_dt_spec s_bus =
    SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);
```

With this split, supporting the same controller on a different board is a new
overlay only — the backend `.c` and the public header are untouched.

---

## 6. Lifecycle + concurrency obligations

### 6.1 Map the backend's internal states onto the transport FSM

The transport FSM is **Pattern B** (`atomic_t s_state`, read from ISR/WQ; CONVENTIONS
§2, creed §1). The states are fixed (wave1 §1.4): `UNINITIALIZED`, `INITIALIZED`,
`STARTED`, `STOPPED`, `ERROR`. Map your controller's lifecycle onto them:

| Transport FSM | NFCT backend | RFAL-class backend (illustrative) |
|---|---|---|
| `init()` → INITIALIZED | `nfc_t4t_setup(cb,NULL)`; stage FWI | bus/GPIO ready; `rfalNfcInitialize()`; `notifyCb` wired; `mStarted=false` |
| `start()` → STARTED | `parameter_set(NFCID1)`; start `nfc_work_q`; `nfc_t4t_emulation_start()` | start worker; `rfalNfcDiscover(cfg)`; `mStarted=true`; kickstart worker |
| `stop()` → STOPPED | emulation stop; cancel/drain/stop WQ | `rfalNfcDeactivate(IDLE)`; `mStarted=false`; stop worker; drain WQ |
| `shutdown()` → UNINITIALIZED | implicit stop; `nfc_t4t_done()` | implicit stop; release bus/GPIO/driver |
| restart-failure → ERROR | rotation restart failed | discovery re-arm failed; terminal until `shutdown()` |

Use **CAS** (`atomic_cas`) for init/shutdown claims — never `atomic_get`+`atomic_set`
(TOCTOU; creed Pattern B). `start()` is idempotent (STARTED→0 no-op); `stop()` and
`shutdown()` are idempotent.

### 6.2 What must be torn down in stop/shutdown

- Stop the controller listening/discovery **first** so no further driver callbacks
  fire (NFCT: `nfc_t4t_emulation_stop()` guarantees this before it returns; RFAL:
  `rfalNfcDeactivate` + clear `mStarted` so `Execute()` early-returns).
- Discard any in-flight assembly buffer under `irq_lock` (NFCT) or the worker guard
  (RFAL).
- Cancel work with the **sync** variant from thread context: `k_work_cancel_sync`
  (and `k_work_cancel_delayable_sync` for the RFAL delayable worker) so no handler
  runs against drained state (creed §8).
- Drain the APDU fifo (unref remaining buffers), then `k_work_queue_drain(...,true)`
  + `k_work_queue_stop`.
- `shutdown()` additionally releases the controller (`nfc_t4t_done()` / bus + GPIO)
  and **always returns 0** (a lib error sets `STATS_ERROR` but still lands
  UNINITIALIZED).

### 6.3 ISR-safety table for each op

| Function | Annotation | Allowed context / backing |
|---|---|---|
| `init`/`register_callbacks`/`start`/`stop`/`shutdown` | `@caller_sync` | thread only; caller serializes |
| `set_uid` | `@threadsafe` | thread only; serialized by `s_uid_mutex` (card role; also serialized against `stop()`) |
| `send_response` | `@threadsafe` | typically `nfc_work_q`; safe from any thread |
| `submit_work` | `@threadsafe` | any thread; `-ENODEV` unless STARTED |
| `get_config` | `@isr_safe` | returns file-static pointer |
| `get_state` | `@isr_safe` | `atomic_get` |
| `get_capabilities` | `@isr_safe` | returns `static const` pointer |
| `get_stats` | `@threadsafe` | copy-out under `s_stats_lock` |
| driver event callback (`nfc_isr_cb` / `RfalNotifyCallback`) | ISR/worker | only FIXED `K_NO_WAIT` alloc, immediate copy, `STATS_*` then submit |

`@isr_safe` requires reading only `atomic_t` or spinlock-protected state (creed §3) —
the annotation must be backed by real synchronization, not by "it happens to work."

### 6.4 Stats counters the backend must maintain

The card-mode `nfc_transport_stats_t` is locked (wave1 §1.3). Maintain every field
that applies, in particular the mandatory `error_count` / `last_error_code` (via
`STATS_ERROR` on every non-fatal failure) and **a named counter for every drop
path** (CONVENTIONS §6): `frag_dropped_no_buffer`, `apdu_oversized_count`,
`apdu_dropped_no_consumer`, plus `field_on_count`, `field_off_count`,
`fragment_rx_count`, `apdu_assembled_count`, `response_tx_count`,
`uid_rotation_count`. Route the fragment hot path through a single private helper
wrapping one `STATS_SCOPE` (STATS_API_DESIGN; compute any elapsed time *before* the
scope). Reader-role counters are **not yet in the locked struct** — see
[§10](#10-open-items-for-spec-owners) (G3).

---

## 7. Reader (poller) sub-API

> **Status:** designed seam, **not implemented in the first slice.** Gated entirely
> under `CONFIG_NFC_ROLE_READER` (wave1 §A). The NFCT backend ships `-ENOTSUP`
> stubs; the BUILD_ASSERT in §4 makes selecting the reader role on NFCT a compile
> error. The **ST25R3916/RFAL backend is the intended first implementer.** Do not
> wire reader implementation code until reader-capable hardware is in hand.

A reader-capable backend additionally implements the reserved poller sub-API,
declared in `nfc_transport.h` under `#if defined(CONFIG_NFC_ROLE_READER)`
(wave1 §A):

```c
typedef struct {
    void (*on_tag_detected)(nfc_tech_t tech, void *user_ctx); /* @nfc_work_q */
    void (*on_tag_removed)(void *user_ctx);                   /* @nfc_work_q */
} nfc_transport_poller_ops_t;

int nfc_transport_poll_start(nfc_tech_t tech_mask);   /* @caller_sync; -ENOTSUP on NFCT */
int nfc_transport_poll_stop(void);                    /* @caller_sync; -ENOTSUP on NFCT */
int nfc_transport_transceive(const uint8_t *tx_buf, size_t txlen,
                             uint8_t *rx_buf, size_t rxcap, size_t *rxlen);
                                                      /* @threadsafe; -ENOTSUP on NFCT */
```

This sub-API activates with `CONFIG_NFC_ROLE_READER`. Mapping onto the RFAL
reference: `poll_start(tech_mask)` configures `mNfcConfig.techs2Find` and calls
`rfalNfcDiscover()`; `RfalNotifyCallback(RFAL_NFC_STATE_ACTIVATED)` →
`on_tag_detected`; deactivation/`DESELECT` → `on_tag_removed`; `transceive` wraps
`rfalNfcDataExchangeStart` and the `DATAEXCHANGE_DONE` capture. Note two unresolved
tensions before implementing: the poller `…ops_t` has **no registration function**
in the locked header, and `transceive` is declared synchronous while RFAL's exchange
is worker-driven/async — both are flagged in [§10](#10-open-items-for-spec-owners)
(G1, G5).

NFCT stubs (present for defensive compilation; dead code due to the BUILD_ASSERT):

```c
#if defined(CONFIG_NFC_ROLE_READER)
int nfc_transport_poll_start(nfc_tech_t tech_mask) { ARG_UNUSED(tech_mask); return -ENOTSUP; }
int nfc_transport_poll_stop(void) { return -ENOTSUP; }
int nfc_transport_transceive(const uint8_t *tx_buf, size_t txlen,
                             uint8_t *rx_buf, size_t rxcap, size_t *rxlen)
{
    ARG_UNUSED(tx_buf); ARG_UNUSED(txlen);
    ARG_UNUSED(rx_buf); ARG_UNUSED(rxcap); ARG_UNUSED(rxlen);
    return -ENOTSUP;
}
#endif
```

---

## 8. Porting checklist

A literal sequence from "new controller on the bench" to "passes the transport
conformance expectations." Commit after each numbered group (wave1 §8 granularity).

1. **Datasheet → topology.** Identify bus (SPI/I²C), IRQ line, reset/enable pins,
   and the technologies the silicon supports. Decide the capability set (§3).
2. **Driver model.** Determine native-ISR (Model A) vs worker-driven (Model B)
   from the vendor driver (§2). For Model B, find the iterate step + notify
   callback (RFAL: `rfalNfcWorker` / `RfalNotifyCallback`).
3. **Kconfig choice + role flags (§4).** Add `NFC_HAL_BACKEND_<NAME>`;
   `select NFC_HAL_BACKEND_HAS_READER` iff reader-capable. Add backend-private
   Kconfig gated by the backend symbol.
4. **CMake (§4.4).** Add the backend source under its `CONFIG_` guard.
5. **Devicetree (§5).** For an external controller: author the binding YAML,
   add the board overlay node, wire `DT_INST`/`DEVICE_DT_GET` in the driver.
6. **Capability descriptor (§3).** Add `static const nfc_transport_caps_t` and
   `nfc_transport_get_capabilities()`. Advertise only what you can fully service.
7. **BUILD_ASSERT (§4.2).** Enforce enabled roles ⊆ advertised capabilities with
   an explanatory message.
8. **Vendor-clean limits.** For each transport `#define` (e.g.
   `NFC_TRANSPORT_MAX_RESPONSE_LEN`), `BUILD_ASSERT` equivalence to the vendor
   constant.
9. **Statics + state helpers.** `atomic_t s_state`; `s_config`/`s_stats`/
   `s_stats_lock`; `s_ops`/`s_user_ctx`; the FIXED `nfc_apdu_pool` + fifo + work
   items; `LOG_MODULE_REGISTER`. CAS-based `state_claim`/`state_set`/`state_get`.
10. **Getters.** `get_config`, `get_state`, `get_stats` (copy-out), `get_capabilities`.
11. **init / shutdown.** CAS claim; `STATS_RESET` first after the guard; init kernel
    objects + controller; rollback to UNINITIALIZED on any error (`STATS_ERROR`,
    `-EIO`). `shutdown` idempotent and always returns 0.
12. **register_callbacks.** Guard table `-ENODEV`/`-EBUSY`/`-EIO`; store/clear
    `s_ops`/`s_user_ctx`.
13. **Event bridge (§2).** ISR/worker → FIXED `K_NO_WAIT` alloc → immediate copy →
    fragment assembly → fifo + `k_work_submit`. WQ handlers NULL-check ops, invoke
    `on_field_*`/`on_apdu`, and (for `on_apdu`) transfer ref ownership to the callee.
14. **start / stop.** Apply UID/discovery config; start dispatch/worker; begin
    listening/polling; idempotent. Stop: stop controller first, discard assembly,
    `*_cancel_sync`, drain + stop WQ.
15. **set_uid (card role).** `-EINVAL`/`-ENODEV` guards; serialize via `s_uid_mutex`
    (also serialized against `stop()`); controller stop → set NFCID1 → restart;
    force ERROR on restart failure; `uid_rotation_count++`.
16. **send_response + submit_work.** Validate (`-EINVAL` on NULL/0/oversize,
    `-ENODEV` unless STARTED); hand the borrowed buffer to the controller; map
    nonzero → `-EIO` + `STATS_ERROR`; `response_tx_count++`. `submit_work` →
    `k_work_submit_to_queue(&s_work_q, work)`.
17. **Reader stubs or impl (§7).** Card-only backend: `-ENOTSUP` stubs under
    `CONFIG_NFC_ROLE_READER`. Reader backend: implement the poller sub-API (subject
    to the §10 open items).
18. **Shell.** Reuse `nfc_transport_shell_cmds.c` (`config`/`stats`/`state`);
    confirm every stats field and the `state` enum print with a `default` arm.
19. **MISRA + DEV-ZEP.** Fixed-width types with `U` suffix; all locals initialized;
    every `switch` has `default` (Rule 16.4); Boolean controlling expressions; all
    lib/kernel returns checked or `(void)`-cast (Rule 17.7); cite DEV-ZEP-005/007/
    008/009/013/016 as applicable. Run cppcheck MISRA with the project suppressions.
20. **Conformance / test expectations (wave1 §8 test plan).** The portable, pure
    helpers are ztest-covered on `native_sim`; vendor-coupled paths are on-target
    integration checks (closed vendor libs do not link on host). A backend must
    pass:
    - **UID length validation** — accept 4/7/10, reject 0/5/8/11.
    - **Errno mapping** — `0 → 0`; any nonzero vendor return → `-EIO`.
    - **Oversize decision** — boundary fits-exactly vs one-over; oversize →
      `6700` + `apdu_oversized_count`, no buffer leak.
    - **Pool exhaustion** — first-fragment alloc failure bumps
      `frag_dropped_no_buffer`, sends no response, never asserts.
    - **No-consumer drop** — APDU ready with `on_apdu` unregistered →
      `net_buf_unref` + `apdu_dropped_no_consumer`.
    - **Lifecycle** — idempotent `start`/`stop`/`shutdown`; restartable; `-EALREADY`
      on double-init; CAS guards; `STATS_RESET` does not wipe live stats on a
      rejected double-init.
    - **Header compiles** with `CONFIG_NFC_ROLE_READER=n` and the BUILD_ASSERT fires
      with `=y` on a card-only backend.
21. **Thread Analyzer.** Enable `CONFIG_THREAD_ANALYZER*`; record the `nfc_work_q`
    (and backend worker) stack peak in the HAL README before production (creed §11).

---

## 9. Worked example index

| Concern | NFCT (in-tree, first slice) | RFAL (aliro reference) |
|---|---|---|
| Backend source | `src/nfc/hal/nfc_transport_nrfx.c` (wave1) | `aliro/platform/nfc/nfc_transport_rfal.cpp` |
| Backend header | `src/nfc/hal/nfc_transport.h` (shared, vendor-clean) | `aliro/platform/nfc/nfc_transport_rfal.h` |
| Driver model | native ISR — `nfc_isr_cb` (wave1 §4.4); legacy `nfc_callback` in `src/main.c` | worker-driven — `Execute()` / `rfalNfcWorker()` |
| Event fan-out | per-event `switch` in ISR callback | `RfalNotifyCallback(rfalNfcState)` |
| Worker scheduling | none (ISR → `nfc_work_q`) | `K_WORK_DELAYABLE` self-reschedule at `RFAL_NFC_WORKER_INTERVAL_MS`; `ncs_pal_submit_nfc_work()` kickstart |
| RX capture | `net_buf_add_mem` in ISR | `CaptureRxData()` (memcpy from `mRxData`/`mRcvLen`) |
| Start/stop | `nfc_t4t_emulation_start/stop` | `rfalNfcDiscover` / `rfalNfcDeactivate` |
| Send | `nfc_t4t_response_pdu_send` | `rfalNfcDataExchangeStart` |
| Capability set | `{CARD, ISO_DEP_A}` | `{CARD|READER, multi-tech}` (pin at impl) |
| Backend-private Kconfig | `src/nfc/hal/Kconfig` (workq size/prio) | `aliro/platform/nfc/Kconfig` (discovery/worker tunables) |
| Lifecycle contract | wave1 §3 | architecture §6 row "ST25R3916" |

Governing docs every backend follows: `docs/API_DESIGN_CREED.md`,
`docs/CALLBACK_REGISTRATION_GUIDE.md`, `docs/STATS_API_DESIGN.md`,
`docs/NETWORK_BUFFERS.md`, `docs/STACK_SPEC.md`, `docs/NFC_STACK_CONVENTIONS.md`.

---

## 10. Open items for spec owners

These are gaps a backend author hits that the locked contract does not yet resolve.
Per the authoring rule, they are flagged here rather than invented in code.

- **G1 — Poller callback registration function is missing.** Wave1 §A defines
  `nfc_transport_poller_ops_t` and comments that it is "Registered via
  `nfc_transport_register_poller_callbacks()`," but **no such function is declared**
  in the header. A reader backend has no contract-blessed way to receive
  `on_tag_detected`/`on_tag_removed`. Spec owners must add the registration
  function (name, guard table, dispatch-thread doc) to `nfc_transport.h` §A.

- **G2 — Poller lifecycle vs the transport FSM is undefined.** The FSM
  (`init→start→stop→shutdown`) is specified for card emulation only. For a
  reader-only or worker-driven backend it is unspecified whether
  `nfc_transport_start()` begins discovery or only readies the worker, and how
  `nfc_transport_poll_start/stop` relate to STARTED/STOPPED. Needs a locked mapping.

- **G3 — No reader-role counters in `nfc_transport_stats_t`.** The struct is locked
  card-only. CONVENTIONS §6 requires a named counter for every drop/error path, but
  there are no fields for tag-detected, transceive attempts/failures, or
  reader-side drops. Spec owners must extend the stats contract for the reader role.

- **G4 — Worker-thread provisioning for worker-driven backends is not in the
  contract.** Only `nfc_work_q` (the upward dispatch queue, `NFC_HAL_WORKQ_*`) is
  defined. A Model-B backend needs its own iterate worker (RFAL uses a delayable
  work item at a backend interval). Whether it reuses `nfc_work_q` or owns a
  separate thread, and the Kconfig for it, should be locked centrally rather than
  re-decided per backend.

- **G5 — `nfc_transport_transceive()` is synchronous but the reference exchange is
  async.** The declared signature returns `*rxlen` synchronously, while the RFAL
  model performs the exchange across worker iterations (`DATAEXCHANGE` →
  `DATAEXCHANGE_DONE`). Either the contract must permit a blocking implementation
  (with a bounded timeout — not currently a parameter), or the poller ops need an
  async receive event. Resolve before the reader role is implemented.

- **G6 — Devicetree binding strategy is not ratified.** Neither wave1 nor the v3
  architecture covers DT for external controllers. The §5 strategy (custom binding +
  board overlay + `DT_INST` driver) and the ST25R3916 binding sketch are proposals.
  Spec owners should ratify the binding conventions and lock the per-controller
  bindings when each backend lands.

- **G7 — Reader-role configuration knobs are absent from `nfc_transport_config_t`.**
  The config struct carries only `fwi` (card-mode ISO-DEP). A reader backend has no
  contract field for default poll cadence, technology mask, or discovery duration;
  today these would leak into backend-private Kconfig with no portable surface.
  Decide whether reader config belongs in the shared `config_t` or stays
  backend-private.
