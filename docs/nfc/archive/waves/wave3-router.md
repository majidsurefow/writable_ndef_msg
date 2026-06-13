# Wave 3: Router (aid_router) Implementation Plan

**Status:** LOCKED — 2026-06-12 (harmonized v2)

**Layer:** `router/aid_router`

**Files produced:**
- `src/nfc/router/aid_router.h`
- `src/nfc/router/aid_router.c`
- `src/nfc/router/service.h`
- `src/nfc/router/aid_router_shell_cmds.c`
- `src/nfc/router/CMakeLists.txt`
- `src/nfc/router/Kconfig`
- `tests/unit/aid_router/` (ztest for AID matching and dispatch decision table — pure logic; Twister tag `sigmation.nfc.aid_router`; `native_sim` is Linux-CI-only — use `qemu_cortex_m33` locally)

**Depends on:**
- `docs/superpowers/plans/wave1-hal.md` (LOCKED — 2026-06-12)
- `docs/superpowers/plans/wave2-framing.md` (LOCKED — 2026-06-12, DECISION-7 deferred to here)

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 lifecycle/state, §3 coupling, §4 callback pattern, §5 buffer model, §6 stats, §7 threading, §8 profile switching, §9 errno, §10 shell, §11 MISRA, §12 checklist)
2. `docs/superpowers/plans/wave1-hal.md` — LOCKED; ground truth for `nfc_transport_send_response`, `nfc_work_q` dispatch context, `net_buf` model, `nfc_transport_ops_t` shape
3. `docs/superpowers/plans/wave2-framing.md` — LOCKED; ground truth for `nfc_apdu_t` (in `apdu_types.h`), parse guarantees, framing→router call sites (§5.2), DECISION-7 text, zero-copy lifetime window (§6)
4. `docs/NFC_WAVE_PLANNING_GUIDE.md` — per-wave process (8 steps) + plan template
5. `docs/nfc/archive/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) — §6.3 (router), §6.4.x (services, for SELECT response ownership and AID table sizing), §6.5 (store, for persistence hook expectations + save/load `-EBUSY` gating), §7 (stack orchestration), §8 (threading), §9 (Kconfig); retained as card-mode/NFCT implementation detail
6. `docs/nfc/archive/specs/2026-06-12-nfc-stack-architecture.md` (v3) — **architecture-of-record**; §4.2 (lane framing: ISO-DEP lane scope, raw lane reserved seam), §4.3 (protocol modules / lane tags), §5 (Kconfig)
7. `docs/API_DESIGN_CREED.md` — module lifecycle, Pattern A/B state, FSM, shell, coupling
8. `docs/CALLBACK_REGISTRATION_GUIDE.md` — `_fn`/`_cb` naming, `user_ctx`, guard/NULL-clear
9. `docs/STATS_API_DESIGN.md` — `s_stats`/`s_stats_lock` recipe, `STATS_*`, copy-out getter
10. `docs/NETWORK_BUFFERS.md` — one-owner rule, borrow lifetime
11. `docs/STACK_SPEC.md` — downward=direct call, upward=A/B/C/D, wiring in `nfc_stack.c`
12. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_RESET`, `STATS_COPY_OUT` macro implementations
13. `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001 through DEV-ZEP-018
14. `/Users/majidfaroud/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012 rules

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (replaced prototype — patterns must not carry forward).

---

> **Architecture Context (spec v3 reframe — additive):**
> This plan is the **NFCT / card-first-slice** implementation of
> [architecture spec v3](../specs/2026-06-12-nfc-stack-architecture.md).
> All existing Type-4 behavior, function/type names, and contracts below are
> **unchanged**.
>
> **Lane scope (spec §4.2):** `apdu_assembler` (Wave 2) + `aid_router` (Wave 3)
> together form the **ISO-DEP / Type-4 dispatch lane** — one lane among several.
> Raw/native lanes (Type 2/3/15693) have no AID/APDU; they bypass framing+router
> and are reserved seams not implemented in this slice. **This router dispatches
> only the ISO-DEP / APDU lane.**
>
> **Protocol modules (spec §4.3):** `nfc_service_t` (§1.1 service.h) is the
> **listener (card-role) face** of a *protocol module*. A protocol module owns:
> data model + (de)serialize + listener (built now, `CONFIG_NFC_ROLE_CARD`) +
> poller (reserved seam, `CONFIG_NFC_ROLE_READER`). Lane tags and reserved poller
> interface added to service.h — see §1.1 below.

---

## DECISION-7 Resolution (Highest Priority — Wave 2 Deferred)

> **DECISION-7: `aid_router_dispatch` accepts the pre-parsed `nfc_apdu_t` view, NOT raw bytes.**
>
> **Final locked signature:**
> ```c
> void aid_router_dispatch(const nfc_apdu_t *apdu);
> ```
>
> **Rationale:**
>
> The conflict: spec v1 §6.2 said framing "forwards the **parsed** APDU" to the router; spec v1 §6.3 defined `void aid_router_dispatch(const uint8_t *apdu, size_t len)` (raw bytes, router re-parses). CONVENTIONS §4 locks the service vtable `on_apdu` as `void (*on_apdu)(const nfc_apdu_t *apdu, void *user_ctx)`. Spec v2 §6.2/§6.3 now state the parsed form everywhere.
>
> Analysis of every path through the router:
>
> | Task | Raw-bytes form | Parsed form |
> |------|---------------|-------------|
> | Detect SELECT (CLA/INS/P1) | Must parse bytes[0..2] | Direct field access (`apdu->cla`, `->ins`, `->p1`) |
> | Extract AID from SELECT | Must parse Lc+data from bytes[4..] | Direct: `apdu->data[0..apdu->lc-1]` |
> | Forward to `svc->on_apdu(const nfc_apdu_t *, ctx)` | Must create an `nfc_apdu_t` view (re-parse from raw) | Pass `apdu` directly — zero extra work |
>
> The raw-bytes form forces the router to re-parse CLA/INS/P1/P2 and the data field — work that framing has already done correctly (with full ISO 7816-4 case handling). This is duplicate CPU cycles and duplicate logic, with no benefit. The parsed form eliminates the duplication, passes the exact type the service vtable requires, and directly implements spec v2 §6.2's "forward the parsed APDU" intent. The spec v1 §6.3 raw-bytes signature was an oversight.
>
> **Wave 2 call site change** (Wave 2 tasks 9/10 `TODO(Wave 3)` markers — replaced by this wave's task 14): In `apdu_assembler_on_apdu_handler`, the line:
> ```c
> /* TODO(Wave 3): aid_router_dispatch(&apdu); */
> ```
> becomes:
> ```c
> aid_router_dispatch(&apdu);   /* apdu is the stack-local nfc_apdu_t from apdu_parse(); */
>                                /* apdu.data points into buf->data, valid until net_buf_unref */
> ```
> The `apdu` local (populated by `apdu_parse`) is on the stack; `apdu.data` is a zero-copy pointer into the net_buf, valid for the entire duration of `aid_router_dispatch` (framing unrefs after dispatch returns). No lifetime problem.

---

## 1. Types and Constants

All public types live in `aid_router.h` (module contract) and `service.h` (service vtable, shared across router + all Wave 5 services + Wave 6 store). `service.h` is pure-C and has no Zephyr kernel headers so services can include it without pulling in kernel dependencies.

### 1.1 `service.h` — Shared Service Vocabulary

This header is the contract between the router layer (provider) and all service implementations (consumers). Wave 5 service agents and the Wave 6 store agent build against it in parallel; it **must be airtight and complete from day one**.

```c
/* src/nfc/router/service.h
 *
 * NFC service vtable — shared between router, all services, and the store layer.
 *
 * Thread safety: all callbacks are invoked exclusively on nfc_work_q.
 * Services must call nfc_transport_send_response() from on_select / on_apdu
 * (synchronously or via deferred work on nfc_work_q for crypto services like Aliro).
 *
 * Data lifetime: nfc_apdu_t.data points into the inbound net_buf, valid only
 * for the duration of on_apdu(). Services that need to retain APDU data past
 * the callback MUST copy it into their own static buffer before returning.
 */
#ifndef SRC_NFC_ROUTER_SERVICE_H
#define SRC_NFC_ROUTER_SERVICE_H

#include <stdint.h>
#include <stddef.h>
#include "framing/apdu_types.h"   /* nfc_apdu_t — zero-copy APDU view */

/* ── Callback typedefs (CONVENTIONS §4: suffix _fn, var/function _cb) ──────── */

/**
 * @brief Called when a SELECT-by-AID APDU matches this service's registered AID.
 *
 * The service MUST send a SELECT response via nfc_transport_send_response()
 * before returning (or submit deferred work to do so). The router does NOT
 * send 9000 — the service controls the SELECT response content (EMV/Aliro
 * return FCI data; NDEF/DeSFire return plain 9000).
 *
 * @param aid      AID bytes from the SELECT APDU data field.
 * @param aid_len  Length of aid (1..NFC_AID_MAX_LEN).
 * @param user_ctx Service-provided context pointer.
 * @nfc_work_q
 */
typedef void (*nfc_service_on_select_fn)(const uint8_t *aid, size_t aid_len,
                                         void *user_ctx);

/**
 * @brief Called for every non-SELECT APDU while this service is selected.
 *
 * The service MUST send a response via nfc_transport_send_response() before
 * returning (or submit deferred work). nfc_apdu_t.data is valid only during
 * this call — copy any data field bytes needed after return.
 *
 * @param apdu     Parsed APDU view (zero-copy into the inbound net_buf).
 * @param user_ctx Service-provided context pointer.
 * @nfc_work_q
 */
typedef void (*nfc_service_on_apdu_fn)(const nfc_apdu_t *apdu, void *user_ctx);

/**
 * @brief Called when a SELECT for a DIFFERENT AID deselects this service.
 *
 * The service should reset any per-session state. No response is sent here —
 * the new SELECT's on_select will send the response.
 *
 * @param user_ctx Service-provided context pointer.
 * @nfc_work_q
 */
typedef void (*nfc_service_on_deselect_fn)(void *user_ctx);

/**
 * @brief Called when the NFC field goes away.
 *
 * All per-session state should be cleared. No response is sent.
 *
 * @param user_ctx Service-provided context pointer.
 * @nfc_work_q
 */
typedef void (*nfc_service_on_field_off_fn)(void *user_ctx);

/* ── Persistence hook typedefs (consumed by Wave 6 store layer) ─────────────── */

/*
 * Concurrency rule (spec v2 §6.5): the store entry points (nfc_stack_save /
 * nfc_stack_load) return -EBUSY while the stack is STARTED, so persistence
 * hooks are never invoked concurrently with nfc_work_q dispatch. Services
 * therefore need NO internal locking between their data model and
 * serialize/deserialize — the hooks always run against a quiesced stack.
 */

/**
 * @brief Serialize the service's data model into out[0..max).
 *
 * Called by nfc_store_save() → svc->serialize(). The service writes its
 * persistent state as a flat byte array (format service-defined). Sets
 * *out_len to the number of bytes written.
 *
 * Never invoked while the stack is STARTED (nfc_stack_save returns -EBUSY —
 * spec v2 §6.5); no locking against on_apdu/on_select state is required.
 *
 * @param out      Destination buffer; exactly max bytes available.
 * @param max      Size of out in bytes.
 * @param out_len  [out] Bytes written; must be ≤ max.
 * @param user_ctx Service-provided context pointer.
 * @retval  0        Success.
 * @retval -EINVAL   out or out_len is NULL.
 * @retval -ENOSPC   max is too small to serialize the data model.
 * @caller_sync — invoked from nfc_stack_save() on the caller thread.
 */
typedef int (*nfc_service_serialize_fn)(uint8_t *out, size_t max,
                                        size_t *out_len, void *user_ctx);

/**
 * @brief Restore the service's data model from in[0..len).
 *
 * Called by nfc_store_load() → svc->deserialize(). The service reads bytes
 * written by a previous serialize() call and restores its persistent state.
 *
 * Never invoked while the stack is STARTED (nfc_stack_load returns -EBUSY —
 * spec v2 §6.5); no locking against on_apdu/on_select state is required.
 *
 * @param in       Source buffer (TLV body for this service's persist_id).
 * @param len      Length of in in bytes.
 * @param user_ctx Service-provided context pointer.
 * @retval  0        Success.
 * @retval -EINVAL   in is NULL or len is 0.
 * @retval -EBADMSG  Data is corrupt or version mismatch.
 * @caller_sync — invoked from nfc_stack_load() on the caller thread.
 */
typedef int (*nfc_service_deserialize_fn)(const uint8_t *in, size_t len,
                                          void *user_ctx);

/* ── Service vtable (listener face of a protocol module) ────────────────────── */

/**
 * @brief Listener (card-role) face of an NFC protocol module (spec v3 §4.3).
 *
 * A *protocol module* owns: data model + (de)serialize +
 * listener [this struct; compiled under CONFIG_NFC_ROLE_CARD] +
 * poller [reserved seam; CONFIG_NFC_ROLE_READER — see nfc_protocol_poller_vtable_t].
 * nfc_service_t IS the listener face. It is registered with aid_router_register()
 * and receives ISO-DEP / APDU events (lane NFC_LANE_ISO_DEP).
 *
 * Registered with aid_router_register(). All function pointers are invoked on
 * nfc_work_q except serialize/deserialize which are @caller_sync.
 *
 * CONVENTIONS §4 base form + Wave 3 persistence extension (per wave guide):
 *   - Core callbacks (on_select, on_apdu, on_deselect, on_field_off, user_ctx):
 *     LOCKED by CONVENTIONS §4.
 *   - Persistence fields (serialize, deserialize, persist_id):
 *     ADDED here per NFC_WAVE_PLANNING_GUIDE §Wave3 mandate so Wave 5 services
 *     build them in from the start. See DECISION-A below.
 *   - serialize and deserialize may be NULL (service not persistable by store).
 */
typedef struct {
    /* Core dispatch callbacks — invoked on nfc_work_q. NULL not permitted. */
    nfc_service_on_select_fn    on_select;     /**< AID matched — send SELECT response */
    nfc_service_on_apdu_fn      on_apdu;       /**< Non-SELECT APDU forwarded here     */
    nfc_service_on_deselect_fn  on_deselect;   /**< Service deselected (re-SELECT)     */
    nfc_service_on_field_off_fn on_field_off;  /**< Field removed                      */

    /* Persistence hooks — consumed by store layer (Wave 6). May be NULL. */
    nfc_service_serialize_fn    serialize;     /**< NULL = service not persistable      */
    nfc_service_deserialize_fn  deserialize;   /**< NULL = service not persistable      */

    /**
     * Stable identifier for TLV framing in saved blobs (store layer).
     * Must be unique across all compiled-in services. 0 = not persistable.
     * Assign from the table below; do not reuse IDs across service types.
     *
     * | Service    | persist_id |
     * |------------|------------|
     * | NDEF       | 0x01       |
     * | DeSFire    | 0x02       |
     * | Ultralight | 0x03       |
     * | EMV        | 0x04       |
     * | Aliro      | 0x05       |
     */
    uint8_t persist_id;

    /**
     * Opaque context pointer — passed unchanged to all callbacks.
     * NULL is legal (stateless/singleton service with file-static state).
     * CONVENTIONS §4: named user_ctx, never ctx/priv/arg.
     */
    void *user_ctx;
} nfc_service_t;

/* Convenience: maximum AID length per ISO 7816-4 §8.2 (DF name, 1..16 bytes). */
#define NFC_AID_MAX_LEN  16U

/* Persist ID assignments — stable; do not renumber. */
#define NFC_PERSIST_ID_NDEF        0x01U
#define NFC_PERSIST_ID_DESFIRE     0x02U
#define NFC_PERSIST_ID_ULTRALIGHT  0x03U
#define NFC_PERSIST_ID_EMV         0x04U
#define NFC_PERSIST_ID_ALIRO       0x05U

/* ── Lane tags (spec v3 §4.2) ────────────────────────────────────────────────── */
/* Protocol modules declare which dispatch lane they use.                         */
/* All five current modules (NDEF, Ultralight, DeSFire, EMV, Aliro) are          */
/* NFC_LANE_ISO_DEP. The aid_router dispatches ONLY the ISO-DEP lane.            */
/* NFC_LANE_RAW is a reserved seam for future Type 2/3/15693 modules.            */
typedef enum {
    NFC_LANE_ISO_DEP = 0, /**< ISO 14443-4 / APDU lane (framing + aid_router)     */
    NFC_LANE_RAW     = 1, /**< Raw / native frame lane (reserved — bypasses router)*/
} nfc_lane_t;

/* ── Reserved: protocol-module poller vtable (READER role) ─────────────────── */
/* RESERVED — NOT implemented in this slice. Header / interface only.            */
/* Spec v3 §4.4: reader role and protocol pollers are defined as interfaces now; */
/* implementation deferred until a reader-capable backend (ST25R3916 / PN7160).  */
/* Gated on CONFIG_NFC_ROLE_READER (never set for NFCT card-only backend).       */
/* Mirrors Flipper NfcPollerBase: alloc / detect / run / get_data, one per module*/
#ifdef CONFIG_NFC_ROLE_READER
typedef struct nfc_protocol_poller nfc_protocol_poller_t;
typedef nfc_protocol_poller_t *(*nfc_protocol_poller_alloc_fn)(void);
typedef bool (*nfc_protocol_poller_detect_fn)(nfc_protocol_poller_t *poller);
typedef int  (*nfc_protocol_poller_run_fn)(nfc_protocol_poller_t *poller);
typedef const void *(*nfc_protocol_poller_get_data_fn)(
                         const nfc_protocol_poller_t *poller);
typedef struct {
    nfc_protocol_poller_alloc_fn    alloc;    /**< Allocate poller state           */
    nfc_protocol_poller_detect_fn   detect;   /**< Detect protocol in NFC field    */
    nfc_protocol_poller_run_fn      run;      /**< Execute read / copy sequence    */
    nfc_protocol_poller_get_data_fn get_data; /**< Return populated data model     */
} nfc_protocol_poller_vtable_t;
#endif /* CONFIG_NFC_ROLE_READER */

#endif /* SRC_NFC_ROUTER_SERVICE_H */
```

> **DECISION-A (service.h persistence extension):** CONVENTIONS §4 locks the base `nfc_service_t` vtable with four callbacks + `user_ctx`. Spec v2 §6.3 additionally defines `serialize`, `deserialize`, and `persist_id` fields in the struct. The NFC_WAVE_PLANNING_GUIDE explicitly mandates "Wave 3 locks persistence hooks so Wave 5 services build them in from the start." Therefore Wave 3 extends the locked base form with the three persistence fields. This is not a contradiction — CONVENTIONS §4 covers the minimal callback shape; this wave adds required persistence fields on top. Wave 5 services must populate all fields; `serialize`/`deserialize` may be NULL for services that choose not to persist.

> **DECISION-B (on_select response ownership):** The router does NOT send a SELECT success response (9000). The service's `on_select` callback is wholly responsible for sending the SELECT response via `nfc_transport_send_response`. Rationale: EMV returns a PPSE FCI blob, Aliro returns protocol version info, DeSFire may return 9000 with card info — the response content is service-specific and unknown to the router. Sending 9000 from the router and then having the service "override" it is impossible (one response per APDU exchange). If `on_select` returns without having called `send_response`, the reader will stall until FWI timeout — this is a service implementation bug, detectable in integration testing.

> **DECISION-C (NFC_SW_COMMAND_NOT_ALLOWED 0x6986):** ISO 7816-4 SW 6986 ("Command not allowed — conditions of use not satisfied" variant for no current EF/DF) is the correct response when a non-SELECT APDU arrives with no service selected. The constant `#define NFC_SW_COMMAND_NOT_ALLOWED 0x6986U` lives in Wave 2's `apdu_types.h` (a native entry of the §1.1 status-word vocabulary, originally proposed by this wave and user-approved); the router uses it directly via `NFC_SW1`/`NFC_SW2` — no local definition exists.

### 1.2 `aid_router_config_t` — Frozen After `init()`

```c
typedef struct {
    /**
     * Maximum number of registered AID→service mappings.
     * Defaults to CONFIG_NFC_ROUTER_MAX_AIDS (default 8).
     * Capacity must be ≥ number of services used by any enabled profile.
     * ALL profile with all 5 services needs 6 table slots (EMV registers 2 AIDs:
     * PPSE + payment app AID; Aliro registers 2 AIDs):
     *   NDEF=1, DeSFire=1, EMV=2, Aliro=2 → total 6 for ALL profile.
     * Default 8 provides 2 slots of headroom for future services.
     */
    uint8_t max_aids;  /* range: 1..CONFIG_NFC_ROUTER_MAX_AIDS */
} aid_router_config_t;

#define AID_ROUTER_MAX_AIDS_DEFAULT  ((uint8_t)CONFIG_NFC_ROUTER_MAX_AIDS)
```

> **DECISION-D (config carries max_aids):** The table capacity is a config-time constant (Kconfig `NFC_ROUTER_MAX_AIDS`, default 8). It is exposed in `aid_router_config_t` for observability via `get_config()` even though init uses it to size the static table. This matches the creed's pattern where config is "what the module was initialized with."

### 1.3 `aid_router_stats_t`

```c
typedef struct {
    uint32_t select_matched_count;    /**< SELECT AID matched a registered service    */
    uint32_t select_unmatched_count;  /**< SELECT AID found no matching entry         */
    uint32_t apdu_routed_count;       /**< Non-SELECT APDUs forwarded to a service    */
    uint32_t apdu_no_service_count;   /**< Non-SELECT arrived with no service selected*/
    uint32_t field_off_count;         /**< aid_router_field_off() calls               */
    uint32_t register_table_full_count; /**< aid_router_register() when table is full */
    uint32_t error_count;             /**< Mandatory — STATS_ERROR increments this    */
    int32_t  last_error_code;         /**< Mandatory — last code passed to STATS_ERROR*/
} aid_router_stats_t;
```

> **DECISION-E (stats struct renamed and extended):** The spec v1 §6.3 named this `nfc_router_stats_t`. CONVENTIONS §2 requires `<module>_stats_t` naming → `aid_router_stats_t`. Added `apdu_no_service_count` and `register_table_full_count` per CONVENTIONS §6 rule: "every silent-drop path has a named counter." The spec v1 struct lacked these; spec v2 §6.3 defers the exact field list to this plan (§1.3).

### 1.4 `aid_router_state_t` — Pattern A

```c
typedef enum {
    AID_ROUTER_STATE_UNINITIALIZED = 0,  /**< Zero-init default; before init()  */
    AID_ROUTER_STATE_INITIALIZED,        /**< After init(); ready for dispatch   */
} aid_router_state_t;
```

Minimal lifecycle per CONVENTIONS §2. No STARTED/STOPPED/ERROR states.

### 1.5 Registration Table Entry (Internal — `aid_router.c` Only)

```c
/* Internal — not in the public header. */
typedef struct {
    uint8_t              aid[NFC_AID_MAX_LEN];  /* stored AID bytes                */
    uint8_t              aid_len;               /* 1..NFC_AID_MAX_LEN              */
    const nfc_service_t *svc;                   /* non-NULL while slot is occupied */
} aid_router_entry_t;
```

### 1.6 Status-Word Response Buffers (Internal — `aid_router.c` Only)

```c
/* file-static const; borrow-until-next-callback lifetime trivially satisfied. */
static const uint8_t s_sw_file_not_found[2] = {
    NFC_SW1(NFC_SW_FILE_NOT_FOUND),       /* 0x6A */
    NFC_SW2(NFC_SW_FILE_NOT_FOUND)        /* 0x82 */
};

/* 6986: Command not allowed (no service selected).
 * NFC_SW_COMMAND_NOT_ALLOWED comes from framing/apdu_types.h (DECISION-C). */
static const uint8_t s_sw_command_not_allowed[2] = {
    NFC_SW1(NFC_SW_COMMAND_NOT_ALLOWED),  /* 0x69 */
    NFC_SW2(NFC_SW_COMMAND_NOT_ALLOWED)   /* 0x86 */
};
```

### 1.7 Kconfig Symbols (`src/nfc/router/Kconfig`)

| Symbol | Type | Default | Purpose |
|--------|------|---------|---------|
| `NFC_ROUTER_MAX_AIDS` | int | `8` | Maximum registered AID→service table entries. Must be ≥ 6 for ALL profile. |

```kconfig
config NFC_ROUTER_MAX_AIDS
    int "Maximum registered AIDs in the router table"
    default 8
    range 1 32
    depends on NFC_STACK
    help
      Maximum number of AID→service mappings the router can hold simultaneously.
      The NFC_PROFILE_ALL configuration requires 6 slots (NDEF=1, DeSFire=1,
      EMV=2 [PPSE + payment app AID], Aliro=2). The default of 8 provides
      headroom for future services. Increase for builds registering additional
      proprietary AIDs.
```

---

## 2. Public API

All declarations in `aid_router.h`. Memory comment at top of header:

```c
/*
 * Memory: aid_router_config_t    ~4 B  (file-static, frozen after init)
 *         aid_router_stats_t    ~32 B  (file-static, spinlock-guarded)
 *         s_state                ~4 B  (plain enum, Pattern A)
 *         s_stats_lock           ~4 B  (struct k_spinlock)
 *         s_table[]             CONFIG_NFC_ROUTER_MAX_AIDS * sizeof(aid_router_entry_t)
 *                               = 8 * (16+1+4) = 168 B (default)
 *         s_table_count, s_active_svc pointer  ~8 B
 *         s_sw_*[2] x2           4 B  (file-static const)
 * No threads, no pools (HAL owns nfc_work_q and nfc_apdu_pool).
 * Lifecycle: init / shutdown only (minimal).
 */
```

```c
/**
 * @brief Initialize the router layer.
 *
 * Resets all stats, populates config, clears the registration table and
 * active-service pointer, then transitions to INITIALIZED.
 * Does NOT start any thread, allocate any buffer, or register any callback.
 * Registration is done subsequently by nfc_stack.c via aid_router_register().
 *
 * @retval  0         Success: UNINITIALIZED → INITIALIZED.
 * @retval -EALREADY  Already INITIALIZED.
 * @caller_sync
 */
int aid_router_init(void);

/**
 * @brief Shut down the router layer.
 *
 * Clears the registration table and active-service pointer, then transitions
 * to UNINITIALIZED. Idempotent. Always returns 0.
 * Must be called only after nfc_transport_stop() (no in-flight dispatch).
 *
 * @retval 0  Always. Shutdown must never fail.
 * @caller_sync
 */
int aid_router_shutdown(void);

/**
 * @brief Register an AID→service mapping.
 *
 * Copies the AID bytes into the table (safe to pass stack-local aid).
 * Multiple entries may point to the same nfc_service_t instance (Aliro: two
 * AIDs, one service). Registration order is preserved; the table is scanned
 * front-to-back on each SELECT.
 *
 * @param aid      AID bytes to register. Must not be NULL.
 * @param aid_len  Length of aid; must be 1..NFC_AID_MAX_LEN (16).
 * @param svc      Service vtable to associate. Must not be NULL.
 *                 Must remain valid for the lifetime of the registration.
 * @retval  0        Success.
 * @retval -EINVAL   aid is NULL, svc is NULL, aid_len == 0, or
 *                   aid_len > NFC_AID_MAX_LEN.
 * @retval -ENODEV   Router is UNINITIALIZED.
 * @retval -ENOMEM   Registration table is full (CONFIG_NFC_ROUTER_MAX_AIDS
 *                   entries already registered).
 * @caller_sync — register after aid_router_init() and before
 *                nfc_transport_start(). Also callable from nfc_work_q
 *                during profile-switch field-off handling (see §4.4).
 */
int aid_router_register(const uint8_t *aid, size_t aid_len,
                        const nfc_service_t *svc);

/**
 * @brief Clear all registered AID→service mappings.
 *
 * Resets the registration table to empty and clears the active-service
 * pointer (without calling on_deselect — field-off already handled that).
 * Used by nfc_stack.c during profile switching (CONVENTIONS §8):
 *   field-off → aid_router_clear() → re-register new profile's AIDs.
 *
 * @caller_sync — safe on caller thread (before start); also callable from
 *                nfc_work_q during field-off profile switching (§4.4).
 * @threadsafe  — NO. Do not call concurrently with dispatch. The caller
 *                (nfc_stack) guarantees this by only calling clear from
 *                field-off context, where no APDU dispatch is in progress.
 */
void aid_router_clear(void);

/**
 * @brief Dispatch a parsed APDU to the appropriate service.
 *
 * Implements the full dispatch decision table (§3). Detects SELECT-by-AID,
 * performs AID matching, invokes service callbacks, and sends protocol-error
 * responses (6A82 / 6986) for unroutable APDUs.
 *
 * @param apdu  Parsed APDU view (zero-copy into the inbound net_buf).
 *              apdu->data is valid until this function returns — the caller
 *              (framing) unrefs the net_buf after this call returns.
 *              Services that need APDU data beyond on_apdu() must copy it.
 *              Must not be NULL.
 * @nfc_work_q — must only be called from the nfc_work_q thread.
 */
void aid_router_dispatch(const nfc_apdu_t *apdu);

/**
 * @brief Notify the router that the NFC field went away.
 *
 * Invokes on_field_off on the currently selected service (if any), then
 * clears the active-service pointer. Does not clear the registration table
 * (services remain registered for the next session).
 *
 * @nfc_work_q — must only be called from the nfc_work_q thread.
 */
void aid_router_field_off(void);

/* ── Module contract getters (CONVENTIONS §2) ────────────────────────────────── */

/**
 * @retval Non-NULL always (file-static backing; safe before init()).
 * @isr_safe
 */
const aid_router_config_t *aid_router_get_config(void);

/**
 * @param out  Destination; must not be NULL.
 * @retval  0       Success.
 * @retval -EINVAL  out is NULL.
 * @threadsafe
 */
int aid_router_get_stats(aid_router_stats_t *out);

/**
 * @retval aid_router_state_t — never fails.
 * @caller_sync  (Pattern A plain enum)
 */
aid_router_state_t aid_router_get_state(void);
```

> **DECISION-F (aid_router_dispatch is void, not int):** Spec v2 §6.3 declares `void aid_router_dispatch(...)`. Wave 2 §6 (MISRA note 12) explicitly notes: "if Wave 3 declares it void, no check needed." The function is void because: (a) every dispatch path has a defined outcome (service callback invoked, or error response sent); (b) there is no recoverable error the framing layer could act on; (c) framing holds the net_buf regardless of dispatch outcome. Protocol-level rejections (6A82, 6986) are outcomes, not errors. Module errors (e.g., send_response fails) are handled internally via STATS_ERROR + LOG. This matches wave2's dispatch contract (DECISION-8 analogy).

> **DECISION-G (aid_router_field_off is void):** Same rationale as DECISION-F. No recoverable error from field-off notification. Errors in callback invocation are logged.

---

## 3. Contracts

### `aid_router_init()`

- **Pre:** `s_state == AID_ROUTER_STATE_UNINITIALIZED`. `@caller_sync`.
- **Post (0):**
  1. Check for `-EALREADY` **before** `STATS_RESET` (never reset stats on double-init).
  2. `STATS_RESET(s_stats)` — first statement after the EALREADY guard.
  3. `s_config` populated (`max_aids = AID_ROUTER_MAX_AIDS_DEFAULT`).
  4. `s_table_count = 0U`, all `s_table[i].svc = NULL` (effectively clear).
  5. `s_active_svc = NULL`.
  6. `s_state = AID_ROUTER_STATE_INITIALIZED`.
  - No threads started, no callbacks invoked, no buffers allocated.
- **Errors:**
  - `-EALREADY` — `s_state` is already INITIALIZED.

### `aid_router_shutdown()`

- **Pre:** any state. `@caller_sync`.
- **Post (0):**
  - `s_table_count = 0U`; `s_active_svc = NULL`.
  - `s_state = AID_ROUTER_STATE_UNINITIALIZED`.
  - Idempotent: UNINITIALIZED input → 0, no-op.
  - No callbacks invoked on shutdown.
- **Errors:** None. Always returns 0.

### `aid_router_register(aid, aid_len, svc)`

- **Pre:** state == INITIALIZED. `@caller_sync` or `@wq` (profile switch context).
- **Post (0):**
  - `s_table_count < max_aids`.
  - AID bytes copied into `s_table[s_table_count].aid[0..aid_len-1]`.
  - `s_table[s_table_count].aid_len = aid_len`.
  - `s_table[s_table_count].svc = svc`.
  - `s_table_count++`.
- **Errors:**
  - `-EINVAL` — `aid == NULL || svc == NULL || aid_len == 0U || aid_len > NFC_AID_MAX_LEN`.
  - `-ENODEV` — `s_state == AID_ROUTER_STATE_UNINITIALIZED`.
  - `-ENOMEM` — `s_table_count >= s_config.max_aids`; bumps `register_table_full_count`.

### `aid_router_clear()`

- **Pre:** any state (safe to call even when UNINITIALIZED). Caller guarantees no concurrent `aid_router_dispatch` is in progress.
- **Post:**
  - `s_table_count = 0U`; all `s_table[i].svc = NULL`.
  - `s_active_svc = NULL` (cleared without calling `on_deselect` — clearing during field-off means deselect was already handled by `aid_router_field_off()`).
  - `s_state` unchanged.
  - Returns void.

### `aid_router_dispatch(apdu)`

Runs exclusively on `nfc_work_q`. `apdu != NULL` guaranteed by the caller (framing NULL-checks before calling — but router also defensively guards).

**Full Dispatch Decision Table:**

| # | Condition | Actions | Protocol Response | Stats |
|---|-----------|---------|-------------------|-------|
| **A** | `apdu == NULL` | `STATS_ERROR(code=-EINVAL)`; `LOG_ERR`; return | none (no response possible) | `error_count++` |
| **B** | `s_state != INITIALIZED` | `STATS_ERROR(-ENODEV)`; `LOG_ERR`; return | none | `error_count++` |
| **C** | SELECT AID (`cla=0x00, ins=0xA4, p1=0x04`), AID match found, `s_active_svc != NULL` (different service) | Call `s_active_svc->on_deselect(ctx)` (if `on_deselect != NULL`); update `s_active_svc = match->svc`; call `new_svc->on_select(aid, aid_len, ctx)` | **service sends its own SELECT response** | `select_matched_count++` |
| **D** | SELECT AID, AID match found, `s_active_svc == NULL` | `s_active_svc = match->svc`; call `svc->on_select(aid, aid_len, ctx)` | **service sends its own SELECT response** | `select_matched_count++` |
| **E** | SELECT AID, AID match found, same service already active | Call `on_deselect` then `on_select` (re-select: reader deselected implicitly); same `s_active_svc` | **service sends its own SELECT response** | `select_matched_count++` |
| **F** | SELECT AID, `apdu->lc == 0` (empty data field) | Treat as no-match; send 6A82 | **router sends 6A82** | `select_unmatched_count++` |
| **G** | SELECT AID, no AID match in table | Send `s_sw_file_not_found` via `nfc_transport_send_response`; `s_active_svc` unchanged (cleared only on field-off) | **router sends 6A82** | `select_unmatched_count++` |
| **H** | Non-SELECT APDU, `s_active_svc != NULL` | Call `s_active_svc->on_apdu(apdu, ctx)` | **service sends its own response** | `apdu_routed_count++` |
| **I** | Non-SELECT APDU, `s_active_svc == NULL` | Send `s_sw_command_not_allowed` via `nfc_transport_send_response` | **router sends 6986** | `apdu_no_service_count++` |

**SELECT detection:**
```c
static bool apdu_is_select_by_aid(const nfc_apdu_t *apdu)
{
    return (apdu->cla == NFC_CLA_ISO7816) &&
           (apdu->ins == NFC_INS_SELECT)  &&
           (apdu->p1  == NFC_SELECT_P1_BY_AID) &&
           (apdu->lc  > 0U);
}
```

(Uses constants already defined in `framing/apdu_types.h`: `NFC_CLA_ISO7816=0x00`, `NFC_INS_SELECT=0xA4`, `NFC_SELECT_P1_BY_AID=0x04`.)

**AID match algorithm** (see also §6):
```c
/* Linear scan; O(n) with n ≤ CONFIG_NFC_ROUTER_MAX_AIDS (max 32, default 8). */
static const aid_router_entry_t *find_aid(const uint8_t *aid, uint8_t aid_len)
{
    for (uint8_t i = 0U; i < s_table_count; i++) {
        if ((s_table[i].aid_len == aid_len) &&
            (memcmp(s_table[i].aid, aid, (size_t)aid_len) == 0)) {
            return &s_table[i];
        }
    }
    return NULL;
}
```

**send_response failure handling** (rows F, G, I):
When `nfc_transport_send_response()` returns non-zero (HAL error), the router:
1. `(void)`-casts nothing — it must check and record the error.
2. `STATS_ERROR(&s_stats_lock, s_stats, rc)` — counts the module error.
3. `LOG_ERR("nfc_transport_send_response failed: %d", rc)`.
4. No further recovery (APDU exchange is lost; reader will retry or timeout at FWI).

```c
static void router_send_sw(const uint8_t *sw_buf, size_t sw_len)
{
    int rc = nfc_transport_send_response(sw_buf, sw_len);
    if (rc != 0) {
        STATS_ERROR(&s_stats_lock, s_stats, rc);
        LOG_ERR("router send_response failed: %d", rc);
    }
}
```

**Case D/E/F — SELECT detail (AID extraction from APDU):**
```c
/* AID is in apdu->data[0..apdu->lc-1].
 * apdu->data is never NULL when apdu->lc > 0 (Wave 2 DECISION-2 guarantee).
 * AID length = apdu->lc; clamp to NFC_AID_MAX_LEN if reader sends oversized AID. */
uint8_t aid_len = (apdu->lc <= NFC_AID_MAX_LEN) ?
                  (uint8_t)apdu->lc : NFC_AID_MAX_LEN;
const uint8_t *aid = apdu->data;
```

> **DECISION-H (AID clamping on oversized SELECT):** ISO 7816-4 §11.2.2 allows SELECT AID up to 16 bytes (`NFC_AID_MAX_LEN`). If a reader sends `apdu->lc > 16`, the AID is truncated to 16 bytes for matching. In practice this produces a no-match (row G → 6A82) since no registered AID exceeds 16 bytes. This is defensive rather than a rejection, consistent with the router being a dispatch layer, not a structural validator (framing already validated structure).

### `aid_router_field_off()`

Runs on `nfc_work_q`.

**Sequence:**
1. `STATS_INC(&s_stats_lock, s_stats, field_off_count)`.
2. If `s_active_svc != NULL && s_active_svc->on_field_off != NULL`:
   - Call `s_active_svc->on_field_off(s_active_svc->user_ctx)`.
3. `s_active_svc = NULL`.
4. Registration table is **NOT** cleared (services remain registered; a new session can proceed immediately without re-registration — clear only on profile switch).

### Getters

- `get_config` — returns `&s_config`; never NULL; valid before `init()`. `@isr_safe`.
- `get_stats` — `STATS_COPY_OUT(&s_stats_lock, s_stats, out)`; `-EINVAL` if `out == NULL`; 0 on success. `@threadsafe`.
- `get_state` — plain `return s_state`; never fails. `@caller_sync`.

---

## 4. Internal State

### 4.1 File-Static State (`aid_router.c`)

```c
/* Lifecycle — Pattern A (plain enum; WQ-only data path; lifecycle caller-sync).
 * No ISR reads s_state; no atomic needed. */
static aid_router_state_t s_state = AID_ROUTER_STATE_UNINITIALIZED;

/* Required module statics */
static aid_router_config_t s_config = { .max_aids = 0U }; /* 0 until init() */
static aid_router_stats_t  s_stats;
static struct k_spinlock   s_stats_lock; /* NOT K_SPINLOCK_DEFINE (NCS v3.2.4) */

/* Registration table — written by register/clear (caller_sync or WQ field-off),
 * read by dispatch (WQ only). No lock needed (see §4.4). */
static aid_router_entry_t  s_table[CONFIG_NFC_ROUTER_MAX_AIDS];
static uint8_t             s_table_count = 0U;

/* Currently selected service; NULL = no selection.
 * Written and read exclusively on nfc_work_q (or before start). */
static const nfc_service_t *s_active_svc = NULL;

/* Error-response buffers — file-static const; borrow lifetime = module lifetime. */
static const uint8_t s_sw_file_not_found[2] = {
    NFC_SW1(NFC_SW_FILE_NOT_FOUND), NFC_SW2(NFC_SW_FILE_NOT_FOUND)
};
static const uint8_t s_sw_command_not_allowed[2] = {
    NFC_SW1(NFC_SW_COMMAND_NOT_ALLOWED), NFC_SW2(NFC_SW_COMMAND_NOT_ALLOWED)
};
```

### 4.2 Lifecycle State Machine (Pattern A)

| From | Event | To | Actions |
|------|-------|----|----|
| UNINITIALIZED | `init()` | INITIALIZED | EALREADY guard; `STATS_RESET`; populate `s_config`; clear table + `s_active_svc`; set state |
| INITIALIZED | `init()` | INITIALIZED | return `-EALREADY` (STATS_RESET NOT called) |
| any | `shutdown()` | UNINITIALIZED | clear table + active; set state; return 0 |
| INITIALIZED | `register(aid, svc)` | INITIALIZED | append to table or return `-ENOMEM` |
| any | `clear()` | (unchanged) | zero table; clear active; return void |
| INITIALIZED | `dispatch(apdu)` | INITIALIZED | full dispatch decision table (§3) |
| INITIALIZED | `field_off()` | INITIALIZED | invoke active service on_field_off; `s_active_svc = NULL` |

No STARTED / STOPPED / ERROR states — minimal lifecycle per CONVENTIONS §2.

### 4.3 Concurrency Table

| Variable | Written by | Read by | Synchronization |
|----------|-----------|---------|----------------|
| `s_state` | `init`/`shutdown` (caller thread) | `dispatch`/`field_off` (WQ, defensive) | None required (Pattern A; lifecycle and WQ serialized by nfc_stack usage per §4.4) |
| `s_stats.*` | `dispatch`/`field_off` (WQ), `register` (caller or WQ), `init`/`shutdown` (caller) | `get_stats` (any thread) | `s_stats_lock` (via `STATS_*` macros) |
| `s_config` | `init()` (caller, once; frozen) | `get_config` (any), `register` | None after init (frozen, read-only) |
| `s_table[]`, `s_table_count` | `register`/`clear` (caller or WQ field-off) | `dispatch` (WQ) | None needed (§4.4) |
| `s_active_svc` | `dispatch`/`field_off`/`clear` (all WQ or pre-start caller) | `dispatch`/`field_off` (WQ) | None needed (§4.4) |
| `s_sw_*` | compile-time constant | `dispatch` (WQ), `router_send_sw` | None (read-only) |

### 4.4 Thread Ownership and Race Analysis

**Registration table (`s_table`, `s_table_count`):**

Two contexts write the table:
1. **@caller_sync** — `aid_router_register`/`aid_router_clear` during initial setup (before `nfc_transport_start()`). No WQ is running; no APDU dispatch is possible.
2. **nfc_work_q** — `aid_router_clear()` + `aid_router_register()` during profile switching (CONVENTIONS §8: "applied on the next field-off, on nfc_work_q").

Field-off processing on the WQ is ordered: `field_off_handler` → `aid_router_field_off()` (clears `s_active_svc`) → nfc_stack profile-switch logic → `aid_router_clear()` → `aid_router_register()` per new profile. This entire sequence runs on the same WQ thread. No concurrent APDU dispatch can occur (the ISR won't enqueue new DATA_IND events while the field is off). Therefore: **no lock is needed on `s_table` or `s_table_count`**.

**`s_active_svc`:**

Read and written exclusively on nfc_work_q (`dispatch`, `field_off`, `clear`) or before start (caller init). Same single-thread ordering guarantee as the table. **No lock needed.**

**`s_state`:**

Written by lifecycle functions (`@caller_sync` only). Read defensively in `dispatch`/`field_off` (on WQ). Under correct nfc_stack usage, lifecycle functions (init/shutdown) do not execute concurrently with dispatch. Pattern A (plain enum) is correct; escalation to Pattern B (`atomic_t`) would only be needed if a future requirement adds ISR reads of `s_state`.

> **DECISION-I (no lock on registration table or s_active_svc):** All writers and readers of the registration table and `s_active_svc` are on a single sequential path (caller thread pre-start, or nfc_work_q). The WQ is single-threaded. The ISR never touches router state. The HAL work-queue design guarantees field-off events are processed before new DATA_IND events in any session boundary. No spinlock is needed. This matches Wave 2's Pattern A justification and is the correct use of the "WQ thread only" data-path model.

---

## 5. Integration Points

### 5.1 Down — Functions Called by the Router

```c
/* ─── Response transmission (HAL, Wave 1 verbatim) ─────────────────────────── */
/* @threadsafe — borrows buf until next t4t callback; static const bufs are safe. */
/* Router calls: router_send_sw(s_sw_file_not_found, 2U); */
/* Router calls: router_send_sw(s_sw_command_not_allowed, 2U); */
int nfc_transport_send_response(const uint8_t *buf, size_t len);
```

No other HAL or framing functions are called by the router. The router does not call any framing functions — it is above framing.

### 5.2 Up — Framing Call Sites (Wave 2 Verbatim + DECISION-7 Resolution)

In `apdu_assembler_on_apdu_handler` (Wave 2 §8, task 10 `TODO Wave 3` comment), the locked call sites are:

```c
/* ─── Normal APDU dispatch (DECISION-7: parsed form) ──────────────────────── */
/* Called from on_apdu_handler on nfc_work_q after successful parse.
 * apdu is a stack-local nfc_apdu_t; apdu.data points into the net_buf.
 * net_buf_unref(buf) happens AFTER this returns. */
void aid_router_dispatch(const nfc_apdu_t *apdu);
    /* framing calls: aid_router_dispatch(&apdu); */

/* ─── Field-off notification ────────────────────────────────────────────────── */
/* Called from on_field_off_handler on nfc_work_q. */
void aid_router_field_off(void);
    /* framing calls: aid_router_field_off(); */
```

Wave 2's `on_field_off_handler` (task 9, currently `/* TODO Wave 3: aid_router_field_off(); */`) becomes:
```c
static void apdu_assembler_on_field_off_handler(void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    STATS_INC(&s_stats_lock, s_stats, field_off_count);
    aid_router_field_off();   /* ← replaces the TODO */
}
```

Wave 2's `on_apdu_handler` success path (task 10, `/* TODO Wave 3 */`) becomes:
```c
/* success path, before net_buf_unref: */
aid_router_dispatch(&apdu);
```

**Wave 2 framing header inclusion in `apdu_assembler.c`:** Add `#include "router/aid_router.h"` (includes `service.h` transitively via `aid_router.h`). The framing layer already includes `apdu_types.h`; `aid_router.h` adds the two router entry points.

> **NOTE:** Framing including `router/aid_router.h` creates a downward→upward dependency. Framing (below router in the stack) calls into the router (above it). This is the **Pattern A upward call via fixed singleton** per CONVENTIONS §3: "framing → router (complete APDU) | up | A direct call to fixed singleton". This is correct and pre-approved by the coupling map.

### 5.3 Services — Consumers of `service.h` (Wave 5 contracts)

Five Wave 5 service agents build against `service.h` in parallel. The vtable contract they must satisfy:

| Callback | Required | Sends response from? | Notes |
|----------|---------|---------------------|-------|
| `on_select` | **Yes** | `nfc_transport_send_response()` inside callback (or deferred WQ work for Aliro) | Router does not send 9000 |
| `on_apdu` | **Yes** | `nfc_transport_send_response()` inside callback | Copy `apdu->data` if needed post-return |
| `on_deselect` | **Yes** (may be no-op) | None | Reset session state |
| `on_field_off` | **Yes** (may be no-op) | None | Reset session state |
| `serialize` | Optional (NULL if not persistable) | N/A | @caller_sync; see §1.1 typedef |
| `deserialize` | Optional (NULL if not persistable) | N/A | @caller_sync |
| `persist_id` | 0 if not persistable; else from §1.1 table | N/A | Must be unique |
| `user_ctx` | Any (NULL OK for singletons) | N/A | Passed unchanged to all callbacks |

**Aliro special case:** Two AID registrations for one `nfc_service_t` instance. The service distinguishes expedited vs. step-up phases from the `aid` argument in `on_select`. The router has no knowledge of this; it calls `on_select` with whatever AID was in the SELECT.

**Ultralight special case:** Registers no AID (no `aid_router_register` call). Ultralight is not an ISO-DEP applet; it provides data to the NDEF service via a service-level API. Not a router concern.

### 5.4 `nfc_stack` — Wave 4 Orchestration

Wave 4 (`nfc_stack.c`) is responsible for all cross-layer wiring (CONVENTIONS §3). Expected usage:

```c
/* Initialization sequence in nfc_stack.c: */
(void)aid_router_init();
(void)apdu_assembler_init(NULL);
(void)nfc_transport_init(NULL);
/* nfc_stack registers its own wrapper ops, chaining to apdu_assembler_get_ops()
 * at runtime (wave4-stack.md §6 DECISION-1): */
(void)nfc_transport_register_callbacks(&s_transport_ops, NULL);
/* Register default profile services: */
(void)aid_router_register(ndef_aid, sizeof(ndef_aid), &ndef_service);
/* ... other services per active profile ... */
(void)nfc_transport_start(&initial_uid);

/* Teardown sequence: */
(void)nfc_transport_stop();     /* drains WQ; no more dispatch callbacks */
(void)aid_router_shutdown();
(void)apdu_assembler_shutdown();
(void)nfc_transport_shutdown();

/* Profile switching (CONVENTIONS §8) — on nfc_work_q, at field-off: */
aid_router_clear();
(void)aid_router_register(new_profile_aid, len, &new_svc);
/* ... register all services for the new profile ... */
```

**Profile switching threading detail (CONVENTIONS §8):** `nfc_stack_set_profile()` sets an `atomic_t` pending-profile and returns immediately (`@threadsafe`). The pending profile is applied on `nfc_work_q` inside nfc_stack's field-off wrapper handler, after the framing chain (which calls `aid_router_field_off()`) has completed — see wave4-stack.md §6 DECISION-1.

> **DECISION-J (profile switch mechanism at field-off):** `aid_router_field_off()` is called by framing (direct call, coupling map row 2). Profile switching requires calling `aid_router_clear()` + `aid_router_register()`, and that logic belongs in `nfc_stack.c` (CONVENTIONS §3 wiring rule). The mechanism: `nfc_stack.c` wraps the framing ops — it registers its own `nfc_transport_ops_t` with the HAL, whose `on_field_off` handler first chains to `apdu_assembler_get_ops()->on_field_off()` (which calls `aid_router_field_off()`), then applies any pending profile. This keeps all wiring in `nfc_stack.c` (designed and locked as wave4-stack.md DECISION-1; spec v2 §7). **Wave 3's contract:** `aid_router_field_off()` is a standalone function callable from nfc_work_q; `aid_router_clear()` + `aid_router_register()` are also callable from nfc_work_q for profile switching. The exact call order is Wave 4's concern.

### 5.5 Store — Wave 6 `serialize`/`deserialize` Consumption

The store layer (Wave 6) calls:
```c
svc->serialize(out, max, &out_len, svc->user_ctx);
svc->deserialize(in, len, svc->user_ctx);
```
These are called from `nfc_store_save`/`nfc_store_load` on the caller thread (`@caller_sync`), NOT from the WQ. The store entry points (`nfc_stack_save`/`nfc_stack_load`) return `-EBUSY` while the stack is STARTED (spec v2 §6.5), so the hooks are never invoked concurrently with `nfc_work_q` dispatch — services need no internal locking between their data model and `serialize`/`deserialize`. The store uses `svc->persist_id` for TLV framing (1-byte tag, 2-byte length, blob body). Services with `serialize == NULL` are silently skipped by the store.

### 5.6 Shell (`aid_router_shell_cmds.c`, CONVENTIONS §10)

```c
SHELL_STATIC_SUBCMD_SET_CREATE(aid_router_cmds,
    SHELL_CMD(config, NULL, "Print config (max_aids)",             cmd_aid_router_config),
    SHELL_CMD(stats,  NULL, "Print stats (all counters)",          cmd_aid_router_stats),
    SHELL_CMD(state,  NULL, "Print lifecycle state",               cmd_aid_router_state),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(aid_router, &aid_router_cmds, "NFC AID router control", NULL);
```

`config` — prints `max_aids`.
`stats` — `aid_router_get_stats(&snap)` → `shell_print` all 8 fields. Includes `select_matched_count`, `select_unmatched_count`, `apdu_routed_count`, `apdu_no_service_count`, `field_off_count`, `register_table_full_count`, `error_count`, `last_error_code`.
`state` — `aid_router_get_state()` → switch with explicit `default` → string.

**Extended shell output (observability):** The `stats` command prints the counters from a spinlock-guarded snapshot (`aid_router_get_stats()` copy-out into a stack-local — never a direct `s_stats` read). It also prints the current registration table (AIDs + service pointers) and the active service pointer, read directly from `s_table` on the shell (caller) thread. Because a profile switch may be rewriting the table on `nfc_work_q` at the same moment, this table display can show a **torn or stale view during a concurrent WQ profile switch — this is acknowledged and benign (display-only diagnostics; nothing is dereferenced beyond the fixed-size array, and the dispatch path never uses the shell's view)**. Format: hex AID per row.

---

## 6. Implementation Notes

### AID Matching Semantics

> **DECISION-K (exact-length AID match only):** ISO 7816-4 §11.2.2 allows SELECT DF Name with partial (prefix) matching (P2 byte controls first/last/only occurrence). Spec v2 §6.3 is silent on partial matching. This stack uses **exact-length matching only**: both `aid_len` values must be equal AND all bytes must match (`memcmp`). Rationale: (a) all registered AIDs are exact (NDEF=7, DeSFire=7, EMV=14, Aliro=9 bytes); (b) partial matching would allow a reader sending a truncated AID to accidentally match, creating unpredictable behavior; (c) exact matching is simpler, has no ambiguity, and is correct for all known consumers. If future services require partial matching, this decision must be revisited with an explicit `aid_router_register_partial` variant.

**AID lengths for reference (all registered services):**

| Service | AID (hex) | Length |
|---------|-----------|--------|
| NDEF | `D2 76 00 00 85 01 01` | 7 |
| DeSFire | `D2 76 00 00 85 01 00` | 7 |
| EMV PPSE | `32 50 41 59 2E 53 59 53 2E 44 44 46 30 31` | 14 |
| Aliro expedited | `A0 00 00 08 58 01 01 01 00` | 9 |
| Aliro step-up | `A0 00 00 08 58 01 01 02 00` | 9 |

Total: 6 entries (EMV registers 2 AIDs: PPSE + payment app AID). `CONFIG_NFC_ROUTER_MAX_AIDS` default 8 provides 2 slots of headroom.

### SELECT AID Detection — Exact Field Check

The router detects SELECT-by-AID via:
```c
(apdu->cla == NFC_CLA_ISO7816 (0x00)) &&
(apdu->ins == NFC_INS_SELECT  (0xA4)) &&
(apdu->p1  == NFC_SELECT_P1_BY_AID (0x04)) &&
(apdu->lc  > 0U)
```

An APDU with `cla=0x00, ins=0xA4, p1=0x04, lc=0` is a malformed SELECT (no AID) — treated as no-match (row F → 6A82). P2 byte is not checked; the router accepts any P2 for SELECT-by-AID (first/last/only occurrence distinction is reader concern, not the card's).

DeSFire-native (`cla=0x90`) and Aliro auth (`cla=0x80`) APDUs are non-SELECT and fall through to row H (forwarded to active service). CLA check is solely for SELECT detection; the service handles its own CLA validation for forwarded APDUs.

### SELECT Response Ownership — Services Send Their Own

Per DECISION-B: the service's `on_select` callback is solely responsible for sending the SELECT response. The router calls `on_select` then returns — it never calls `nfc_transport_send_response` on behalf of a successful SELECT. Services are documented in `service.h` to send a response from `on_select` (synchronously or via deferred WQ work for Aliro). If a service fails to send a response, the reader stalls until FWI timeout (~5 s at FWI=8). This is a service implementation bug.

**Implication for NDEF service:** `on_select` must send `{ 0x90, 0x00 }` (9000 OK).  
**Implication for DeSFire service:** `on_select` must send `{ 0x90, 0x00 }` (or appropriate response).  
**Implication for EMV service:** `on_select` must send the PPSE FCI TLV blob + `{ 0x90, 0x00 }`.  
**Implication for Aliro service:** `on_select` must send SELECTResponse (protocol versions + card info) + `{ 0x90, 0x00 }` or deferred.

### net_buf Lifetime in Dispatch Chain

Verbatim from Wave 2 §6 (zero-copy parse view validity window):

```
on_apdu(buf) called — framing owns buf (ref count = 1)
  │
  ├─ apdu_parse(buf->data, buf->len, ...) → nfc_apdu_t apdu (stack-local)
  │         apdu.data → &buf->data[offset]
  │
  ├─ aid_router_dispatch(&apdu)           ← buf->data still valid
  │     ├─ SELECT: apdu->data[0..lc-1] = AID bytes — valid here
  │     ├─ svc->on_select(aid, aid_len)   ← service reads AID — valid here
  │     │   OR
  │     ├─ svc->on_apdu(&apdu, ctx)       ← apdu->data valid here
  │     │   service must COPY any apdu->data bytes it needs after on_apdu returns
  │     └─ returns
  │
  └─ net_buf_unref(buf)   ← ALL pointers into buf->data are INVALID after this
```

This constraint is documented in `service.h` (`on_apdu` Doxygen) and must be restated in Wave 5 service plans.

### MISRA C:2012 Decisions + DEV-ZEP Citations

1. **`memcmp` in `find_aid`** — MISRA Dir 4.12 (no dynamic allocation) satisfied: no alloc. `memcmp` is a standard library function permitted by MISRA; returns `int`, must be checked (Rule 17.7) — but here `== 0` comparison is the check, so `(memcmp(...) == 0)` is compliant.
2. **`LOG_*` macros** — **DEV-ZEP-008** (Rule 20.1 / Rule 19.10; variadic logging).
3. **`shell_print` variadic macro** in `aid_router_shell_cmds.c` — **DEV-ZEP-009**.
4. **`ARG_UNUSED`** in shell command handlers (`argc`, `argv` unused) — **DEV-ZEP-016** (Rule 2.7).
5. **No `CONTAINER_OF`** — router does not recover parent structs from work items (HAL owns WQ); DEV-ZEP-001 not needed.
6. **No `k_fifo_put/get`** — router does not touch the APDU fifo (HAL-owned); DEV-ZEP-005 not needed.
7. **`K_SPINLOCK` scoped macro** NOT used — `STATS_*` macros use explicit `k_spin_lock/unlock`. DEV-ZEP-014 not needed.
8. **MISRA Rule 7.2** (`U` suffix): all unsigned literals — `0U`, `0x00U`, `0xFFU`, `2U`, `NFC_AID_MAX_LEN 16U`, shift amounts.
9. **MISRA Rule 9.1** (initialize at declaration): `aid_router_entry_t entry = {0};` if used as a local; `nfc_apdu_t apdu = {0}` in framing (Wave 2 done). The router's table is a file-static array — zero-initialized at program start (C standard). `s_active_svc` initialized to `NULL` at declaration.
10. **MISRA Rule 14.4** (Boolean controlling expressions): `(apdu->lc > 0U)`, `(rc != 0)`, `(s_active_svc != NULL)`, `(s_table_count < s_config.max_aids)`.
11. **MISRA Rule 16.4** (switch default): all `switch` statements in shell cmds and in dispatch switch (if any) have `default` arms.
12. **MISRA Rule 17.7** (return values): `nfc_transport_send_response` returns `int` — must not be silently discarded; router checks it in `router_send_sw()` and records `STATS_ERROR`. `aid_router_register` returns `int` — caller (nfc_stack) must check. `aid_router_init` / `aid_router_shutdown` return `int` — caller must check.
13. **MISRA Dir 4.12** (no dynamic allocation): no `malloc`/`free`; all state is file-static; table is a fixed-size array. Fully compliant.
14. **MISRA Rule 10.1/10.3** (integer type compatibility): `uint8_t aid_len` comparisons to `NFC_AID_MAX_LEN (uint8_t)`; `size_t` vs `uint8_t` cast explicitly where needed: `(uint8_t)apdu->lc` after range check.
15. **MISRA Rule 11.5** (`void *` conversion via service vtable `user_ctx`): `user_ctx` is stored and passed as `void *`; the service is responsible for casting it to its own type internally. This is the canonical callback context pattern (CALLBACK_REGISTRATION_GUIDE); covered by the conventions' pre-approval of Pattern A. If the cppcheck tool flags it, suppress under the same DEV-ZEP-001 class rationale.

---

## 7. Conventions Compliance

- [x] **Lifecycle matches §2** — Minimal (init/shutdown only); `shutdown` not `deinit`; `aid_router_state_t` has only UNINITIALIZED and INITIALIZED (no STARTED/STOPPED/ERROR for minimal lifecycle). `aid_router` row in CONVENTIONS §2 table: ✓ matched.
- [x] **`config_t`/`stats_t`/`state_t` + three getters defined (§2)** — `aid_router_config_t` (§1.2), `aid_router_stats_t` (§1.3), `aid_router_state_t` (§1.4) all defined. `get_config` never NULL (`@isr_safe`); `get_stats` copy-out (`@threadsafe`); `get_state` never fails (`@caller_sync`).
- [x] **State storage Pattern A per §2** — `static aid_router_state_t s_state`; WQ reads defensively; lifecycle writes on caller thread; no ISR state reads; no `atomic_t` needed. CONVENTIONS §2 `router/aid_router` row: "Pattern A plain enum (WQ thread only)".
- [x] **Coupling matches §3; callbacks follow §4** — Service vtable is Pattern A (registered by nfc_stack.c, not by aid_router); `user_ctx` named correctly; callbacks NULL-checked before invoking (`if (svc->on_field_off != NULL)`); dispatch thread (`nfc_work_q`) documented in `service.h`; `_fn` suffix on all function-pointer typedefs; no `_fn` typedef for `aid_router_dispatch` (it is a direct singleton call per coupling map).
- [x] **Buffer model §5** — Router receives a zero-copy `nfc_apdu_t` view (borrowed pointer; framing owns the net_buf); router never holds or copies the APDU past dispatch. Response buffers (`s_sw_file_not_found`, `s_sw_command_not_allowed`) are file-static const — borrow-until-next-callback satisfied. No net_buf usage in the router. Static outbound buffers are service-owned (CONVENTIONS §5).
- [x] **Stats recipe §6** — `static aid_router_stats_t s_stats`; `static struct k_spinlock s_stats_lock` (NOT `K_SPINLOCK_DEFINE`); `STATS_RESET(s_stats)` as first statement after `-EALREADY` guard in `init()`; `STATS_INC` for every named counter; `STATS_ERROR` for send_response failures; `STATS_COPY_OUT` in getter; `error_count` + `last_error_code` present.
- [x] **Threading + annotations §7** — `@caller_sync` for `init`/`shutdown`/`register`; `@nfc_work_q` for `dispatch`/`field_off`; `@caller_sync or @wq` for `clear` (with documented invariant); `@isr_safe` for `get_config`; `@threadsafe` for `get_stats`; `@caller_sync` for `get_state`. No sleep, no alloc, no blocking in dispatch/field-off. Thread ownership of all state documented in §4.3.
- [x] **Error codes §9; shell §10** — Standard errno only (`-EALREADY`, `-EINVAL`, `-ENODEV`, `-ENOMEM`); ISO 7816 6A82 / 6986 are protocol responses sent via `nfc_transport_send_response`, never C return codes; `aid_router` shell with `config`/`stats`/`state` in dedicated `aid_router_shell_cmds.c`.
- [x] **MISRA notes + DEV-ZEP citations §11** — DEV-ZEP-008/009/016 cited; DEV-ZEP-001/005/014 noted as not needed; Rules 7.2, 9.1, 10.1/10.3, 11.5, 14.4, 16.4, 17.7, Dir 4.12 all addressed in §6.

---

## 8. Tasks

> Granularity: 2–5 min/step. TDD applies to the **pure-logic helpers** (`apdu_is_select_by_aid`, `find_aid`, dispatch decision table cases) — these have no Zephyr kernel dependencies and are fully testable on `qemu_cortex_m33` or DK. Test path: `tests/unit/aid_router/` (standalone `CMakeLists.txt` + `prj.conf` + `testcase.yaml`, built `--no-sysbuild`; Twister tag `sigmation.nfc.aid_router`). `native_sim` is Linux-CI-only in this repo. Commit after each numbered group.

- [ ] **1. Scaffolding.**
  Create `src/nfc/router/CMakeLists.txt`:
  ```cmake
  if(CONFIG_NFC_STACK)
    target_sources(app PRIVATE
      aid_router.c
      aid_router_shell_cmds.c)
    target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  ```
  Create `src/nfc/router/Kconfig` with `NFC_ROUTER_MAX_AIDS` (int, default 8, range 1..32, depends on NFC_STACK). Create `tests/unit/aid_router/` with minimal `CMakeLists.txt` + `prj.conf` + `testcase.yaml` (built `--no-sysbuild`; compile `aid_router.c` directly into test). **Commit.**

- [ ] **2. `service.h`.**
  Write `src/nfc/router/service.h` per §1.1:
  - Include guard `SRC_NFC_ROUTER_SERVICE_H`.
  - `#include <stdint.h>`, `#include <stddef.h>`, `#include "framing/apdu_types.h"`.
  - All six callback typedefs (`_fn` suffix) with full Doxygen.
  - `nfc_service_t` struct with core callbacks + persistence fields + `persist_id` + `user_ctx`.
  - `NFC_AID_MAX_LEN 16U`.
  - `NFC_PERSIST_ID_*` constants.
  - Doxygen on `nfc_service_t` explaining DECISION-A (extension), thread annotations, data lifetime constraint on `apdu->data`.
  **Commit.**

- [ ] **3. `aid_router.h`.**
  Write `src/nfc/router/aid_router.h` with include guard. Includes: `service.h`, `<stdint.h>`, `<stddef.h>`. Memory comment at top. Define `aid_router_config_t` + `AID_ROUTER_MAX_AIDS_DEFAULT`; `aid_router_stats_t` (8 fields); `aid_router_state_t`; all 9 public prototypes with `@`-annotations and Doxygen. Note DECISION-7 resolution in `aid_router_dispatch` Doxygen. Note DECISION-J in `aid_router_field_off` Doxygen.
  **Commit.**

- [ ] **4. (TDD) SELECT detection helper.**
  In `aid_router.c`, write and test the pure static helper:
  ```c
  static bool apdu_is_select_by_aid(const nfc_apdu_t *apdu);
  ```
  In `tests/unit/aid_router/test_aid_router.c`:
  - `test_select_by_aid_basic`: `{cla=0x00, ins=0xA4, p1=0x04, lc=7}` → true.
  - `test_select_wrong_cla`: `{cla=0x80, ins=0xA4, p1=0x04, lc=7}` → false.
  - `test_select_wrong_ins`: `{cla=0x00, ins=0xB0, p1=0x04, lc=7}` → false.
  - `test_select_wrong_p1`: `{cla=0x00, ins=0xA4, p1=0x00, lc=7}` → false (by-file-id, not by-AID).
  - `test_select_zero_lc`: `{cla=0x00, ins=0xA4, p1=0x04, lc=0}` → false (empty AID).
  - `test_not_select`: typical non-SELECT APDU → false.
  All pass before continuing. **Commit.**

- [ ] **5. (TDD) AID matching helper.**
  In `aid_router.c`, write and test:
  ```c
  static const aid_router_entry_t *find_aid(const uint8_t *aid, uint8_t aid_len);
  ```
  (Requires `s_table`, `s_table_count` in scope — compile `aid_router.c` into tests with a test harness that seeds the table.)
  Tests:
  - `test_find_aid_match`: register one 7-byte AID; find with exact same bytes → non-NULL.
  - `test_find_aid_no_match`: search for different 7-byte AID → NULL.
  - `test_find_aid_len_mismatch`: same bytes but different length → NULL.
  - `test_find_aid_empty_table`: `s_table_count=0` → NULL.
  - `test_find_aid_multiple`: register 3 AIDs; find middle one → correct entry.
  - `test_find_aid_aliro_two`: register Aliro expedited (9 bytes) + step-up (9 bytes); find each → correct distinct entries.
  All pass. **Commit.**

- [ ] **6. (TDD) Dispatch decision table — mock harness.**
  Set up a minimal test harness that:
  - Mocks `nfc_transport_send_response` (captures calls, records `buf`/`len`).
  - Exposes a `test_service_t` that records which callbacks were called (using a file-static log).
  - Seeds `s_table` and `s_state` directly (test-only accessors or `aid_router_init()` + `aid_router_register()`).

  Tests:
  - `test_dispatch_select_matched_no_prior`: dispatch SELECT matching svc1 → `on_select` called, no `nfc_transport_send_response` from router.
  - `test_dispatch_select_matched_with_prior`: svc1 active, dispatch SELECT matching svc2 → `svc1.on_deselect` called then `svc2.on_select` called.
  - `test_dispatch_select_same_service`: svc1 active, dispatch SELECT matching svc1 again → `on_deselect` then `on_select` on svc1.
  - `test_dispatch_select_unmatched`: dispatch SELECT for unknown AID → router sends 6A82; no service callback.
  - `test_dispatch_select_zero_lc`: SELECT with `lc=0` → router sends 6A82.
  - `test_dispatch_non_select_with_service`: svc1 active, dispatch READ_BINARY APDU → `svc1.on_apdu` called.
  - `test_dispatch_non_select_no_service`: no active service, dispatch non-SELECT → router sends 6986.
  - `test_field_off_with_service`: svc1 active, call `aid_router_field_off()` → `svc1.on_field_off` called; `s_active_svc = NULL`.
  - `test_field_off_no_service`: no active service, call `aid_router_field_off()` → no crash; `field_off_count` incremented.
  All pass. **Commit.**

- [ ] **7. Module statics + lifecycle.**
  In `aid_router.c`:
  - Define all §4.1 statics (in order: `s_state`, `s_config`, `s_stats`, `s_stats_lock`, `s_table`, `s_table_count`, `s_active_svc`, `s_sw_*`).
  - Build `s_sw_command_not_allowed` from `NFC_SW_COMMAND_NOT_ALLOWED` (in `framing/apdu_types.h`) via `NFC_SW1`/`NFC_SW2`.
  - `LOG_MODULE_REGISTER(aid_router, CONFIG_LOG_DEFAULT_LEVEL)`.
  - Implement `aid_router_init()`:
    1. `if (s_state != AID_ROUTER_STATE_UNINITIALIZED) { return -EALREADY; }`
    2. `STATS_RESET(s_stats);`
    3. `s_config.max_aids = AID_ROUTER_MAX_AIDS_DEFAULT;`
    4. Clear `s_table_count`, `s_active_svc`.
    5. `s_state = AID_ROUTER_STATE_INITIALIZED; return 0;`
  - Implement `aid_router_shutdown()`: clear table + active; set state UNINIT; return 0.
  **Commit.**

- [ ] **8. Getters.**
  Implement `aid_router_get_config` (`return &s_config`), `aid_router_get_stats` (`return STATS_COPY_OUT(...)`), `aid_router_get_state` (`return s_state`).
  **Commit.**

- [ ] **9. `aid_router_register()` and `aid_router_clear()`.**
  Implement per §3 contracts:
  - `register`: guards (`-EINVAL`, `-ENODEV`, `-ENOMEM` with `register_table_full_count++`); `memcpy` AID; set `aid_len`, `svc`; `s_table_count++`.
  - `clear`: zero `s_table_count`; `s_active_svc = NULL`; memset table entries to 0 (defensive).
  **Commit.**

- [ ] **10. `router_send_sw()` helper and static helpers.**
  Implement the private `router_send_sw(const uint8_t *sw, size_t len)` (calls `nfc_transport_send_response`, records `STATS_ERROR` on failure, logs error). Implement `apdu_is_select_by_aid` and `find_aid` (already unit-tested). **Commit.**

- [ ] **11. `aid_router_dispatch()`.**
  Implement the full dispatch decision table per §3:
  - Defensive NULL check on `apdu` (row A).
  - Defensive state check (row B).
  - `apdu_is_select_by_aid` branch:
    - `lc == 0` → row F (6A82).
    - AID clamp: `aid_len = (uint8_t)MIN(apdu->lc, NFC_AID_MAX_LEN)`.
    - `find_aid` → row D/C/E (match) or row G (no match).
    - On match: call `on_deselect` on prior service if different; update `s_active_svc`; call `on_select(aid, aid_len, ctx)`.
  - Non-SELECT branch:
    - `s_active_svc != NULL` → row H: `s_active_svc->on_apdu(apdu, ctx)` with NULL check on the pointer.
    - `s_active_svc == NULL` → row I: `router_send_sw(s_sw_command_not_allowed, 2U)`.
  - All `STATS_INC` calls in the correct paths.
  **Commit.**

- [ ] **12. `aid_router_field_off()`.**
  Implement per §3 contract: `STATS_INC(field_off_count)`; NULL-guard `on_field_off`; invoke; `s_active_svc = NULL`. **Commit.**

- [ ] **13. Shell.**
  Write `aid_router_shell_cmds.c`:
  - `cmd_aid_router_config`: `get_config()` → print `max_aids`.
  - `cmd_aid_router_stats`: `get_stats(&snap)` → print all 8 fields from the copy-out snapshot; print registration table from `s_table` (note: the table display may be torn/stale during a concurrent WQ profile switch — benign, display-only; document this in a comment per §5.6).
  - `cmd_aid_router_state`: `get_state()` → `switch` with `default` → string.
  - `ARG_UNUSED(argc)`, `ARG_UNUSED(argv)` in each handler → DEV-ZEP-016.
  - `SHELL_STATIC_SUBCMD_SET_CREATE` + `SHELL_CMD_REGISTER` per §5.6.
  **Commit.**

- [ ] **14. Update Wave 2 framing call sites — replaces BOTH `TODO(Wave 3)` markers.**
  In `src/nfc/framing/apdu_assembler.c`, replace the two `TODO(Wave 3)` markers left by Wave 2 tasks 9 and 10:
  - Wave 2 task 9 `on_field_off_handler`: add `#include "router/aid_router.h"` to `apdu_assembler.c`; replace `/* TODO(Wave 3): aid_router_field_off(); */` with `aid_router_field_off();`.
  - Wave 2 task 10 `on_apdu_handler` success path: replace `/* TODO(Wave 3): aid_router_dispatch(&apdu); */` with `aid_router_dispatch(&apdu);`.
  Verify Wave 2 `on_apdu_handler` still unrefs `buf` after `aid_router_dispatch` returns. No change to unref discipline.
  **Commit.**

- [ ] **15. Build + lint.**
  Build with `CONFIG_NFC_STACK=y, CONFIG_NFC_SERVICE_NDEF=y, CONFIG_NFC_SERVICE_ALIRO=y` (exercising the 6-AID ALL profile). Run the ztest suite:
  ```
  west twister -T tests/unit/aid_router --platform qemu_cortex_m33 --no-sysbuild
  ```
  (`native_sim` is Linux-CI-only; use `qemu_cortex_m33` or DK locally.)
  Run:
  ```
  cppcheck --enable=all --addon=misra.py \
           --suppressions-list=misra-suppressions.txt \
           src/nfc/router/
  ```
  Verify DEV-ZEP-008/009/016 suppressions cover all flagged uses. Confirm no new deviations beyond those cited in §6. Document the `nfc_work_q` stack budget contribution from this layer (dispatch frame: `sizeof(nfc_apdu_t)` + router locals ≈ 32 B; negligible vs. the 2048 B HAL stack) in the HAL README. **Commit.**

---

### DECISION Flags Raised (for Human Review)

| # | Description | Section |
|---|-------------|---------|
| **DECISION-7** | `aid_router_dispatch` takes `const nfc_apdu_t *` (parsed form), not raw bytes. Spec v1 §6.3 raw form overridden (spec v2 states the parsed form; spec v3 §4.2 confirms ISO-DEP lane dispatches parsed APDUs). Wave 2 call site is `aid_router_dispatch(&apdu)`. | §DECISION-7 |
| **DECISION-A** | `nfc_service_t` extended beyond CONVENTIONS §4 base to add `serialize`, `deserialize`, `persist_id` (per wave guide mandate; Wave 5 must populate all). Per spec v3 §4.3 protocol modules own their (de)serialize contracts; per spec v2 §6.3 the fields are required. | §1.1 |
| **DECISION-B** | `on_select` is solely responsible for sending the SELECT response; the router sends nothing on a successful SELECT match. | §1.1 + §3 |
| **DECISION-C** | `NFC_SW_COMMAND_NOT_ALLOWED 0x6986` lives in Wave 2 `apdu_types.h` as a native status-word entry (proposed by this wave, user-approved); the router uses it directly — no local define. | §1.1 + §1.6 |
| **DECISION-D** | `aid_router_config_t` carries `max_aids` for observability (Kconfig default shadowed into the config struct). | §1.2 |
| **DECISION-E** | `aid_router_stats_t` renamed from spec `nfc_router_stats_t` (CONVENTIONS §2) and extended with `apdu_no_service_count` + `register_table_full_count`. | §1.3 |
| **DECISION-F** | `aid_router_dispatch` is `void` — no recoverable error for framing to act on; protocol outcomes are handled internally. | §2 |
| **DECISION-G** | `aid_router_field_off` is `void` — same rationale as DECISION-F. | §2 |
| **DECISION-H** | Oversized SELECT AID (`apdu->lc > NFC_AID_MAX_LEN`) clamped to 16 bytes for matching (practical no-match); not rejected structurally. | §3 |
| **DECISION-I** | No lock on registration table or `s_active_svc` — single-threaded WQ writer/reader; lifecycle writes pre-start on caller thread. Pattern A fully justified. | §4.4 |
| **DECISION-J** | Profile-switch mechanism at field-off (wave 4 concern): Wave 3 exports `aid_router_clear()` + `aid_router_register()` as WQ-callable; Wave 4 wires the nfc_stack field-off handler that sequences them. | §5.4 |
| **DECISION-K** | Exact-length AID matching only (no ISO 7816-4 partial/prefix matching). | §6 |
