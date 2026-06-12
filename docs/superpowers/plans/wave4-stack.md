# Wave 4: Stack (nfc_stack) Implementation Plan

**Status:** LOCKED — 2026-06-12 (harmonized v2)

**Layer:** nfc_stack (orchestrator)
**Files produced:**
- `src/nfc/nfc_stack.h`
- `src/nfc/nfc_stack.c`
- `src/nfc/nfc_stack_shell_cmds.c`
- `src/nfc/CMakeLists.txt` (top-level NFC CMake, includes all sub-layer directories)
- `src/nfc/Kconfig` (NFC_STACK parent + shared pool symbols + service symbols + NFC_STORE)

**Depends on:**
- `docs/superpowers/plans/wave1-hal.md` (LOCKED — 2026-06-12)
- `docs/superpowers/plans/wave2-framing.md` (LOCKED — 2026-06-12)
- `docs/superpowers/plans/wave3-router.md` (LOCKED — 2026-06-12)

**Sources consulted (Step 1 context sweep):**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 lifecycle/state patterns, §3 coupling map + wiring rule, §4 callback patterns, §5 buffer model, §6 stats recipe, §7 threading, §8 profile switching, §9 errno, §10 shell, §11 MISRA, §12 compliance checklist)
2. `docs/superpowers/plans/wave1-hal.md` — LOCKED; ground truth for `nfc_transport_*` API surface, `nfc_uid_t`, `nfc_work_q` ownership, `nfc_transport_ops_t` (no `user_ctx` member), `nfc_transport_register_callbacks(ops, user_ctx)`, `set_uid` semantics
3. `docs/superpowers/plans/wave2-framing.md` — LOCKED; ground truth for `apdu_assembler_init/shutdown`, `apdu_assembler_get_ops()`, DECISION-7 resolution (parsed dispatch form), Wave 2 wiring expectations in §5.3
4. `docs/superpowers/plans/wave3-router.md` — LOCKED; ground truth for `aid_router_init/shutdown/register/clear/dispatch/field_off`, `service.h` (`nfc_service_t` vtable with persistence hooks, `NFC_AID_MAX_LEN=16`), DECISION-J (profile-switch mechanism deferred to Wave 4), AID table sizing
5. `docs/NFC_WAVE_PLANNING_GUIDE.md` — per-wave process (8 steps) + exact template
6. `docs/superpowers/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) — §3 (AID table per service), §7 (public stack API + profile enum + event callback + save/load + main.c usage), §8 (thread model), §9 (Kconfig sourcing + layout), §6.4 (service AIDs — all five services), §6.5 (store API form + save/load scope and `-EBUSY` gating); retained as card-mode/NFCT implementation detail
7. `docs/superpowers/specs/2026-06-12-nfc-stack-architecture.md` (v3) — **architecture-of-record**; §4.4 (card-role orchestrator / reader-role sibling seam), §5 (Kconfig orthogonality — role symbols vs. service symbols), §9 (mapping to wave plans)
8. `docs/API_DESIGN_CREED.md` — full lifecycle contract, Pattern A/B state, FSM, shell, coupling, error handling
9. `docs/CALLBACK_REGISTRATION_GUIDE.md` — `_fn`/`_cb`, `user_ctx`, guard/NULL-clear
10. `docs/STATS_API_DESIGN.md` — `s_stats`/`s_stats_lock` recipe, `STATS_*`, copy-out getter
11. `docs/STACK_SPEC.md` — downward=direct call, upward=A/B/C/D; wiring in `nfc_stack.c`; no layer includes another solely for callbacks
12. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_RESET`, `STATS_COPY_OUT` macro implementations
13. `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001 through DEV-ZEP-018
14. `/Users/majidfaroud/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012 rules

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (replaced prototype; patterns must not carry forward).

---

> **Architecture Context (spec v3 reframe — additive):**
> This plan is the **NFCT / card-first-slice** implementation of
> [architecture spec v3](../specs/2026-06-12-nfc-stack-architecture.md).
> All existing lifecycle, profile, and card-mode contracts below are **unchanged**.
>
> **Role scope (spec §4.4):** `nfc_stack` is the **CARD-role orchestrator** — it
> drives HAL listen events through the ISO-DEP lane (framing + router) to protocol
> listeners. The **READER role** is a sibling engine (reserved seam; spec §4.4)
> that shares the same protocol modules and card file format but drives the HAL
> poller sub-API. No changes to `nfc_stack` are required to accommodate it.
>
> **Kconfig orthogonality (spec §5):** `NFC_SERVICE_<X>` symbols enable protocol
> modules (data model + listener + poller, each compiled under its role guard).
> `CONFIG_NFC_ROLE_CARD` / `CONFIG_NFC_ROLE_READER` are **orthogonal** role Kconfig
> defined in Wave 1. Existing `NFC_SERVICE_*` symbol names are unchanged.

---

## 1. Types and Constants

All public types live in `nfc_stack.h`. Internal-only types live in `nfc_stack.c`.

### 1.1 Profile Enum

```c
/**
 * @brief Active NFC emulation profile — controls which services are registered
 *        in the AID router.
 *
 * Only profiles whose required Kconfig symbol is enabled are valid at runtime.
 * nfc_stack_set_profile() returns -ENOTSUP for a profile whose required service
 * is not compiled in. NFC_PROFILE_ALL is always valid (contains only enabled
 * services).
 *
 * Ultralight has no ISO-DEP AID; NFC_PROFILE_ULTRALIGHT registers the NDEF AID
 * and relies on ndef_service being configured with Ultralight data. This
 * distinction is a service-level concern — to be confirmed by Wave 5.
 */
typedef enum {
    NFC_PROFILE_NDEF        = 0,  /**< NDEF T4T; AID D2 76 00 00 85 01 01        */
    NFC_PROFILE_DESFIRE     = 1,  /**< MF DeSFire EV1/EV2; AID D2 76 00 00 85 01 00 */
    NFC_PROFILE_ULTRALIGHT  = 2,  /**< Ultralight NDEF wrapper; registers NDEF AID;
                                   *   content injected by the ultralight adapter
                                   *   service (wave5-ultralight)                 */
    NFC_PROFILE_EMV         = 3,  /**< EMV: PPSE AID (32 50 41 59 2E 53 59 53 2E 44 44 46 30 31)
                                   *   + the payment app AID exported by the EMV service — §1.5 */
    NFC_PROFILE_ALIRO       = 4,  /**< Aliro credential; two AIDs — see §1.5      */
    NFC_PROFILE_ALL         = 5,  /**< Every compiled-in service; profile switch
                                   *   always valid                               */
    NFC_PROFILE_COUNT_,           /**< Sentinel — do not use                      */
} nfc_profile_t;

/** Value used to initialise s_pending_profile when no switch is pending.
 *  Stored in an atomic_t; cast to/from nfc_profile_t with range-check. */
#define NFC_PROFILE_NONE  ((atomic_val_t)NFC_PROFILE_COUNT_)
```

> **DECISION-2 (default profile):** The default active profile on reboot is `CONFIG_NFC_STACK_DEFAULT_PROFILE` (Kconfig int, default `NFC_PROFILE_NDEF`). NDEF is chosen as the most broadly compatible and simplest service. The user can change the default at build time. During `nfc_stack_init()`, the default profile is registered in the router. `s_pending_profile` is initialized to `NFC_PROFILE_NONE` (no switch pending).

### 1.2 `nfc_stack_config_t` (frozen after `init()`)

```c
typedef struct {
    /**
     * Initial active profile. Registered in the router during init().
     * Default: CONFIG_NFC_STACK_DEFAULT_PROFILE.
     * NULL cfg → built-in default.
     */
    nfc_profile_t default_profile;
} nfc_stack_config_t;

#define NFC_STACK_CONFIG_DEFAULT \
    ((nfc_stack_config_t){ .default_profile = (nfc_profile_t)CONFIG_NFC_STACK_DEFAULT_PROFILE })
```

### 1.3 `nfc_stack_stats_t`

```c
typedef struct {
    uint32_t profile_switch_count;   /**< Pending profiles applied on field-off       */
    uint32_t uid_rotation_count;     /**< nfc_stack_set_uid() succeeded               */
    uint32_t save_count;             /**< nfc_stack_save() succeeded                  */
    uint32_t load_count;             /**< nfc_stack_load() succeeded                  */
    uint32_t profile_notsup_count;   /**< nfc_stack_set_profile() returned -ENOTSUP   */
    uint32_t apdu_drop_count;        /**< APDUs unref'd in stack_on_apdu because the
                                      *   framing ops were unavailable (CONVENTIONS §6:
                                      *   every silent-drop path has a named counter)  */
    uint32_t error_count;            /**< Mandatory — STATS_ERROR increments this      */
    int32_t  last_error_code;        /**< Mandatory — last code passed to STATS_ERROR  */
} nfc_stack_stats_t;
```

### 1.4 `nfc_stack_state_t` — Pattern A (plain enum, @caller_sync lifecycle)

```c
typedef enum {
    NFC_STACK_STATE_UNINITIALIZED = 0,  /**< Zero-init default; before init()        */
    NFC_STACK_STATE_INITIALIZED,        /**< After init(); ready to start             */
    NFC_STACK_STATE_STARTED,            /**< Field emulation active                   */
    NFC_STACK_STATE_STOPPED,            /**< Emulation stopped; restartable           */
    NFC_STACK_STATE_ERROR,              /**< Terminal; requires shutdown() + re-init  */
} nfc_stack_state_t;
```

> **Pattern A rationale:** The nfc_stack lifecycle is `@caller_sync`. No ISR reads `s_state` — only lifecycle functions on the caller thread and the application inspect it. Pattern B (`atomic_t`) is not required. The pending profile uses a separate `atomic_t` because `nfc_stack_set_profile()` is `@threadsafe` and `nfc_stack_apply_pending_profile()` reads it from `nfc_work_q`.

### 1.5 Per-Profile AID Registration Tables (internal — `nfc_stack.c`)

Static `const` AID byte arrays, declared at file scope (not inside functions — MISRA Dir 4.9 / creed §9 no static locals):

```c
/* All AIDs from spec v3 §4.2 (ISO-DEP lane) / spec v2 §3 / wave3-router.md §6 */
static const uint8_t k_ndef_aid[]    = { 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x01U };
static const uint8_t k_desfire_aid[] = { 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U };
static const uint8_t k_emv_ppse_aid[] = {
    0x32U, 0x50U, 0x41U, 0x59U, 0x2EU, 0x53U, 0x59U,
    0x53U, 0x2EU, 0x44U, 0x44U, 0x46U, 0x30U, 0x31U
};
/* The EMV payment application AID is owned by the EMV service and obtained at
 * registration time via emv_service_get_app_aid()/emv_service_get_app_aid_len()
 * (wave5-emv) — both the PPSE AID and the app AID are registered for
 * NFC_PROFILE_EMV (the reader SELECTs the app AID after reading the PPSE
 * directory; an unregistered app AID would 6A82 at the router). */
static const uint8_t k_aliro_expedited_aid[] = {
    0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x01U, 0x00U
};
static const uint8_t k_aliro_stepup_aid[] = {
    0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x02U, 0x00U
};
```

`k_ndef_aid` is also used for `NFC_PROFILE_ULTRALIGHT` registration — confirmed by spec v2 §6.4.3 ("Ultralight service registers no AID") and by wave5-ultralight (the ultralight service is a persistence-only adapter that injects its rendered NDEF message via `ndef_service_set_content()`).

### 1.6 Kconfig Symbol Set — Full Reconciled Tree for `src/nfc/`

**`src/nfc/Kconfig`** (this wave's deliverable — parent symbol + shared pool symbols + all service/store symbols):

**Sourcing (spec v2 §9):** the application's top-level `Kconfig` `rsource`s `src/nfc/Kconfig`; `src/nfc/Kconfig` in turn `rsource`s every per-layer Kconfig. The top-level `CMakeLists.txt` does `add_subdirectory(src/nfc)`; `src/nfc/CMakeLists.txt` adds each layer subdirectory (§5.5). `NFC_STACK` is backend-neutral — the nrfxlib dependency lives on the HAL backend choice (`NFC_HAL_BACKEND_NRFX` depends on `NFC_T4T_NRFXLIB`, `src/nfc/hal/Kconfig` — wave1-hal §1.7), not here.

```kconfig
config NFC_STACK
    bool "NFC emulation stack (hal/framing/router/services)"
    help
      Layered NFC card emulation stack. Provides ISO-DEP card emulation
      for NDEF, DeSFire, Ultralight, EMV, and Aliro profiles on nRF54L15 (primary target) / nRF52840 (secondary).
      The transport backend (and its vendor dependency) is selected by the
      NFC_HAL_BACKEND choice in hal/Kconfig.

# ─── Per-layer Kconfigs (explicit rsource — spec v2 §9) ──────────────────────

rsource "hal/Kconfig"
rsource "framing/Kconfig"
rsource "router/Kconfig"
# rsource "store/Kconfig"       (added when the store layer lands — Wave 6)
# rsource "services/*/Kconfig"  (added when each service lands — Wave 5)

# ─── Shared pool symbols (defined here; consumed by hal/Kconfig) ────────────

config NFC_APDU_BUF_SIZE
    int "Inbound APDU net_buf data size in bytes"
    default 512
    range 64 4096
    depends on NFC_STACK
    help
      Size of each buffer in the FIXED inbound APDU pool. An assembled
      APDU whose length exceeds this value is rejected with SW 6700
      (Wrong Length) and dropped in the HAL ISR. Default 512 is
      sufficient for all supported protocols including extended APDUs
      used by DeSFire and Aliro.

config NFC_APDU_POOL_COUNT
    int "Inbound APDU net_buf pool count"
    default 4
    range 2 16
    depends on NFC_STACK
    help
      Number of buffers in the FIXED inbound APDU pool. Pool exhaustion
      causes the in-flight APDU chain to be silently dropped (dropped
      stat incremented). Default 4 provides 4 concurrent assembly slots;
      in practice at most 1 is needed (single reader session) — headroom
      is for burst DATA_IND events before nfc_work_q drains the fifo.

# ─── Default profile ─────────────────────────────────────────────────────────

config NFC_STACK_DEFAULT_PROFILE
    int "Default active profile on boot (NFC_PROFILE_* enum value)"
    default 0
    range 0 5
    depends on NFC_STACK
    help
      Integer value of nfc_profile_t registered during nfc_stack_init().
      0=NDEF, 1=DESFIRE, 2=ULTRALIGHT, 3=EMV, 4=ALIRO, 5=ALL.
      The selected profile must have its NFC_SERVICE_* symbol enabled.

# ─── Service symbols ──────────────────────────────────────────────────────────

config NFC_SERVICE_NDEF
    bool "NDEF T4T service"
    depends on NFC_STACK
    help
      NFC Forum T4T NDEF file system emulation (CC + NDEF file, read/write).

config NFC_SERVICE_DESFIRE
    bool "MF DeSFire EV1/EV2 service"
    depends on NFC_STACK
    # NOTE: PSA_WANT_ALG_AES does not exist; use KEY_TYPE + cipher-mode selects.
    # Verify these symbol names against NCS v3.2.4 at implementation.
    select PSA_WANT_KEY_TYPE_AES
    select PSA_WANT_ALG_CBC_NO_PADDING
    select PSA_WANT_ALG_ECB_NO_PADDING
    select PSA_WANT_ALG_CMAC
    help
      MF DeSFire EV1/EV2 card emulation with AES authentication.

config NFC_SERVICE_ULTRALIGHT
    bool "MF Ultralight NDEF wrapper service"
    depends on NFC_STACK && NFC_SERVICE_NDEF
    help
      Presents Ultralight page data as NDEF content via the NDEF service.
      Registers no separate AID — uses the NDEF AID.

config NFC_SERVICE_EMV
    bool "EMV PPSE service"
    depends on NFC_STACK
    help
      EMV PPSE directory emulation (payment card detection).

config NFC_SERVICE_ALIRO
    bool "Aliro credential service"
    depends on NFC_STACK
    select PSA_WANT_ALG_ECDH
    select PSA_WANT_ALG_ECDSA
    select PSA_WANT_ALG_SHA_256
    select PSA_WANT_ALG_GCM
    select PSA_WANT_ALG_HKDF
    select PSA_WANT_KEY_TYPE_ECC_KEY_PAIR
    help
      Aliro 0.9.4 credential (card-side) emulation. Registers two AIDs
      (expedited + step-up phases).

# ─── Store symbol ─────────────────────────────────────────────────────────────

config NFC_STORE
    bool "Card save/load stub seam"
    depends on NFC_STACK
    default y
    help
      Serialize/deserialize the active card profile via registered save/load
      callbacks. Default stubs dump to shell (save) and read from a
      compiled-in header (load). A real NVS/LittleFS backend can be added
      by registering a different callback.

# ─── NDEF buffer size (service-specific; kept in top-level for discoverability)

config NFC_NDEF_MAX_SIZE
    int "NDEF file buffer size in bytes"
    default 256
    range 16 4096
    depends on NFC_SERVICE_NDEF
    help
      Maximum size of the NDEF data file buffer. Also bounds the NDEF TLV
      payload that can be stored and served to a reader.
```

**Already introduced by sub-layers (sub-`Kconfig` files, not re-declared here):**

| Symbol | Defined in |
|---|---|
| `NFC_HAL_WORKQ_STACK_SIZE` (default 2048) | `src/nfc/hal/Kconfig` (Wave 1) |
| `NFC_HAL_WORKQ_PRIORITY` (default 5) | `src/nfc/hal/Kconfig` (Wave 1) |
| `NFC_APDU_EXTENDED_SUPPORT` (default y) | `src/nfc/framing/Kconfig` (Wave 2) |
| `NFC_ROUTER_MAX_AIDS` (default 8) | `src/nfc/router/Kconfig` (Wave 3) |

> **DECISION-5 (Kconfig placement for NFC_APDU_BUF_SIZE / NFC_APDU_POOL_COUNT):** Per wave1-hal §1.7: "Consumed here [HAL], defined at the stack level per spec v2 §9." These shared-pool symbols belong in `src/nfc/Kconfig`, not in `hal/Kconfig`, to reflect that they are stack-wide contracts. The HAL `nfc_apdu_pool` definition uses these via `CONFIG_NFC_APDU_BUF_SIZE` / `CONFIG_NFC_APDU_POOL_COUNT`. `src/nfc/Kconfig` `rsource`s every per-layer Kconfig explicitly (spec v2 §9), and the application's top-level Kconfig `rsource`s `src/nfc/Kconfig` — no `depends on` duplication needed. Note `NFC_STACK` itself carries no nrfxlib dependency; `NFC_T4T_NRFXLIB` is depended on by the `NFC_HAL_BACKEND_NRFX` backend choice in `hal/Kconfig` (wave1-hal §1.7).

### 1.7 Application Event Vocabulary and Callback Typedef

The application needs a field-off trigger so it can rotate the UID per field-off
(preserving the prior main.c behavior where the UID changes every time the reader
field drops) — see DECISION-4 and spec v2 §7.

```c
/**
 * @brief Events delivered to the application-registered callback.
 *
 * Deliberately minimal — this enum exists to give the application a UID-rotation
 * trigger, not to mirror the full internal event flow. Extend only with proven
 * need.
 */
typedef enum {
    NFC_STACK_EVENT_FIELD_ON  = 0,  /**< Reader field detected                    */
    NFC_STACK_EVENT_FIELD_OFF = 1,  /**< Reader field removed. Fired AFTER session
                                     *   cleanup and AFTER any pending profile has
                                     *   been applied (see §6 DECISION-1 chain).   */
} nfc_stack_event_t;

/**
 * @brief Application event callback (CONVENTIONS §4: typedef suffix _fn).
 *
 * Dispatch thread: nfc_work_q — ALWAYS. Must not block, sleep, or perform
 * dynamic allocation. Bounded execution required (CONVENTIONS §7).
 *
 * Calling nfc_stack_set_uid() from this callback is EXPLICITLY SUPPORTED
 * (the canonical per-field-off rotation pattern) — see §6 deadlock analysis.
 *
 * @param event    The event that occurred.
 * @param user_ctx Context pointer passed at registration (CONVENTIONS §4 naming).
 */
typedef void (*nfc_stack_event_cb_fn)(nfc_stack_event_t event, void *user_ctx);
```

---

## 2. Public API

All declarations in `nfc_stack.h`. Memory comment at top:

```c
/*
 * Memory: nfc_stack_config_t ~4 B  (file-static, frozen after init)
 *         nfc_stack_stats_t  ~32 B (file-static, spinlock-guarded)
 *         s_state            ~4 B  (plain enum, Pattern A)
 *         s_active_profile   ~4 B  (nfc_profile_t, WQ-only after start)
 *         s_pending_profile  ~4 B  (atomic_t, @threadsafe writes)
 *         s_transport_ops    ~12 B (file-static const, wrapper for framing)
 *         s_event_cb + s_event_user_ctx ~8 B (application event callback slot)
 *         s_stats_lock       ~0-4 B (struct k_spinlock)
 * No additional threads or pools — these are owned by the HAL.
 * Lifecycle: init / start / stop / shutdown (full, Pattern A).
 */
```

```c
#include "hal/nfc_transport.h"    /* nfc_uid_t */

/* ── Lifecycle ──────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize all NFC stack layers and register the default profile.
 *
 * Initializes: framing, router, store (Wave 6), transport. Registers the
 * wrapper transport ops (nfc_stack's s_transport_ops) with the HAL.
 * Initializes all compiled-in service modules. Registers the default profile's
 * AIDs with the router. cfg == NULL → NFC_STACK_CONFIG_DEFAULT.
 *
 * STATS_RESET is the first statement after the -EALREADY lifecycle guard
 * (CONVENTIONS §6). No thread is started; no emulation begins.
 *
 * @param cfg  Config; NULL → built-in defaults.
 * @retval  0         UNINITIALIZED → INITIALIZED.
 * @retval -EALREADY  Already INITIALIZED.
 * @retval -EIO       A lower-layer init or HAL ops registration failed;
 *                    rollback to UNINITIALIZED. Exact layer logged.
 * @retval -EINVAL    Default profile's required service not enabled (Kconfig).
 * @caller_sync
 */
int nfc_stack_init(const nfc_stack_config_t *cfg);

/**
 * @brief Start NFC field emulation with the given initial UID.
 *
 * Delegates to nfc_transport_start(uid). The active profile (registered during
 * init or last applied on field-off) is already in the router.
 *
 * @param uid  Initial NFCID1. NULL → library default NFCID1.
 * @retval  0         INITIALIZED|STOPPED → STARTED, or already STARTED
 *                    (idempotent; returns 0 per creed §1).
 * @retval -ENODEV    State == UNINITIALIZED or ERROR.
 * @retval -EINVAL    uid != NULL and uid->len not in {4, 7, 10}.
 * @retval -EIO       nfc_transport_start() failed.
 * @caller_sync
 */
int nfc_stack_start(const nfc_uid_t *uid);

/**
 * @brief Stop NFC field emulation.
 *
 * Delegates to nfc_transport_stop(). Drains nfc_work_q; no further callbacks
 * fire after this returns. Restartable via nfc_stack_start(). Idempotent.
 *
 * @retval  0       STARTED → STOPPED (or already INITIALIZED/STOPPED; no-op).
 * @retval -ENODEV  State == UNINITIALIZED.
 * @retval -EIO     nfc_transport_stop() failed.
 * @caller_sync
 */
int nfc_stack_stop(void);

/**
 * @brief Tear down the entire stack.
 *
 * Idempotent from any state. Performs implicit stop() if STARTED. Shuts down
 * all lower layers (transport, store, router, framing) and all services.
 * State → UNINITIALIZED. Always succeeds (errors logged, not returned — shutdown
 * must not fail per creed §1).
 *
 * @retval 0  Always.
 * @caller_sync
 */
int nfc_stack_shutdown(void);

/* ── Application event callback (spec v2 §7) ─────────────────────────────────── */

/**
 * @brief Register the application event callback.
 *
 * Register AFTER nfc_stack_init() and BEFORE nfc_stack_start()
 * (CALLBACK_REGISTRATION_GUIDE guard table). cb == NULL clears the
 * registration (both cb and user_ctx slots).
 *
 * Dispatch thread for the callback: nfc_work_q. The callback must not block
 * or sleep. Calling nfc_stack_set_uid() from the callback is supported.
 *
 * @param cb        Callback to register, or NULL to clear.
 * @param user_ctx  Opaque context passed unchanged to cb. Ignored when cb == NULL.
 * @retval  0       Registered (or cleared).
 * @retval -ENODEV  State == UNINITIALIZED.
 * @retval -EBUSY   State == STARTED (registration only when not running).
 * @retval -EIO     State == ERROR.
 * @caller_sync — register after init(), before start()
 */
int nfc_stack_register_event_cb(nfc_stack_event_cb_fn cb, void *user_ctx);

/* ── UID rotation ────────────────────────────────────────────────────────────── */

/**
 * @brief Rotate the NFC UID while emulation is running.
 *
 * Maps directly to nfc_transport_set_uid(uid): the transport briefly stops
 * emulation, sets the NFCID1, and restarts (backend-internal). The module
 * stays STARTED; the work queue is not torn down. Application-driven — the
 * canonical rotation trigger is the NFC_STACK_EVENT_FIELD_OFF event delivered
 * to the registered event callback; calling this function from inside that
 * callback (on nfc_work_q) is explicitly supported. See DECISION-4 and §6
 * deadlock analysis.
 *
 * @param uid  New NFCID1. Must not be NULL; len must be in {4, 7, 10}.
 * @retval  0       Success; uid_rotation_count++.
 * @retval -EINVAL  uid is NULL or uid->len invalid.
 * @retval -ENODEV  Stack not STARTED.
 * @retval -EIO     nfc_transport_set_uid() failed.
 * @threadsafe
 */
int nfc_stack_set_uid(const nfc_uid_t *uid);

/* ── Profile switching ──────────────────────────────────────────────────────── */

/**
 * @brief Request a profile switch (applied on the next field-off event).
 *
 * Sets an atomic_t pending profile and returns immediately. The switch is
 * applied on the next field-off on nfc_work_q: aid_router_clear() followed by
 * re-registration of the new profile's AIDs. In-memory only — does not persist
 * across reboots.
 *
 * @param profile  Target profile. Must be a valid nfc_profile_t value.
 * @retval  0         Pending profile set.
 * @retval -EINVAL    profile is out of range (>= NFC_PROFILE_COUNT_).
 * @retval -ENOTSUP   Profile's required Kconfig not enabled (service not compiled in).
 * @threadsafe
 */
int nfc_stack_set_profile(nfc_profile_t profile);

/**
 * @brief Return the currently active profile (applied in the router).
 *
 * Reads s_active_profile — only valid after init(). Returns NFC_PROFILE_COUNT_
 * (sentinel) if called before init().
 *
 * @caller_sync  (Pattern A plain enum; not atomic)
 */
nfc_profile_t nfc_stack_get_active_profile(void);

/* ── Store operations (Wave 6 — to be confirmed) ─────────────────────────────── */

/**
 * @brief Save the active profile's card data via the store layer.
 *
 * Gathers the active profile's services, calls nfc_store_save(tag, svcs, n).
 * The save callback (default: shell hex dump) is called by the store layer.
 *
 * Scope (spec v2 §6.5): load (provisioning from defaults/shell) is the primary
 * store path; save is a debug/capture facility for reader-written state.
 * Refused while STARTED so persistence hooks never run concurrently with
 * nfc_work_q dispatch — services need no internal locking for serialize().
 *
 * @param tag  Identifier string for the saved blob.
 * @retval  0       Success; save_count++.
 * @retval -EINVAL  tag is NULL.
 * @retval -ENODEV  Stack not INITIALIZED or above.
 * @retval -EBUSY   State == STARTED (stop emulation before saving; spec v2 §6.5).
 * @retval -EIO     nfc_store_save() failed.
 * @caller_sync
 * @note to be confirmed by Wave 6
 */
int nfc_stack_save(const char *tag);

/**
 * @brief Load card data into the active profile's services via the store layer.
 *
 * Calls nfc_store_load(tag, svcs, n). The load callback (default: compiled-in
 * header) feeds the store; each service's deserialize() is called.
 *
 * Scope (spec v2 §6.5): this is the primary store path — provisioning service
 * state from defaults or the shell before start. Refused while STARTED so
 * persistence hooks never run concurrently with nfc_work_q dispatch.
 *
 * @param tag  Identifier string for the blob to load.
 * @retval  0       Success; load_count++.
 * @retval -EINVAL  tag is NULL.
 * @retval -ENODEV  Stack not INITIALIZED or above.
 * @retval -EBUSY   State == STARTED (stop emulation before loading; spec v2 §6.5).
 * @retval -EIO     nfc_store_load() failed.
 * @caller_sync
 * @note to be confirmed by Wave 6
 */
int nfc_stack_load(const char *tag);

/* ── Module contract getters (CONVENTIONS §2) ────────────────────────────────── */

/**
 * @retval Non-NULL always (file-static backing; safe before init()).
 * @isr_safe
 */
const nfc_stack_config_t *nfc_stack_get_config(void);

/**
 * @param out  Destination; must not be NULL.
 * @retval  0       Success (copy-out under spinlock).
 * @retval -EINVAL  out is NULL.
 * @threadsafe
 */
int nfc_stack_get_stats(nfc_stack_stats_t *out);

/**
 * @retval nfc_stack_state_t — never fails. Pattern A plain enum.
 * @caller_sync
 */
nfc_stack_state_t nfc_stack_get_state(void);
```

---

## 3. Contracts

### `nfc_stack_init(cfg)` — Full Sequence and Rollback Table

**Pre:** `s_state == NFC_STACK_STATE_UNINITIALIZED`. `@caller_sync`.

**Sequence (in strict order):**

```
1.  if (s_state != UNINITIALIZED) → return -EALREADY  [before STATS_RESET]
2.  STATS_RESET(s_stats)                               [first statement after EALREADY guard]
3.  s_config = (cfg != NULL) ? *cfg : NFC_STACK_CONFIG_DEFAULT
4.  Validate default profile: if required Kconfig not enabled → return -EINVAL
5.  s_state = NFC_STACK_STATE_INITIALIZED              [claim early so rollback unwinds correctly]
6.  s_pending_profile = NFC_PROFILE_NONE               [no pending switch]
7.  s_active_profile  = s_config.default_profile       [set active]

Layer init (strict order — each failure rolls back all prior):

8.  apdu_assembler_init(NULL)
    → on fail: s_state = UNINIT; return -EIO
9.  aid_router_init()
    → on fail: apdu_assembler_shutdown(); s_state = UNINIT; return -EIO
10. nfc_store_init(NULL)                               [Wave 6 — to be confirmed]
    → on fail: aid_router_shutdown(); apdu_assembler_shutdown(); s_state = UNINIT; return -EIO
11. nfc_transport_init(NULL)
    → on fail: nfc_store_shutdown(); aid_router_shutdown(); apdu_assembler_shutdown();
               s_state = UNINIT; return -EIO

Service init (all compiled-in services, regardless of active profile):
   — see DECISION-3 —

12. IS_ENABLED(CONFIG_NFC_SERVICE_NDEF):      ndef_service_init(NULL)
    → on fail: goto err_svc_init  [unwind services already inited + layers + state UNINIT]
13. IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE):   desfire_service_init(NULL)
    → on fail: goto err_svc_init
14. IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT): ultralight_service_init(NULL)
    → on fail: goto err_svc_init
15. IS_ENABLED(CONFIG_NFC_SERVICE_EMV):       emv_service_init(NULL)
    → on fail: goto err_svc_init
16. IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO):     aliro_service_init(NULL)
    → on fail: goto err_svc_init

Wiring (after ALL inits, before start — CONVENTIONS §3):

17. nfc_transport_register_callbacks(&s_transport_ops, NULL)
    → on fail: goto err_svc_init [all services + layers already up; shut everything down]

Default profile registration (after wiring, before start):

18. stack_register_profile(s_active_profile)
    → calls aid_router_register() for each service in the profile
    → on fail (e.g. -ENOMEM if NFC_ROUTER_MAX_AIDS too small):
       goto err_svc_init

19. return 0   [state remains INITIALIZED]
```

**Rollback label `err_svc_init`** (unwinds in reverse init order):
```c
err_svc_init:
    /* Services first, in reverse of init order — only those already inited
     * (matches §6 "Service Init Order and Rollback"): */
    if (IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO)      && aliro_inited)      aliro_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_EMV)        && emv_inited)        emv_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT) && ultralight_inited) ultralight_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)    && desfire_inited)    desfire_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)       && ndef_inited)       ndef_service_shutdown();
    /* Then layers, in reverse of init order: */
    nfc_transport_shutdown();    /* safe: backend shutdown idempotent */
    nfc_store_shutdown();        /* Wave 6 */
    aid_router_shutdown();
    apdu_assembler_shutdown();
    s_state = NFC_STACK_STATE_UNINITIALIZED;
    STATS_ERROR(&s_stats_lock, s_stats, rc);
    return -EIO;
```

> **Tracking which services are inited:** Use local `bool` flags (`ndef_inited`, `desfire_inited`, etc.) in `nfc_stack_init()`. Each is set to `true` immediately after a successful service init call. The rollback path uses these flags to know which shutdowns to call. Do NOT use static state for this — these are transient init-phase locals.

**Post (0):** All layers initialized; framing ops wired to HAL (`s_transport_ops` registered); default profile's AIDs registered in router; all compiled-in services inited; state == INITIALIZED. No emulation running.

**Errors:**
- `-EALREADY` — state not UNINITIALIZED.
- `-EINVAL` — default profile's Kconfig not enabled.
- `-EIO` — any lower-layer init or wiring failure; full rollback executed; state == UNINITIALIZED.

---

### `nfc_stack_start(uid)` — Sequence

**Pre:** `s_state ∈ {INITIALIZED, STOPPED}`.

```
1. if (s_state == STARTED) → return 0   [idempotent per creed §1]
2. if (s_state == UNINITIALIZED || ERROR) → return -ENODEV
3. if (uid != NULL && uid->len ∉ {4,7,10}) → return -EINVAL
4. s_state = NFC_STACK_STATE_STARTED
5. rc = nfc_transport_start(uid)
   → on fail: s_state = NFC_STACK_STATE_ERROR; STATS_ERROR(&s_stats_lock, s_stats, rc);
              return -EIO
6. return 0
```

**Post (0):** `nfc_transport_start()` succeeded; ISR callbacks now flow; framing, router, services receive events on nfc_work_q. State == STARTED.

**Errors:** `-ENODEV`, `-EINVAL`, `-EIO`. Already-STARTED returns 0 (idempotent per creed §1).

---

### `nfc_stack_stop(void)` — Sequence

**Pre:** any state ≥ INITIALIZED.

```
1. if (s_state == UNINITIALIZED) → return -ENODEV
2. if (s_state ∈ {INITIALIZED, STOPPED}) → return 0   [idempotent]
3. rc = nfc_transport_stop()
   → nfc_transport_stop() drains nfc_work_q — ALL pending callbacks complete before
      this function returns. This includes any pending profile switch submitted
      during the last field-off. After this returns, no WQ work is in flight.
   → on fail: STATS_ERROR(&s_stats_lock, s_stats, rc); s_state = ERROR; return -EIO
4. s_state = NFC_STACK_STATE_STOPPED
5. return 0
```

**Post (0):** No more callbacks; nfc_work_q drained and stopped; state == STOPPED. `start()` may be called again.

---

### `nfc_stack_shutdown(void)` — Sequence

**Pre:** any state. Idempotent.

```
1. if (s_state == UNINITIALIZED) → return 0   [no-op]
2. if (s_state == STARTED):
   rc = nfc_transport_stop()   [implicit stop; errors logged, not returned]
   if (rc != 0) STATS_ERROR(&s_stats_lock, s_stats, rc); LOG_ERR(...)
3. nfc_transport_shutdown()    [always called; idempotent in HAL]
4. nfc_store_shutdown()        [Wave 6]
5. aid_router_shutdown()
6. apdu_assembler_shutdown()
7. Shutdown all compiled-in services in reverse init order:
   IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO):     aliro_service_shutdown()   
   IS_ENABLED(CONFIG_NFC_SERVICE_EMV):       emv_service_shutdown()     
   IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT): ultralight_service_shutdown()
   IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE):   desfire_service_shutdown() 
   IS_ENABLED(CONFIG_NFC_SERVICE_NDEF):      ndef_service_shutdown()    
8. s_event_cb = NULL; s_event_user_ctx = NULL   [clear app callback registration]
9. s_state = NFC_STACK_STATE_UNINITIALIZED
10. return 0   [always; shutdown must not fail per creed §1]
```

> **Shutdown error policy:** Sub-layer shutdown errors are logged via `LOG_ERR` and recorded via `STATS_ERROR` but do NOT cause the function to return an error. All shutdown steps are always attempted in sequence regardless of earlier failures. State lands UNINITIALIZED regardless.

---

### `nfc_stack_register_event_cb(cb, user_ctx)` — Contract

**Pre:** `s_state ∈ {INITIALIZED, STOPPED}` (registration only when not running). `@caller_sync`.

**Post (0):**
- `s_event_cb = cb`, `s_event_user_ctx = user_ctx`.
- `cb == NULL` clears **both** slots (unregister); `user_ctx` argument ignored in that case.
- No callback is invoked from within this function.

**Errors (CALLBACK_REGISTRATION_GUIDE guard table):**
- `-ENODEV` — `s_state == UNINITIALIZED`.
- `-EBUSY` — `s_state == STARTED`.
- `-EIO` — `s_state == ERROR`.

**Callback invocation guarantees (once registered and stack STARTED):**
1. All invocations occur on `nfc_work_q` — never the ISR, never the caller thread.
2. `NFC_STACK_EVENT_FIELD_ON` fires from `stack_on_field_on` AFTER framing's
   `on_field_on` chain has completed.
3. `NFC_STACK_EVENT_FIELD_OFF` fires from `stack_on_field_off` AFTER (a) framing's
   field-off chain (`aid_router_field_off()` → service `on_field_off` → active
   service cleared) AND (b) `nfc_stack_apply_pending_profile()` — so the
   application observes the post-cleanup, post-profile-switch state. Exact order
   in the §6 DECISION-1 chain diagram.
4. The invoker NULL-checks `s_event_cb` before every call (CONVENTIONS §4).
5. After `nfc_transport_stop()` returns (via `nfc_stack_stop/shutdown`), no
   further invocations occur (WQ drained and stopped — wave1-hal §3 stop).
6. No invocation is ever dropped silently while STARTED: the event fires on
   every field transition delivered by the HAL.

**Callback-side rules:** must not block, sleep, or allocate; bounded execution
(it runs on the same WQ that serves APDU dispatch). Calling
`nfc_stack_set_uid()` from the callback is explicitly supported (§6 deadlock
analysis). Calling lifecycle functions (`stop`/`shutdown`) from the callback is
NOT supported (they wait on the WQ the callback occupies — self-deadlock).

---

### `nfc_stack_set_uid(uid)` — Sequence

```
1. if (uid == NULL || uid->len ∉ {4,7,10}) → return -EINVAL
2. if (s_state != STARTED) → return -ENODEV
3. rc = nfc_transport_set_uid(uid)
4. if (rc != 0): STATS_ERROR; return -EIO
5. STATS_INC(uid_rotation_count); return 0
```

> **DECISION-4 (UID rotation and application responsibility):** `nfc_stack_set_uid()` is a thin delegation to `nfc_transport_set_uid()`; nfc_stack performs NO autonomous per-field-off rotation. The field-off notification the application needs is provided by the application event callback: `nfc_stack_register_event_cb(cb, user_ctx)` delivers `NFC_STACK_EVENT_FIELD_ON` / `NFC_STACK_EVENT_FIELD_OFF` on `nfc_work_q`. The canonical pattern — preserving the prior main.c behavior where the UID changes on every field drop — is to call `nfc_stack_set_uid()` from the callback on `NFC_STACK_EVENT_FIELD_OFF`. This matches spec v2 §7: "on field-off event: profile applied, call nfc_stack_set_uid() to rotate" (the event fires after the pending profile is applied). Deadlock-freedom of set_uid-from-callback is verified in §6.

---

### `nfc_stack_set_profile(profile)` — Sequence

```
1. if (profile >= NFC_PROFILE_COUNT_) → return -EINVAL
2. Check Kconfig guard (IS_ENABLED) for the requested profile:
   NDEF: IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)             → -ENOTSUP if not
   DESFIRE: IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)        → -ENOTSUP if not
   ULTRALIGHT: IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT)  → -ENOTSUP if not
   EMV: IS_ENABLED(CONFIG_NFC_SERVICE_EMV)                → -ENOTSUP if not
   ALIRO: IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO)            → -ENOTSUP if not
   ALL: always valid (contains only enabled services — cannot fail -ENOTSUP)
   → on -ENOTSUP: STATS_INC(profile_notsup_count); return -ENOTSUP
3. atomic_set(&s_pending_profile, (atomic_val_t)profile)
4. return 0
```

**@threadsafe** — the atomic store is the only write. No lock on `s_active_profile` (it is only read/written on nfc_work_q via `nfc_stack_apply_pending_profile()`).

> Returns 0 immediately even if no field-off has yet occurred. The actual switch (clear + re-register) happens when the NFC field goes away.

---

### `nfc_stack_save(tag)` / `nfc_stack_load(tag)` — Sequence

```c
/* save: */
1. if (tag == NULL) → return -EINVAL
2. if (s_state == UNINITIALIZED) → return -ENODEV
3. if (s_state == STARTED) → return -EBUSY    [spec v2 §6.5 — never while running]
4. Build service pointer array svcs[] + count n for s_active_profile
5. rc = nfc_store_save(tag, svcs, n)         [locked: wave6-store.md §2]
6. if (rc != 0): STATS_ERROR; return -EIO
7. STATS_INC(save_count); return 0

/* load: */
(symmetric; uses nfc_store_load)
```

**Scope (spec v2 §6.5):** load — provisioning service state from defaults or the shell — is the primary store path; save is a debug/capture facility for reader-written state. Both return `-EBUSY` while STARTED, so persistence hooks are never invoked concurrently with `nfc_work_q` dispatch and services need no internal locking between their data model and `serialize`/`deserialize` (wave3-router §1.1).

Both functions gather only the active profile's services. Because the `-EBUSY` guard means they only run while the stack is not STARTED, the caller-thread read of `s_active_profile` cannot race a WQ profile switch (the WQ is quiesced). For PROFILE_ALL, all compiled-in services are included in the array. For the ULTRALIGHT profile, the array contains only the ultralight service instance (persist_id 0x03; the NDEF service holds a derived view and is not separately persisted — wave5-ultralight DECISION-UL-9).

---

### Getters

- `get_config` — `&s_config`; never NULL; valid before `init()`. `@isr_safe`.
- `get_stats` — `STATS_COPY_OUT(&s_stats_lock, s_stats, out)`; `-EINVAL` if NULL; 0 on success. `@threadsafe`.
- `get_state` — plain `return s_state`; never fails. `@caller_sync`.
- `get_active_profile` — plain `return s_active_profile`; never fails; returns sentinel (`NFC_PROFILE_COUNT_`) before init. `@caller_sync`.

---

## 4. Internal State

### 4.1 File-Static State (`nfc_stack.c`)

```c
/* Lifecycle — Pattern A (plain enum; lifecycle @caller_sync only) */
static nfc_stack_state_t s_state = NFC_STACK_STATE_UNINITIALIZED;

/* Required module statics */
static nfc_stack_config_t s_config = NFC_STACK_CONFIG_DEFAULT;
static nfc_stack_stats_t  s_stats;
static struct k_spinlock  s_stats_lock;   /* NOT K_SPINLOCK_DEFINE (NCS v3.2.4) */

/* Profile state */
static atomic_t           s_pending_profile;  /* set @threadsafe; read on nfc_work_q */
static nfc_profile_t      s_active_profile = NFC_PROFILE_COUNT_;  /* sentinel until init */

/* Application event callback (spec v2 §7; CONVENTIONS §4 storage form).
 * Written only by register_event_cb (caller thread, not STARTED) and shutdown;
 * read only by the wrapper handlers on nfc_work_q while STARTED. */
static nfc_stack_event_cb_fn s_event_cb       = NULL;
static void                 *s_event_user_ctx = NULL;

/* ── nfc_stack wrapper ops (the heart of DECISION-1) ─────────────────────────
 * Registered with the HAL instead of apdu_assembler_get_ops().
 * All wiring happens here, in nfc_stack.c. Never in any sub-layer.         */
static void stack_on_field_on(void *user_ctx);
static void stack_on_field_off(void *user_ctx);
static void stack_on_apdu(struct net_buf *apdu, void *user_ctx);

static const nfc_transport_ops_t s_transport_ops = {
    .on_field_on  = stack_on_field_on,
    .on_field_off = stack_on_field_off,
    .on_apdu      = stack_on_apdu,
};
```

### 4.2 Lifecycle State Machine (Pattern A)

| From | Event | To | Action |
|---|---|---|---|
| UNINITIALIZED | `init()` | INITIALIZED | STATS_RESET; init layers; wire ops; register default profile |
| INITIALIZED | `init()` | INITIALIZED | return `-EALREADY` |
| INITIALIZED \| STOPPED | `start(uid)` | STARTED | `nfc_transport_start(uid)` |
| STARTED | `start()` | STARTED | idempotent; return 0 |
| STARTED | `stop()` | STOPPED | `nfc_transport_stop()` (drains WQ) |
| INITIALIZED \| STOPPED | `stop()` | (unchanged) | idempotent; return 0 |
| any | `shutdown()` | UNINITIALIZED | implicit stop if STARTED; shutdown all layers + services |
| STARTED (transport fails) | — | ERROR | terminal until `shutdown()` |
| STARTED (field-off, WQ) | — | STARTED | apply pending profile (clear + re-register) |

### 4.3 Pending-Profile State Machine (atomic_t)

| Event | State change | Thread |
|---|---|---|
| `nfc_stack_init()` | `s_pending_profile = NFC_PROFILE_NONE` | Caller |
| `nfc_stack_set_profile(p)` | `atomic_set(&s_pending_profile, p)` | Any (`@threadsafe`) |
| `nfc_stack_apply_pending_profile()` at field-off | `atomic_set(&s_pending_profile, NFC_PROFILE_NONE)` | `nfc_work_q` |

### 4.4 Internal: `nfc_stack_apply_pending_profile()` (on `nfc_work_q`)

This function is called from `stack_on_field_off()` on the `nfc_work_q` thread, after framing's field-off handler (which calls `aid_router_field_off()`) has completed:

```c
static void nfc_stack_apply_pending_profile(void)
{
    atomic_val_t pending = atomic_get(&s_pending_profile);
    if (pending == NFC_PROFILE_NONE) {
        return;  /* no switch requested */
    }
    /* Guard against late callbacks during shutdown */
    if (s_state != NFC_STACK_STATE_STARTED) {
        return;
    }
    atomic_set(&s_pending_profile, NFC_PROFILE_NONE);  /* consume */
    aid_router_clear();  /* clears registration table; s_active_svc already NULL from field_off */
    int rc = stack_register_profile((nfc_profile_t)pending);
    if (rc == 0) {
        s_active_profile = (nfc_profile_t)pending;
        STATS_INC(&s_stats_lock, s_stats, profile_switch_count);
        LOG_INF("Profile switched to %d", (int)s_active_profile);
    } else {
        /* Re-register the OLD profile as a fallback to avoid empty table */
        (void)stack_register_profile(s_active_profile);
        STATS_ERROR(&s_stats_lock, s_stats, rc);
        LOG_ERR("Profile switch to %d failed: %d; kept profile %d",
                (int)pending, rc, (int)s_active_profile);
    }
}
```

> **On profile registration failure:** If `aid_router_register()` returns `-ENOMEM` (table too small for the new profile), nfc_stack re-registers the old profile as a safety net. This avoids the stack entering a state with an empty AID table while STARTED. The error is counted and logged. Flag for human review: if the old profile's re-registration also fails, the table may be partially filled — this is an extremely unlikely misconfiguration; log `LOG_ERR` and continue (the reader will receive 6A82 for all SELECTs until the next field-off + retry).

### 4.5 `stack_register_profile(profile)` (internal)

```c
static int stack_register_profile(nfc_profile_t profile)
{
    int rc = 0;
    switch (profile) {
    case NFC_PROFILE_NDEF:
        if (IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)) {
            rc = aid_router_register(k_ndef_aid, sizeof(k_ndef_aid),
                                     ndef_service_get());
        }
        break;
    case NFC_PROFILE_DESFIRE:
        if (IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)) {
            rc = aid_router_register(k_desfire_aid, sizeof(k_desfire_aid),
                                     desfire_service_get());
        }
        break;
    case NFC_PROFILE_ULTRALIGHT:
        /* Ultralight uses the NDEF AID; the ultralight service is a
         * persistence-only adapter (wave5-ultralight) — never registered */
        if (IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT) &&
            IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)) {
            rc = aid_router_register(k_ndef_aid, sizeof(k_ndef_aid),
                                     ndef_service_get());
        }
        break;
    case NFC_PROFILE_EMV:
        if (IS_ENABLED(CONFIG_NFC_SERVICE_EMV)) {
            rc = aid_router_register(k_emv_ppse_aid, sizeof(k_emv_ppse_aid),
                                     emv_service_get());
            if (rc == 0) {
                rc = aid_router_register(emv_service_get_app_aid(),
                                         emv_service_get_app_aid_len(),
                                         emv_service_get());
            }
        }
        break;
    case NFC_PROFILE_ALIRO:
        if (IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO)) {
            rc = aid_router_register(k_aliro_expedited_aid,
                                     sizeof(k_aliro_expedited_aid),
                                     aliro_service_get());
            if (rc == 0) {
                rc = aid_router_register(k_aliro_stepup_aid,
                                         sizeof(k_aliro_stepup_aid),
                                         aliro_service_get());
            }
        }
        break;
    case NFC_PROFILE_ALL:
        rc = stack_register_all_enabled();  /* registers every IS_ENABLED service */
        break;
    default:
        rc = -EINVAL;
        break;
    }
    return rc;
}
```

> **DECISION-6 (NFC_PROFILE_ULTRALIGHT router registration):** The Ultralight service has no ISO-DEP AID (spec v2 §6.4.3). In the router, `NFC_PROFILE_ULTRALIGHT` registers the same NDEF AID (`D2 76 00 00 85 01 01`) using `ndef_service_get()`. The distinction between NDEF and Ultralight content is a service-level concern handled at Wave 5 — the NDEF service is configured with Ultralight page data, not a standalone NDEF payload. This plan uses `ndef_service_get()` for both NDEF and ULTRALIGHT profiles — confirmed by wave5-ultralight: the ultralight service owns the page data model and injects its rendered NDEF message via `ndef_service_set_content()`; it is never router-registered.

### 4.6 Wrapper Ops Handler Implementations (on `nfc_work_q`)

```c
static void stack_on_field_on(void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    const nfc_transport_ops_t *ops = apdu_assembler_get_ops();
    if ((ops != NULL) && (ops->on_field_on != NULL)) {
        ops->on_field_on(NULL);   /* framing's handler */
    }
    /* Fire app event LAST (invoker NULL-checks per CONVENTIONS §4) */
    if (s_event_cb != NULL) {
        s_event_cb(NFC_STACK_EVENT_FIELD_ON, s_event_user_ctx);
    }
}

static void stack_on_field_off(void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    const nfc_transport_ops_t *ops = apdu_assembler_get_ops();
    if ((ops != NULL) && (ops->on_field_off != NULL)) {
        ops->on_field_off(NULL);  /* → aid_router_field_off() inside */
    }
    /* Apply pending profile AFTER framing's field-off completes */
    nfc_stack_apply_pending_profile();
    /* Fire app event LAST — app sees post-cleanup, post-profile-switch state.
     * Calling nfc_stack_set_uid() from this callback is supported (§6). */
    if (s_event_cb != NULL) {
        s_event_cb(NFC_STACK_EVENT_FIELD_OFF, s_event_user_ctx);
    }
}

static void stack_on_apdu(struct net_buf *apdu, void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    const nfc_transport_ops_t *ops = apdu_assembler_get_ops();
    if ((ops != NULL) && (ops->on_apdu != NULL)) {
        ops->on_apdu(apdu, NULL);  /* framing owns and unrefs apdu */
    } else {
        /* Framing ops unavailable (shutdown in progress): we still own the
         * buffer — unref it and count the drop (CONVENTIONS §6). No
         * double-unref risk: the branches are exclusive. */
        net_buf_unref(apdu);
        STATS_INC(&s_stats_lock, s_stats, apdu_drop_count);
    }
}
```

> **Ownership note for `stack_on_apdu`:** The HAL transfers `net_buf` ownership to the registered ops handler (`stack_on_apdu`). `stack_on_apdu` chains to `framing->on_apdu`, which takes ownership and unrefs. If `framing->on_apdu` is NULL (defensive shutdown path), `stack_on_apdu` must `net_buf_unref(apdu)` to avoid a leak. This is the only case where nfc_stack touches the `net_buf` directly.

### 4.7 Concurrency Table

| Variable | Written by | Read by | Synchronization |
|---|---|---|---|
| `s_state` | `init/start/stop/shutdown` (caller_sync) | `set_uid`, `save/load` (caller_sync); WQ guard in `apply_pending` | None needed (Pattern A; all writers/readers on caller thread or protected by nfc_stack usage contract) |
| `s_active_profile` | `init` (caller), `apply_pending_profile` (WQ) | `get_active_profile` (caller), `save/load` (caller) | No lock: WQ writes during field-off; `save/load` read only when NOT STARTED (`-EBUSY` guard, spec v2 §6.5 — WQ quiesced, no race). `get_active_profile` while STARTED may observe the pre-switch value during a concurrent field-off apply — informational only; treat as "last applied". See DECISION-9. |
| `s_pending_profile` | `set_profile` (any thread, `atomic_set`) | `apply_pending_profile` (WQ, `atomic_get`); `apply_pending_profile` (WQ, `atomic_set` to NONE) | `atomic_t` — lock-free |
| `s_stats.*` | lifecycle (caller), WQ (`apply_pending`, `set_uid` delegate) | `get_stats` (any) | `s_stats_lock` |
| `s_config` | `init()` (caller, once; frozen) | `get_config` (any) | None after init (frozen) |
| `s_transport_ops` | compile-time constant | HAL `nfc_transport_register_callbacks`; WQ dispatch | None (read-only) |
| `s_event_cb` / `s_event_user_ctx` | `register_event_cb` (caller, only when NOT STARTED — `-EBUSY` guard), `shutdown` (caller, after WQ stopped) | wrapper handlers (`nfc_work_q`, while STARTED) | No lock needed: writes occur only when the WQ is not running (same justification as Wave 1 `s_ops`/`s_user_ctx` row — "only set when not STARTED; only read when STARTED") |

---

## 5. Integration Points

### 5.1 Down — Every Wave 1/2/3 Function Consumed (Verbatim Signatures)

#### Wave 1 (HAL) — consumed by nfc_stack.c

```c
/* Lifecycle — called in init/start/stop/shutdown order */
int nfc_transport_init(const nfc_transport_config_t *cfg);            /* @caller_sync */
int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops,
                                     void *user_ctx);                 /* @caller_sync */
int nfc_transport_start(const nfc_uid_t *uid);                        /* @caller_sync */
int nfc_transport_stop(void);                                         /* @caller_sync */
int nfc_transport_shutdown(void);                                     /* @caller_sync */

/* UID rotation — delegated from nfc_stack_set_uid() */
int nfc_transport_set_uid(const nfc_uid_t *uid);                      /* @threadsafe */

/* Registration call in init: */
/*   nfc_transport_register_callbacks(&s_transport_ops, NULL); */
/*   NOT apdu_assembler_get_ops() — see DECISION-1              */
```

#### Wave 2 (Framing) — consumed by nfc_stack.c

```c
/* Lifecycle */
int apdu_assembler_init(const apdu_assembler_config_t *cfg);  /* @caller_sync; NULL → defaults */
int apdu_assembler_shutdown(void);                            /* @caller_sync; always 0 */

/* Ops getter — used to chain through s_transport_ops wrapper handlers */
const nfc_transport_ops_t *apdu_assembler_get_ops(void);      /* @isr_safe; never NULL */
```

`apdu_assembler_get_ops()` is called at RUNTIME from `stack_on_field_on/off/apdu` handlers (on `nfc_work_q`), not only during init. This is safe: the function returns a pointer to a compile-time constant ops struct (wave2-framing §4.1 `s_ops_impl`).

#### Wave 3 (Router) — consumed by nfc_stack.c

```c
/* Lifecycle */
int  aid_router_init(void);        /* @caller_sync */
int  aid_router_shutdown(void);    /* @caller_sync; always 0 */

/* Registration — called in stack_register_profile() during init and field-off */
int  aid_router_register(const uint8_t *aid, size_t aid_len,
                         const nfc_service_t *svc);   /* @caller_sync or @wq */

/* Profile switch — called from nfc_stack_apply_pending_profile() on nfc_work_q */
void aid_router_clear(void);       /* @wq (during profile switch at field-off) */
```

`aid_router_dispatch()` and `aid_router_field_off()` are **NOT** called by nfc_stack directly — they are called by framing's ops handlers (which nfc_stack chains through `s_transport_ops`).

### 5.2 Services — Expected Surface (to be confirmed by Wave 5)

Every compiled-in service is expected to expose:

```c
/* Minimal lifecycle — init/shutdown (CONVENTIONS §2 minimal).
 * init takes an optional config; NULL => built-in defaults. nfc_stack always
 * passes NULL — non-NULL config is an application/shell provisioning path. */
int <svc>_service_init(const <svc>_service_config_t *cfg); /* @caller_sync; 0/-errno */
int <svc>_service_shutdown(void);  /* @caller_sync; always 0; idempotent */

/* Vtable getter — returns pointer to file-static nfc_service_t */
const nfc_service_t *<svc>_service_get(void);  /* @isr_safe; never NULL; safe before init() */
```

Concretely:
```c
int ndef_service_init(const ndef_service_config_t *cfg);
int ndef_service_shutdown(void);      const nfc_service_t *ndef_service_get(void);

int desfire_service_init(const desfire_service_config_t *cfg);
int desfire_service_shutdown(void);   const nfc_service_t *desfire_service_get(void);

int ultralight_service_init(const ultralight_service_config_t *cfg);
int ultralight_service_shutdown(void); const nfc_service_t *ultralight_service_get(void);

int emv_service_init(const emv_service_config_t *cfg);
int emv_service_shutdown(void);       const nfc_service_t *emv_service_get(void);
/* EMV also exports its payment app AID for stack_register_profile (§4.5): */
const uint8_t *emv_service_get_app_aid(void);
size_t emv_service_get_app_aid_len(void);

int aliro_service_init(const aliro_service_config_t *cfg);
int aliro_service_shutdown(void);     const nfc_service_t *aliro_service_get(void);
```

> **All service API signatures are marked "to be confirmed by Wave 5."** Wave 5 agents must produce exactly these symbols (or a superset) to remain compatible with nfc_stack. If Wave 5 chooses a different API shape, nfc_stack must be updated before locking.

### 5.3 Store — Expected Surface (to be confirmed by Wave 6)

```c
/* From spec v2 §6.5 — used by nfc_stack_save() / nfc_stack_load() */
int nfc_store_init(const nfc_store_config_t *cfg);    /* @caller_sync */
int nfc_store_shutdown(void);                         /* @caller_sync; always 0 */

/* nfc_stack calls these directly (downward direct call per CONVENTIONS §3) */
int nfc_store_save(const char *tag,
                   const nfc_service_t *const *svcs, size_t n); /* @caller_sync */
int nfc_store_load(const char *tag,
                   const nfc_service_t *const *svcs, size_t n); /* @caller_sync */
```

> **All store API signatures are marked "to be confirmed by Wave 6."** The `nfc_service_t *const *svcs` form (array of const pointers to const service structs) matches spec v2 §6.5. nfc_stack builds a local array of service pointers for the active profile and passes it.

### 5.4 Up — Application (`main.c`) Usage Pattern

```c
/* ── Per-field-off UID rotation (replaces the old main.c rotation pattern) ──── */

/* Runs on nfc_work_q. Must not block. nfc_stack_set_uid() is @threadsafe and
 * safe to call from here (§6 deadlock analysis). */
static void app_nfc_event_cb(nfc_stack_event_t event, void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    if (event == NFC_STACK_EVENT_FIELD_OFF) {
        nfc_uid_t next_uid = { 0 };
        app_generate_next_uid(&next_uid);      /* application policy (random/sequence) */
        (void)nfc_stack_set_uid(&next_uid);    /* rotate on every field drop */
    }
}

/* ── Initialization ──────────────────────────────────────────────────────────── */
int rc;

/* 1. Initialize stack with defaults */
rc = nfc_stack_init(NULL);
if (rc != 0) {
    LOG_ERR("nfc_stack_init failed: %d", rc);
    return;  /* or handle */
}

/* 2. Register the application event callback — AFTER init, BEFORE start */
rc = nfc_stack_register_event_cb(app_nfc_event_cb, NULL);
if (rc != 0) {
    LOG_ERR("event cb registration failed: %d", rc);
}

/* 3. Optionally load a saved card — BEFORE start (load returns -EBUSY while
 *    STARTED; spec v2 §6.5) */
rc = nfc_stack_load("my_card");   /* backed by nfc_store_load — wave6-store.md §2 */
if (rc != 0) {
    LOG_WRN("No saved card; using defaults");
}

/* 4. Start emulation with initial UID */
nfc_uid_t uid = {
    .bytes = { 0x04U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU },
    .len   = NFC_UID_LEN_DOUBLE
};
rc = nfc_stack_start(&uid);
if (rc != 0) {
    LOG_ERR("nfc_stack_start failed: %d", rc);
}

/* ── Runtime ─────────────────────────────────────────────────────────────────── */

/* Switch to Aliro profile (e.g., on button press).
 * Applied on next field-off; the FIELD_OFF event (and thus the UID rotation in
 * app_nfc_event_cb) fires AFTER the profile is applied. */
(void)nfc_stack_set_profile(NFC_PROFILE_ALIRO);

/* Save current card state to shell dump (debug/capture facility; requires the
 * stack to be stopped first — save returns -EBUSY while STARTED, spec v2 §6.5) */
(void)nfc_stack_stop();
(void)nfc_stack_save("my_card");
(void)nfc_stack_start(&uid);

/* ── Teardown ────────────────────────────────────────────────────────────────── */
(void)nfc_stack_stop();
(void)nfc_stack_shutdown();
```

> **Application field-off notification:** Spec v2 §7 shows the application calling `nfc_stack_set_uid()` after a field-off event, delivered via `nfc_stack_register_event_cb()`. The application registers `app_nfc_event_cb` and rotates the UID on every `NFC_STACK_EVENT_FIELD_OFF`, preserving the prior main.c behavior (UID changes every time the reader field drops). See DECISION-4.

### 5.5 `src/nfc/CMakeLists.txt` (top-level NFC cmake)

```cmake
# src/nfc/CMakeLists.txt
if(CONFIG_NFC_STACK)
  add_subdirectory(hal)
  add_subdirectory(framing)
  add_subdirectory(router)

  target_sources(app PRIVATE
    nfc_stack.c
    nfc_stack_shell_cmds.c)

  target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(CONFIG_NFC_STORE)
  add_subdirectory(store)
endif()

# Service subdirectories added by Wave 5
if(CONFIG_NFC_SERVICE_NDEF)
  add_subdirectory(services/ndef)
endif()
if(CONFIG_NFC_SERVICE_DESFIRE)
  add_subdirectory(services/desfire)
endif()
if(CONFIG_NFC_SERVICE_ULTRALIGHT)
  add_subdirectory(services/ultralight)
endif()
if(CONFIG_NFC_SERVICE_EMV)
  add_subdirectory(services/emv)
endif()
if(CONFIG_NFC_SERVICE_ALIRO)
  add_subdirectory(services/aliro)
endif()
```

### 5.6 Shell (`nfc_stack_shell_cmds.c`, CONVENTIONS §10)

```c
SHELL_STATIC_SUBCMD_SET_CREATE(nfc_stack_cmds,
    SHELL_CMD(config,  NULL, "Print config (default_profile)",    cmd_nfc_stack_config),
    SHELL_CMD(stats,   NULL, "Print stats (all counters)",        cmd_nfc_stack_stats),
    SHELL_CMD(state,   NULL, "Print lifecycle state",             cmd_nfc_stack_state),
    SHELL_CMD(profile, NULL, "Print active profile",              cmd_nfc_stack_profile),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(nfc_stack, &nfc_stack_cmds, "NFC stack orchestrator", NULL);
```

`config` — prints `default_profile`.
`stats` — `nfc_stack_get_stats(&snap)` → `shell_print` all 8 fields including `apdu_drop_count` and `error_count`/`last_error_code`.
`state` — `nfc_stack_get_state()` → switch with `default` → string.
`profile` — prints `nfc_stack_get_active_profile()` as integer and mnemonic string. Also prints pending profile from `atomic_get(&s_pending_profile)` (or "none").

---

## 6. Implementation Notes

### DECISION-1: Field-Off / Pending-Profile Mechanism — Wrapper Ops (Exhaustive)

**The problem:** CONVENTIONS §8 requires the pending profile to be applied on the next field-off on `nfc_work_q`. CONVENTIONS §3 requires all cross-layer callback registration to happen in `nfc_stack.c`. Framing calls `aid_router_field_off()` directly from its ops handler. Profile switching (clear + re-register) is nfc_stack's responsibility. How does nfc_stack hook into the field-off event without violating coupling?

**Mechanism chosen: nfc_stack wrapper ops (`s_transport_ops`)**

nfc_stack defines a file-static `const nfc_transport_ops_t s_transport_ops` with three handlers. During `nfc_stack_init()`, `nfc_transport_register_callbacks(&s_transport_ops, NULL)` is called — NOT `apdu_assembler_get_ops()`. The wrapper handlers chain to framing's corresponding handlers (via `apdu_assembler_get_ops()`) and then add nfc_stack-specific logic.

**Field-off chain (in exact execution order on `nfc_work_q`):**

```
HAL field_off_handler (nfc_work_q)
  └─ calls s_ops->on_field_off(NULL)
       └─ stack_on_field_off (nfc_stack's wrapper, on nfc_work_q)
            │
            ├─ 1. apdu_assembler_get_ops()->on_field_off(NULL)
            │        └─ apdu_assembler_on_field_off_handler
            │             ├─ STATS_INC(field_off_count)
            │             └─ aid_router_field_off()
            │                  ├─ STATS_INC(field_off_count)
            │                  ├─ s_active_svc->on_field_off(ctx)  [service resets session]
            │                  └─ s_active_svc = NULL
            │
            ├─ 2. nfc_stack_apply_pending_profile()   [on same WQ thread]
            │         ├─ atomic_get(s_pending_profile) == NFC_PROFILE_NONE → return (no-op)
            │         │   OR
            │         └─ pending != NONE:
            │              ├─ atomic_set(s_pending_profile, NONE)  [consume]
            │              ├─ aid_router_clear()
            │              ├─ stack_register_profile(pending)
            │              └─ s_active_profile = pending
            │
            └─ 3. if (s_event_cb != NULL)                           [spec v2 §7]
                      s_event_cb(NFC_STACK_EVENT_FIELD_OFF, s_event_user_ctx)
                      └─ app callback: typically calls nfc_stack_set_uid(&next_uid)
                         → nfc_transport_set_uid → emulation_stop → param(NFCID1)
                           → emulation_start  [deadlock-free; see analysis below]
```

**Field-off execution order is therefore:** (1) framing chain → router field-off → service session cleanup, (2) pending-profile application (clear + re-register), (3) application event callback — the app always observes the post-cleanup, post-profile-switch state. The same wrapper position fires `NFC_STACK_EVENT_FIELD_ON` as the last step of `stack_on_field_on`.

**Why this is the correct choice:**

| Alternative | Rejected because |
|---|---|
| Router exposes `aid_router_set_field_off_cb(fn, ctx)` | Creates upward coupling from router to nfc_stack (router calling into nfc_stack's callback). Violates CONVENTIONS §3: "no layer includes another solely for callbacks." Router is below nfc_stack; callbacks flow upward, but the router cannot know about nfc_stack without an include. |
| Framing calls nfc_stack's `nfc_stack_on_field_off()` directly | Framing would need to `#include "nfc_stack.h"` — framing is below nfc_stack; this inverts the include hierarchy. |
| Separate `k_work` item submitted from `stack_on_field_off` for pending-profile | More complex; opens a race window where a new NFC field (and its SELECT) can arrive between `aid_router_field_off()` completing and the deferred profile-switch work item running on the WQ. The synchronous call within `stack_on_field_off` eliminates this race: the profile is switched before the WQ returns to its fifo drain loop. |
| HAL provides a second ops-slot for nfc_stack | The HAL only supports one registered ops struct (Pattern A singleton). Adding a second would require redesigning the locked Wave 1 HAL — not permissible. |

**Why no race on `s_active_profile`:** `s_active_profile` is written only from `nfc_stack_init()` (caller thread, before start) and from `nfc_stack_apply_pending_profile()` (nfc_work_q, inside field-off handler). The caller-thread writer (`init`) runs before `nfc_transport_start()`, so no WQ writes are in flight. After start, `s_active_profile` is written exclusively on the WQ during field-off. The application reads `s_active_profile` via `nfc_stack_get_active_profile()` on the caller thread. This is a benign race under Pattern A semantics: the application receives either the old or the new profile — never a torn value (it's an enum that fits in a single word on ARM32).

### Event Callback Threading Contract + set_uid Deadlock Analysis

**Contract:** the application event callback runs on `nfc_work_q`. It must not
block, sleep, or allocate; execution must be bounded (it shares the thread that
serves APDU dispatch — a slow callback delays the next session's first SELECT).

**`nfc_stack_set_uid()` from the callback is deadlock-free.** Verified against
wave1-hal.md §3 (`nfc_transport_set_uid` contract) and §4.1 (internal state):

1. `nfc_transport_set_uid()` takes `s_uid_mutex` (`k_mutex`, `K_FOREVER`). The
   callback runs in WQ *thread* context (not ISR), so taking a mutex is legal.
   The mutex is HAL-internal and serializes `set_uid` callers against each other
   and against `nfc_transport_stop()` (wave1-hal §3 stop) — no other code path
   on `nfc_work_q` ever holds it, so the WQ thread can never block on a lock
   held by itself or by a party waiting on the WQ.
2. The body is `emulation_stop()` → discard `s_cur_buf` (irq-locked) →
   `parameter_set(NFCID1)` → `emulation_start()`. **None of these interacts
   with `nfc_work_q`** — no `k_work_cancel_sync`, no queue drain, no fifo wait.
   Wave 1 states explicitly: "Does NOT tear down the work queue" and "The WQ
   thread keeps running across rotation."
3. `emulation_stop()` guarantees no further t4t callbacks before it returns —
   it does not wait on any WQ work item, so no circular wait exists.
4. Lifecycle interleaving: `nfc_stack_stop()`/`shutdown()` from the caller
   thread call `nfc_transport_stop()`, which first acquires `s_uid_mutex`
   (serializing against any in-flight `set_uid`, so a mid-callback rotation
   cannot re-enable emulation after stop — wave1-hal §3/§6) and whose
   `k_work_cancel_sync` then blocks until the in-flight field-off handler
   (including the callback and any `set_uid` inside it) completes. The mutex
   wait and the WQ wait are both bounded and one-directional — safe. (The
   ordering is benign: if `set_uid` holds the mutex, `stop` simply waits for
   the brief stop→param→start body to finish before proceeding.)
5. What is NOT allowed from the callback: `nfc_stack_stop()` / `nfc_stack_shutdown()`
   (they wait on the WQ the callback occupies — self-deadlock), and
   `nfc_stack_get_state()`/`get_active_profile()` misuse aside, any blocking call.
   `nfc_stack_set_profile()` IS allowed (`@threadsafe`, atomic store only);
   note that a profile set from within the FIELD_OFF callback is applied at the
   *next* field-off (step 2 of this chain has already run).

### Ordering Hazards

**1. Register profile services BEFORE `nfc_transport_start()`**

CONVENTIONS §3: callbacks must be registered after the emitting module's init() and before its start(). Service registration in the router (`aid_router_register`) is done during `nfc_stack_init()`, after all layers are init'd and wired, but before `nfc_transport_start()` is called (from `nfc_stack_start()`). This ensures that when the first field-on fires, the router's table is populated. No APDU can arrive before the first DATA_IND, and a DATA_IND requires a field-on first — which is submitted as a WQ work item. Since `nfc_work_q` is not running until `nfc_transport_start()` calls `k_work_queue_start()`, there is zero window for a SELECT to arrive against an empty table.

**2. Transport init AFTER framing and router inits**

Order in `nfc_stack_init()`: framing → router → store → transport. The transport is last because `nfc_transport_init()` registers the backend ISR callback (`nfc_t4t_setup()` in the nRFX backend). If the ISR fires before framing and router are initialized, `stack_on_apdu` → `framing->on_apdu` would call `apdu_assembler_on_apdu_handler`, which defensively guards on `s_state == INITIALIZED`. Under correct init order this guard never fires; but wrong order would cause a false-defensive drop (APDU silently discarded). Ordering transport last avoids the window entirely.

**3. `aid_router_clear()` before `aid_router_register()` on profile switch**

In `nfc_stack_apply_pending_profile()`, `aid_router_clear()` is called before re-registering the new profile. If `aid_router_register()` fails (table full due to misconfiguration), we attempt to re-register the OLD profile. Both operations run on the WQ inside the field-off handler — no APDU dispatch is in progress (the field is off; the ISR won't enqueue DATA_IND events until the next field-on). This guarantees the table is in a consistent state before any new reader interaction.

### Shutdown-While-Field-Present

If `nfc_stack_shutdown()` is called while the stack is STARTED and an NFC field is present:

1. `nfc_transport_stop()` first acquires `s_uid_mutex`, serializing against any in-flight `nfc_transport_set_uid()` (e.g. a rotation running inside the application's FIELD_OFF callback). This is the Wave 1 stop-vs-rotation fix (wave1-hal §3/§6): a mid-callback rotation cannot re-enable emulation after stop has begun.
2. It then stops backend emulation — guaranteeing no new ISR callbacks — and tears down the WQ: `k_work_cancel_sync` for all work items + `k_work_queue_drain(..., true)` + `k_work_queue_stop`. Any DATA_IND or field event already queued is either completed or cancelled before `nfc_transport_stop()` returns.
3. So when `nfc_transport_stop()` returns, the WQ is fully drained and stopped. No WQ callbacks can fire after that; `nfc_stack_apply_pending_profile()` cannot be called after `nfc_transport_stop()` returns. nfc_stack_shutdown then proceeds with service and layer teardown.

The state guard `if (s_state != NFC_STACK_STATE_STARTED) { return; }` in `nfc_stack_apply_pending_profile()` is a belt-and-suspenders defense for any future usage changes.

### Service Init Order and Rollback

**DECISION-3 (all compiled-in services inited during `nfc_stack_init()`):** nfc_stack initializes ALL compiled-in services during `init()`, not just the active profile's services. Rationale: profile switching (`set_profile` + field-off) must be able to immediately call `<svc>_service_get()` and register services without first calling `init()` on-the-fly. On-demand service init during profile switch would require `init()` on the WQ thread, which complicates the atomic ordering of `clear()` + `register()`. Eager init of all services during `nfc_stack_init()` eliminates this complexity at the cost of slightly higher boot-time initialization (typically < 1 ms total for all service inits). If a service has a very expensive init (unlikely), this design can be revisited at Wave 5 review.

**Rollback tracking:** `nfc_stack_init()` uses local `bool` flags to track which services have been successfully initialized. The rollback path (`err_svc_init`) shuts down only those services whose flag is `true`, in reverse order. Example:

```c
bool ndef_inited = false, desfire_inited = false, /* ... */;

if (IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)) {
    rc = ndef_service_init(NULL);
    if (rc != 0) { goto err_svc_init; }
    ndef_inited = true;
}
/* ... */
err_svc_init:
    /* Reverse order */
    if (IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO)     && aliro_inited)     aliro_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_EMV)       && emv_inited)       emv_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT) && ult_inited)      ultralight_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)   && desfire_inited)   desfire_service_shutdown();
    if (IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)      && ndef_inited)      ndef_service_shutdown();
    nfc_transport_shutdown();
    nfc_store_shutdown();
    aid_router_shutdown();
    apdu_assembler_shutdown();
    s_state = NFC_STACK_STATE_UNINITIALIZED;
    STATS_ERROR(&s_stats_lock, s_stats, rc);
    return -EIO;
```

### MISRA C:2012 Notes + DEV-ZEP Citations

1. **`LOG_*` macros** (`LOG_MODULE_REGISTER`, `LOG_ERR`, `LOG_INF`, `LOG_WRN`) in `nfc_stack.c` → **DEV-ZEP-008** (Rule 20.1 / Rule 19.10; variadic logging macro).
2. **`shell_print` variadic macro** in `nfc_stack_shell_cmds.c` → **DEV-ZEP-009** (Rule 20.1 / Rule 19.10).
3. **`ARG_UNUSED(user_ctx)`** in `stack_on_field_on/off/apdu` (signature fixed by `nfc_transport_ops_t`), and `ARG_UNUSED(argc)` / `ARG_UNUSED(argv)` in shell handlers → **DEV-ZEP-016** (Rule 2.7; unused parameter in framework callback).
4. **`IS_ENABLED(CONFIG_NFC_SERVICE_*)`** Zephyr utility macro — safe evaluation of Kconfig symbols. If flagged under Rule 20.1 → cover under **DEV-ZEP-008** class.
5. **`atomic_set` / `atomic_get`** on `s_pending_profile` — Zephyr atomic API is a thin wrapper over `__atomic_*` GCC builtins. No specific DEV-ZEP; these are MISRA-compliant when used only for `atomic_val_t` (plain integer type). Cast `(nfc_profile_t)pending` after range-check before use.
6. **`switch` on `nfc_profile_t`** in `stack_register_profile` and `nfc_stack_set_profile` must include `default:` arm → **MISRA Rule 16.4**.
7. **`memcpy` in `STATS_COPY_OUT`** — standard library function; compliant. No dynamic allocation. **MISRA Dir 4.12** satisfied.
8. **No `CONTAINER_OF`** — nfc_stack does not recover parent structs; DEV-ZEP-001 not needed.
9. **No `k_fifo`** — nfc_stack does not own the APDU fifo; DEV-ZEP-005 not needed.
10. **`K_SPINLOCK` scoped macro** NOT used — `STATS_*` macros use explicit `k_spin_lock/unlock`. DEV-ZEP-014 not needed.
11. **MISRA Rule 7.2** (`U` suffix): all unsigned literals — AID array byte values have `U` suffix; `sizeof(k_ndef_aid)` is a `size_t` constant expression (no suffix needed on `sizeof`).
12. **MISRA Rule 9.1** (initialized at declaration): `bool ndef_inited = false;` etc. in init rollback tracking; all locals declared with initializers.
13. **MISRA Rule 14.4** (Boolean controlling expressions): `(rc != 0)`, `(pending == NFC_PROFILE_NONE)`, `(s_state != NFC_STACK_STATE_STARTED)`, `(ops != NULL)`.
14. **MISRA Rule 17.7** (check return values): `nfc_transport_stop()` in shutdown path — explicitly `(void)`-cast when the return is intentionally ignored (shutdown always proceeds): `(void)nfc_transport_stop()`. Same for service shutdowns. `aid_router_register()` return value MUST be checked in `stack_register_profile()`.
15. **MISRA Rule 11.5** (`void *` conversion): `user_ctx` in wrapper ops — `ARG_UNUSED(user_ctx)`; no cast needed since it is unused. Pre-approved under CALLBACK_REGISTRATION_GUIDE canonical pattern.
16. **MISRA Dir 4.12** (no dynamic allocation): no `malloc`/`free`; all state is file-static; service pointers for save/load are stack-local arrays of fixed size (max 5 services). **Fully compliant.**
17. **MISRA Rule 10.1 / 10.5** (enum arithmetic): cast `(atomic_val_t)profile` and `(nfc_profile_t)atomic_get(...)` explicitly with range check before use. The `atomic_val_t` is a signed or unsigned integer — the cast is explicit and guarded.

---

## 7. Conventions Compliance (CONVENTIONS §12)

- [x] **Lifecycle matches §2** — Full (init/start/stop/shutdown); no `deinit`; restartable (stop + start). `nfc_stack` row in CONVENTIONS §2: "Full — init/start/stop/shutdown". Pattern A lifecycle + `atomic_t` pending-profile. ✓
- [x] **`config_t`/`stats_t`/`state_t` + three getters defined (§2)** — `nfc_stack_config_t` (§1.2), `nfc_stack_stats_t` (§1.3), `nfc_stack_state_t` (§1.4). `get_config` never NULL (`@isr_safe`); `get_stats` copy-out under spinlock (`@threadsafe`); `get_state` never fails (`@caller_sync`). ✓
- [x] **State storage Pattern A per §2** — `static nfc_stack_state_t s_state`; lifecycle written by `@caller_sync` only; no ISR reads of lifecycle state; `atomic_t s_pending_profile` for the cross-thread pending-profile value. CONVENTIONS §2 nfc_stack row: "Pattern A lifecycle + atomic_t pending-profile". ✓
- [x] **Coupling matches §3; callbacks follow §4** — All cross-layer wiring in `nfc_stack.c` (CONVENTIONS §3 wiring rule). nfc_stack does NOT include any layer header solely to wire callbacks — it includes them for lifecycle orchestration and uses `apdu_assembler_get_ops()` (already public API) for the wrapper. Service vtable (`nfc_service_t`) is Pattern A with `_fn` suffix typedefs, `user_ctx` naming, NULL-checks in router (Wave 3). `s_transport_ops` has no `user_ctx` member (DECISION-1 aligned with Wave 1 ops form). The application event callback (spec v2 §7) follows CALLBACK_REGISTRATION_GUIDE exactly: `nfc_stack_event_cb_fn` typedef (`_fn` suffix); setter `nfc_stack_register_event_cb(cb, user_ctx)` returns int with state guards (`-ENODEV` UNINITIALIZED / `-EBUSY` STARTED / `-EIO` ERROR); NULL clears both slots; `user_ctx` naming; invoker NULL-checks `s_event_cb` before every call; dispatch thread (`nfc_work_q`) documented in the header. ✓
- [x] **Buffer model §5** — nfc_stack does not directly handle `net_buf` in the steady state. The one edge case (`stack_on_apdu` when `framing->on_apdu == NULL`) calls `net_buf_unref(apdu)` — one call, one owner. No outbound buffers owned by nfc_stack. ✓
- [x] **Stats recipe §6** — `static nfc_stack_stats_t s_stats`; `static struct k_spinlock s_stats_lock` (NOT `K_SPINLOCK_DEFINE`); `STATS_RESET(s_stats)` as first statement after `-EALREADY` guard in `init()`; `STATS_INC` for `profile_switch_count`, `uid_rotation_count`, `save_count`, `load_count`, `profile_notsup_count`, `apdu_drop_count`; `STATS_ERROR` on failure paths; `STATS_COPY_OUT` in getter; `error_count` + `last_error_code` mandatory fields present. ✓
- [x] **Threading + annotations §7** — `@caller_sync` for `init/start/stop/shutdown/register_event_cb/save/load/get_state/get_active_profile`; `@threadsafe` for `set_uid/set_profile/get_stats/get_config`; `@isr_safe` for `get_config`; wrapper ops and the application event callback run on `nfc_work_q` (documented in header, with the no-block/no-sleep rule and the set_uid-from-callback support note). No sleep, no blocking in wrapper handlers. ✓
- [x] **Error codes §9; shell §10** — Standard errno only (`-EALREADY`, `-ENODEV`, `-EINVAL`, `-EIO`, `-ENOTSUP`); no invented codes; ISO 7816 status words are protocol-level only; `nfc_stack` shell with `config`/`stats`/`state`/`profile` in dedicated `nfc_stack_shell_cmds.c`. ✓
- [x] **MISRA notes + DEV-ZEP citations §11** — DEV-ZEP-008/009/016 cited; DEV-ZEP-001/005/014 noted as not needed; Rules 7.2, 9.1, 10.1, 10.5, 11.5, 14.4, 16.4, 17.7, Dir 4.12 all addressed in §6. ✓

---

## 8. Tasks

> Granularity: 2–5 min/step. TDD applies to the pure-logic helpers (profile validation, `stack_register_profile` decision table, pending-profile state machine) — these are testable with mocked layer APIs on `qemu_cortex_m33` or DK hardware (`native_sim` is Linux-CI-only in this repo). Lifecycle functions require integration-level testing (nrfxlib closed binary). Test path: `tests/unit/nfc_stack/` (standalone `CMakeLists.txt` + `prj.conf` + `testcase.yaml`, built `--no-sysbuild`; Twister tag `sigmation.nfc.stack`). Commit after each numbered group.

- [ ] **1. Scaffolding.**
  Create `src/nfc/CMakeLists.txt` per §5.5. Create `src/nfc/Kconfig` with all symbols from §1.6 in order — this REPLACES the provisional Wave-1 stub (wave1-hal task 1) — including the explicit `rsource "hal/Kconfig"` / `rsource "framing/Kconfig"` / `rsource "router/Kconfig"` lines (spec v2 §9). Verify the application top-level `Kconfig` `rsource`s `src/nfc/Kconfig` and the top-level `CMakeLists.txt` has `add_subdirectory(src/nfc)` (added in Wave 1's bootstrap; keep as-is). Verify that `hal/Kconfig`, `framing/Kconfig`, and `router/Kconfig` already exist and have their symbols (should be present from Waves 1-3 stubs). Verify the `NFC_APDU_BUF_SIZE` / `NFC_APDU_POOL_COUNT` symbols do NOT appear in `hal/Kconfig` (they must be in the top-level). Create stub files `nfc_stack.c` and `nfc_stack.h` (empty with include guards). Create `tests/unit/nfc_stack/` directory with minimal `CMakeLists.txt` + `prj.conf` + `testcase.yaml` (built `--no-sysbuild`). **Commit.**

- [ ] **2. `nfc_stack.h` — types and public API.**
  Write `src/nfc/nfc_stack.h` with include guard `SRC_NFC_NFC_STACK_H`. Includes: `hal/nfc_transport.h` (for `nfc_uid_t`), `<stdint.h>`, `<stdbool.h>`. Memory comment at top. Define `nfc_profile_t` enum with `NFC_PROFILE_NONE` constant; `nfc_stack_config_t` + `NFC_STACK_CONFIG_DEFAULT`; `nfc_stack_stats_t` (8 fields); `nfc_stack_state_t`; `nfc_stack_event_t` enum + `nfc_stack_event_cb_fn` typedef (§1.7); all 15 public prototypes (incl. `nfc_stack_register_event_cb`) with `@`-annotations and Doxygen. Include Doxygen for DECISION-1 in `nfc_stack_set_profile()`, DECISION-4 (rotation via event callback) in `nfc_stack_set_uid()`, and the nfc_work_q/no-block/set_uid-supported contract on `nfc_stack_event_cb_fn`. **Commit.**

- [ ] **3. (TDD) Profile validation helpers.**
  In `nfc_stack.c`, write and test the static helpers:
  ```c
  static bool profile_kconfig_enabled(nfc_profile_t profile);
  static bool profile_is_valid(nfc_profile_t profile);
  ```
  In `tests/unit/nfc_stack/test_nfc_stack_profile.c` (compiles `nfc_stack.c` with mock includes):
  - `test_profile_valid_ndef`: `NFC_PROFILE_NDEF` → valid (when `CONFIG_NFC_SERVICE_NDEF=y`).
  - `test_profile_valid_all`: `NFC_PROFILE_ALL` → always valid.
  - `test_profile_invalid_out_of_range`: `NFC_PROFILE_COUNT_` and beyond → invalid.
  - `test_profile_notsup_desfire`: `NFC_PROFILE_DESFIRE` when `CONFIG_NFC_SERVICE_DESFIRE=n` → ENOTSUP.
  All pass. **Commit.**

- [ ] **4. (TDD) AID constants and `stack_register_profile` decision table.**
  Define all 5 AID byte arrays (§1.5) in `nfc_stack.c`. Write `stack_register_profile(profile)` per §4.5. In tests:
  - Mock `aid_router_register()` (capture calls: AID + len + svc pointer).
  - Mock `ndef_service_get()`, `aliro_service_get()`, etc. returning distinct non-NULL pointers.
  - `test_register_ndef`: `NFC_PROFILE_NDEF` → one `register` call with k_ndef_aid (7 bytes).
  - `test_register_aliro`: `NFC_PROFILE_ALIRO` → two `register` calls (expedited 9 bytes, step-up 9 bytes, same service pointer).
  - `test_register_emv`: `NFC_PROFILE_EMV` → two calls: 14-byte PPSE AID, then the app AID from the mocked `emv_service_get_app_aid()` (same service pointer).
  - `test_register_all_enabled`: `NFC_PROFILE_ALL` with all services enabled → 6 register calls (NDEF+DeSFire+EMV×2+Aliro×2).
  - `test_register_all_partial`: `NFC_PROFILE_ALL` with only NDEF+ALIRO enabled → 3 register calls.
  All pass. **Commit.**

- [ ] **5. File-static state + statics declaration.**
  In `nfc_stack.c`:
  - Define all §4.1 statics (in order: `s_state`, `s_config`, `s_stats`, `s_stats_lock`, `s_pending_profile`, `s_active_profile`).
  - Forward-declare `stack_on_field_on`, `stack_on_field_off`, `stack_on_apdu`.
  - Define `s_transport_ops` const struct.
  - `LOG_MODULE_REGISTER(nfc_stack, CONFIG_LOG_DEFAULT_LEVEL)`.
  **Commit.**

- [ ] **6. Getters.**
  Implement `nfc_stack_get_config()` (`return &s_config`), `nfc_stack_get_stats()` (`STATS_COPY_OUT`), `nfc_stack_get_state()` (plain `return s_state`), `nfc_stack_get_active_profile()` (plain `return s_active_profile`). **Commit.**

- [ ] **7. Wrapper ops handlers.**
  Implement `stack_on_field_on`, `stack_on_field_off`, `stack_on_apdu` per §4.6 — including the application event firing: `field_on` fires `NFC_STACK_EVENT_FIELD_ON` last; `field_off` fires `NFC_STACK_EVENT_FIELD_OFF` last (after the framing chain AND after `nfc_stack_apply_pending_profile()`); both NULL-check `s_event_cb` first:
  ```c
  if (s_event_cb != NULL) {
      s_event_cb(NFC_STACK_EVENT_FIELD_OFF, s_event_user_ctx);
  }
  ```
  `stack_on_apdu` MUST include the explicit else branch from §4.6 (not a comment-only placeholder):
  ```c
  } else {
      net_buf_unref(apdu);
      STATS_INC(&s_stats_lock, s_stats, apdu_drop_count);
  }
  ```
  Implement `nfc_stack_apply_pending_profile()` per §4.4. Implement `stack_register_profile()` per §4.5 (uses `IS_ENABLED` guards). Implement `stack_register_all_enabled()` helper. All `ARG_UNUSED(user_ctx)` in handlers. **Commit.**

- [ ] **8. (TDD) `nfc_stack_apply_pending_profile()` logic.**
  In tests, mock `aid_router_clear()` (records call count) and `aid_router_register()` (records calls). Set `s_state = STARTED` directly. Seed `atomic_set(&s_pending_profile, NFC_PROFILE_NDEF)`.
  - `test_apply_pending_no_pending`: `s_pending_profile == NFC_PROFILE_NONE` → zero router calls.
  - `test_apply_pending_switches_profile`: pending = ALIRO → `clear` called once, 2 registers, `s_active_profile == ALIRO`, counter incremented.
  - `test_apply_pending_not_started`: `s_state != STARTED` → no calls.
  - `test_apply_pending_fallback_on_register_fail`: mock `register()` to return `-ENOMEM` for the pending profile → fallback re-registers old profile; `error_count` incremented.

  Event-ordering tests (mock event callback records an ordered call log alongside the mocked router calls):
  - `test_field_off_event_after_profile_apply`: register a mock event cb; seed pending = ALIRO; invoke `stack_on_field_off(NULL)` → log order is exactly [framing field_off chain, `aid_router_clear`, registers, event cb FIELD_OFF]; cb receives `NFC_STACK_EVENT_FIELD_OFF` + the registered `user_ctx`.
  - `test_field_off_event_no_cb`: `s_event_cb == NULL` → no crash; rest of the chain runs.
  - `test_field_on_event_fired`: invoke `stack_on_field_on(NULL)` → cb receives `NFC_STACK_EVENT_FIELD_ON` after framing's field-on.
  All pass. **Commit.**

- [ ] **9. `nfc_stack_init()` — full sequence.**
  Implement per §3: EALREADY guard; STATS_RESET; populate config; set `s_pending_profile = NFC_PROFILE_NONE` and `s_active_profile = s_config.default_profile`; default profile Kconfig validation; call sub-layer inits in order with rollback; call service inits with per-service bool tracking; register `s_transport_ops` with HAL; call `stack_register_profile(s_active_profile)`; set state = INITIALIZED. Full `err_svc_init` rollback label per §3.
  Note: write each init call with explicit `rc` check and `LOG_ERR` on failure. **Commit.**

- [ ] **10. `nfc_stack_start()` / `nfc_stack_stop()`.**
  Implement per §3:
  - `start`: idempotent guard; state check; `nfc_transport_start(uid)`; state machine; STATS_ERROR on fail; state = ERROR on hard fail.
  - `stop`: idempotent guard; `nfc_transport_stop()`; state machine; STATS_ERROR on fail.
  **Commit.**

- [ ] **11. `nfc_stack_shutdown()`.**
  Implement per §3: idempotent; implicit stop if STARTED (errors logged, not returned); shutdown all layers in reverse order; service shutdowns in reverse service init order (IS_ENABLED guards); clear `s_event_cb`/`s_event_user_ctx`; state = UNINITIALIZED; always return 0. **Commit.**

- [ ] **12. `nfc_stack_set_uid()` / `nfc_stack_set_profile()`.**
  - `set_uid`: NULL guard; state check; delegate to `nfc_transport_set_uid(uid)`; STATS_INC on success.
  - `set_profile`: range guard (`>= NFC_PROFILE_COUNT_` → `-EINVAL`); Kconfig guard (`-ENOTSUP` + `STATS_INC(profile_notsup_count)`); `atomic_set(&s_pending_profile, profile)`; return 0.
  **Commit.**

- [ ] **13. `nfc_stack_register_event_cb()`.**
  Implement per §3 contract with the CALLBACK_REGISTRATION_GUIDE guard table:
  ```c
  int nfc_stack_register_event_cb(nfc_stack_event_cb_fn cb, void *user_ctx)
  {
      if (s_state == NFC_STACK_STATE_UNINITIALIZED) {
          return -ENODEV;
      }
      if (s_state == NFC_STACK_STATE_STARTED) {
          return -EBUSY;
      }
      if (s_state == NFC_STACK_STATE_ERROR) {
          return -EIO;
      }
      s_event_cb       = cb;                          /* NULL clears */
      s_event_user_ctx = (cb != NULL) ? user_ctx : NULL;
      return 0;
  }
  ```
  Ztests (state seeded directly): `-ENODEV` when UNINITIALIZED; `-EBUSY` when STARTED; 0 + stored when INITIALIZED; NULL cb clears both slots; re-registration overwrites. **Commit.**

- [ ] **14. `nfc_stack_save()` / `nfc_stack_load()`.**
  Implement per §3: `-EINVAL` NULL-tag guard, `-ENODEV` UNINITIALIZED guard, `-EBUSY` guard when `s_state == STARTED` (spec v2 §6.5 — persistence never runs concurrently with WQ dispatch). Build `const nfc_service_t *svcs[5]` + `size_t n` for `s_active_profile` (inline switch per service; ULTRALIGHT → the ultralight service instance only, persist_id 0x03 — §3). Call `nfc_store_save/load(tag, svcs, n)` (contract locked in wave6-store.md §2; stub until Wave 6 is implemented). STATS_INC on success. **Commit.**

- [ ] **15. Shell.**
  Write `nfc_stack_shell_cmds.c`:
  - Includes: `nfc_stack.h`, `<zephyr/shell/shell.h>`.
  - `cmd_nfc_stack_config`: `get_config()` → print `default_profile`.
  - `cmd_nfc_stack_stats`: `get_stats(&snap)` → print all 8 fields; `last_error_code` as signed.
  - `cmd_nfc_stack_state`: `get_state()` → switch with `default` → string map.
  - `cmd_nfc_stack_profile`: `get_active_profile()` → string; also print pending profile from `atomic_get(&s_pending_profile)` as string ("none" if `NFC_PROFILE_NONE`).
  - `SHELL_STATIC_SUBCMD_SET_CREATE` + `SHELL_CMD_REGISTER(nfc_stack, ...)`.
  - `ARG_UNUSED(argc)`, `ARG_UNUSED(argv)` in all handlers → DEV-ZEP-016.
  **Commit.**

- [ ] **16. Build + lint.**
  Build minimal configuration (`CONFIG_NFC_STACK=y`, `CONFIG_NFC_SERVICE_NDEF=y`) against NCS v3.2.4 targeting `nrf54l15dk/nrf54l15/cpuapp`. Confirm the sub-layer `Kconfig` and `CMakeLists.txt` files (Waves 1-3 stubs) are found by the build system. Run the ztest suite:
  ```
  west twister -T tests/unit/nfc_stack --platform qemu_cortex_m33 --no-sysbuild
  ```
  (`native_sim` is Linux-CI-only; use `qemu_cortex_m33` or DK locally.)
  Run MISRA check:
  ```
  cppcheck --enable=all --addon=misra.py \
           --suppressions-list=misra-suppressions.txt \
           src/nfc/nfc_stack.c src/nfc/nfc_stack_shell_cmds.c
  ```
  Verify DEV-ZEP-008/009/016 suppressions cover all flagged macro uses. Confirm no new DEV-ZEP deviations beyond those cited in §6. **Commit.**

- [ ] **17. Multi-profile build smoke test.**
  Build with `CONFIG_NFC_SERVICE_NDEF=y CONFIG_NFC_SERVICE_ALIRO=y CONFIG_NFC_STACK_DEFAULT_PROFILE=5` (ALL profile). Confirm `CONFIG_NFC_ROUTER_MAX_AIDS=8` (default) accommodates 3 AIDs (NDEF + Aliro×2) with headroom. Add asserts/static asserts or a build-time check:
  ```c
  BUILD_ASSERT(CONFIG_NFC_ROUTER_MAX_AIDS >= 6,
               "NFC_ROUTER_MAX_AIDS must be >= 6 for NFC_PROFILE_ALL with all services");
  ```
  Document the Thread Analyzer `nfc_work_q` stack budget contribution from nfc_stack's wrapper handlers (stack frame ~16 B; negligible). **Commit.**

---

### Integration Note — BLE+NFC Coexistence (sigmation_experimental)

> **Open item (decide at integration time):** The `sigmation_experimental` product runs continuous BLE extended advertising and scan. NFC `nfc_transport_start()` / `nfc_transport_stop()` coordination with the BLE-driven product lifecycle is **not defined in this plan** — it is resolved at integration time. The analog orchestrator in the integration repo is `system_lifecycle.c`:
> - **Option A (app-driven):** the application calls `nfc_stack_start()` / `nfc_stack_stop()` independently from `system_lifecycle.c` based on product state machine.
> - **Option B (lifecycle-driven):** `system_lifecycle.c` wires NFC start/stop alongside BLE transitions, potentially pausing NFC when BLE requires exclusive RF time (if coexistence is needed on nRF54L15).
> Decision deferred to integration. No stack API change is required by either option — the existing `nfc_stack_start/stop` surface is sufficient.

---

### DECISION Flags Raised (for Human Review)

| # | Description | Section |
|---|---|---|
| **DECISION-1** | **Field-off/pending-profile mechanism: nfc_stack wrapper ops (`s_transport_ops`).** nfc_stack registers its OWN ops struct with the HAL (not `apdu_assembler_get_ops()`). The wrapper chains framing's handlers then applies the pending profile on field-off. All wiring stays in nfc_stack.c. Alternatives (router callback, framing→nfc_stack call, deferred k_work) all rejected with rationale. | §4.4, §6 |
| **DECISION-2** | Default profile on boot is `NFC_PROFILE_NDEF` (Kconfig-selectable via `NFC_STACK_DEFAULT_PROFILE`). | §1.1, §1.2 |
| **DECISION-3** | ALL compiled-in services are initialized during `nfc_stack_init()` (not just the active profile's services). Enables instant profile switching without on-demand service init on the WQ. | §4.4, §6, §3 |
| **DECISION-4** | **UID rotation is application-driven.** `nfc_stack_set_uid()` is a thin delegation to `nfc_transport_set_uid()`; nfc_stack performs no autonomous rotation. The application's field-off notification is the event callback: `nfc_stack_event_cb_fn` + `nfc_stack_register_event_cb(cb, user_ctx)` delivering `NFC_STACK_EVENT_FIELD_ON/OFF` on `nfc_work_q` (spec v2 §7). The app rotates the UID by calling `nfc_stack_set_uid()` from the FIELD_OFF event (deadlock-free per §6 analysis), preserving the prior main.c per-field-off rotation behavior. | §1.7, §2, §3, §6 |
| **DECISION-5** | `NFC_APDU_BUF_SIZE` and `NFC_APDU_POOL_COUNT` are defined in `src/nfc/Kconfig` (top-level), not in `hal/Kconfig`. This matches wave1-hal §1.7 "defined at the stack level." `src/nfc/Kconfig` explicitly `rsource`s every per-layer Kconfig; the application top-level Kconfig `rsource`s `src/nfc/Kconfig` (spec v2 §9). `NFC_STACK` carries no nrfxlib dependency — `NFC_T4T_NRFXLIB` is depended on by `NFC_HAL_BACKEND_NRFX` in `hal/Kconfig` (wave1-hal §1.7). | §1.6 |
| **DECISION-6** | `NFC_PROFILE_ULTRALIGHT` registers the NDEF AID (`D2 76 00 00 85 01 01`) with `ndef_service_get()` — same as `NFC_PROFILE_NDEF` from the router's perspective. The Ultralight/NDEF distinction is a service-level concern — confirmed by wave5-ultralight (persistence-only adapter, content injected via `ndef_service_set_content()`). | §1.5, §4.5 |
| **DECISION-7** | **Shutdown-while-field-present:** HAL's `nfc_transport_stop()` drains and stops `nfc_work_q` before returning. By the time teardown continues, no WQ callbacks can fire. The `s_state` guard in `nfc_stack_apply_pending_profile()` is belt-and-suspenders only. | §6 |
| **DECISION-8** | **Profile switch on `aid_router_register()` failure:** fallback to old profile re-registration. If the old profile also fails to re-register (pathological misconfiguration), the router table may be partially filled — logged as `LOG_ERR`, counted as `error_count`; no ERROR state (stack remains STARTED). Flagged for human review. | §4.4 |
| **DECISION-9** | **`s_active_profile` has a benign race** between WQ writes (at field-off) and caller-thread reads from `get_active_profile`. Pattern A is used (not atomic); the read is a single-word enum read on ARM32 — never torn. `save/load` are NOT exposed to the race: their `-EBUSY` guard means they read only while the WQ is quiesced (spec v2 §6.5). Application should not rely on `get_active_profile()` being exactly up-to-date during a field-off window. Flagged for human review. | §4.7 |
| **DECISION-10** | **Store init/shutdown included in `nfc_stack_init/shutdown` rollback chain** per CONVENTIONS §3. If Wave 6 chooses a different init signature or makes `nfc_store_init` always-succeed, this plan is still correct (error path simply never triggers). **To be confirmed by Wave 6.** | §3 |
