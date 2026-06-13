# Wave 5a: NDEF Service Implementation Plan

**Status:** DRAFT — 2026-06-12

**Layer:** `services/ndef`

**Files produced:**
- `src/nfc/services/ndef/ndef_service.h`
- `src/nfc/services/ndef/ndef_service.c`
- `src/nfc/services/ndef/ndef_service_shell_cmds.c`
- `src/nfc/services/ndef/CMakeLists.txt`
- `src/nfc/services/ndef/Kconfig` (empty — `NFC_SERVICE_NDEF` and `NFC_NDEF_MAX_SIZE` live in `src/nfc/Kconfig` per wave4 §1.6)
- `tests/unit/nfc_ndef/` (ztest suite: T4T file state machine, all APDU handlers, serialize/deserialize; Twister tag `sigmation.nfc.ndef`; `native_sim` is Linux-CI-only — use `qemu_cortex_m33` or DK locally)

**Depends on (all LOCKED — 2026-06-12):**
- `docs/superpowers/plans/wave1-hal.md` — `nfc_transport_send_response`, `NFC_TRANSPORT_MAX_RESPONSE_LEN`, `nfc_work_q`
- `docs/superpowers/plans/wave2-framing.md` — `nfc_apdu_t`, `apdu_types.h` status-word vocabulary, INS/CLA constants
- `docs/superpowers/plans/wave3-router.md` — `nfc_service_t` vtable (full form with `serialize`/`deserialize`/`persist_id`), `NFC_PERSIST_ID_NDEF`, `service.h`
- `docs/superpowers/plans/wave4-stack.md` — `ndef_service_init/shutdown/get` expected surface (§5.2), `NFC_SERVICE_NDEF` / `NFC_NDEF_MAX_SIZE` Kconfig (§1.6), DECISION-6 (Ultralight uses NDEF AID via `ndef_service_get()`)

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 minimal lifecycle, §3 coupling/wiring rule, §4 callback pattern, §5 buffer model, §6 stats recipe, §7 threading + annotations, §9 errno, §10 shell, §11 MISRA, §12 compliance checklist)
2. `docs/superpowers/plans/wave3-router.md` §1 — `service.h` vtable, persist-ID table, `NFC_PERSIST_ID_NDEF=0x01`, persistence hook contracts and concurrency rule
3. `docs/superpowers/plans/wave4-stack.md` §5.2 — expected `ndef_service_init/shutdown/get` surface; §1.6 — `NFC_SERVICE_NDEF`, `NFC_NDEF_MAX_SIZE` in `src/nfc/Kconfig`; DECISION-6 — Ultralight registration (confirmed by DECISION-7 below)
4. `docs/nfc/archive/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) §6.4.1 — NDEF service scope, commands, data model; §6.4.3 — Ultralight injection pattern; §6.5 — store/persistence scope; §13 — non-goals
5. `docs/superpowers/plans/wave1-hal.md` §2 — `nfc_transport_send_response` contract, borrow-until-next-event, `NFC_TRANSPORT_MAX_RESPONSE_LEN`
6. `docs/superpowers/plans/wave2-framing.md` §1 — `nfc_apdu_t` fields and Le=0 semantics, `NFC_SW_*` vocabulary, `NFC_INS_SELECT / READ_BINARY / UPDATE_BINARY`, `NFC_SELECT_P1_BY_FILE_ID`
7. `docs/API_DESIGN_CREED.md` — module lifecycle, Pattern A state, creed §9 (no static locals), FSM
8. `docs/CALLBACK_REGISTRATION_GUIDE.md` — `_fn`/`_cb`, `user_ctx`, guard/NULL-clear
9. `docs/STATS_API_DESIGN.md` + `src/stats.h` — `s_stats`/`s_stats_lock`, `STATS_*` macros, `STATS_RESET` after `-EALREADY` guard
10. `docs/STACK_SPEC.md` — downward=direct call; upward=vtable; wiring in `nfc_stack.c`
11. `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001 through DEV-ZEP-018
12. `/Users/majidfaroud/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012 rules
13. NFC Forum Type 4 Tag Technical Specification (T4T v2.0) — CC structure, file-select semantics, READ/UPDATE BINARY status codes

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (replaced prototype — patterns must not carry forward).

---

> **Architecture Framing — spec v3 (2026-06-12):** This service is the
> **NFCT/card-role first-slice** of the **NDEF** protocol module as defined by the
> [NFC Stack Architecture v3](../specs/2026-06-12-nfc-stack-architecture.md) (§4.3).
> A protocol module owns: data model (`s_ndef_file`, CC) · (de)serialize ·
> **listener** (this file, built under `CONFIG_NFC_ROLE_CARD`) · **poller**
> (reader role — RESERVED, not implemented in this slice).
> **Lane:** `NFC_LANE_ISO_DEP` — Type-4, dispatched via APDU framing (Wave 2) + AID
> router (Wave 3). **Kconfig:** `NFC_SERVICE_NDEF` enables this protocol module; the
> listener compiles under `CONFIG_NFC_ROLE_CARD` (orthogonal, Wave 1). Existing symbol
> names are unchanged. **Serialize completeness:** this service's output is classified
> `NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED`
> (wave6 canonical C symbols — wave6-store.md §1.7 / spec v3 §4.5).

---

## 1. Types and Constants

All public types live in `ndef_service.h`. Internal-only types (file-selection enum, CC content, internal helpers) live in `ndef_service.c`. `ndef_service.h` includes `router/service.h` (for `nfc_service_t`) and `framing/apdu_types.h` (for status-word constants used in inline documentation).

### 1.1 Protocol Constants

These constants are `#define`d in `ndef_service.c` (not exported — no consumer needs them outside this module):

```c
/* T4T file identifiers (NFC Forum T4T spec §7.x) */
#define NDEF_FILE_ID_CC    0xE103U   /* Capability Container file */
#define NDEF_FILE_ID_NDEF  0xE104U   /* NDEF data file */

/* CC file fixed content — 15 bytes (CC length = 0x000F) */
#define NDEF_CC_FILE_LEN   15U

/* NDEF file layout:
 *   offset 0–1 : NLEN (uint16_t big-endian) — length of NDEF message in bytes
 *   offset 2–N : NDEF message bytes (N = NLEN)
 * Total buffer: 2 + CONFIG_NFC_NDEF_MAX_SIZE bytes */
#define NDEF_FILE_BUF_SIZE  ((uint32_t)(2U + CONFIG_NFC_NDEF_MAX_SIZE))

/* Response buffer: worst case = entire NDEF file + 2-byte SW */
#define NDEF_RESP_BUF_SIZE  ((uint32_t)(NDEF_FILE_BUF_SIZE + 2U))

/* CC content layout field offsets (into the 15-byte CC) */
#define CC_OFF_CC_LEN_HI   0U    /* 0x00 */
#define CC_OFF_CC_LEN_LO   1U    /* 0x0F */
#define CC_OFF_MAP_VER     2U    /* 0x20 = NDEF mapping v2.0 */
#define CC_OFF_MLE_HI      3U    /* max R-APDU data bytes, high */
#define CC_OFF_MLE_LO      4U    /* max R-APDU data bytes, low */
#define CC_OFF_MLC_HI      5U    /* max C-APDU data bytes, high */
#define CC_OFF_MLC_LO      6U    /* max C-APDU data bytes, low */
#define CC_OFF_TLV_T       7U    /* 0x04 = NDEF File Control TLV tag */
#define CC_OFF_TLV_L       8U    /* 0x06 = NDEF File Control TLV length */
#define CC_OFF_FILE_ID_HI  9U    /* 0xE1 */
#define CC_OFF_FILE_ID_LO  10U   /* 0x04 */
#define CC_OFF_FSIZE_HI    11U   /* NDEF file size, high byte */
#define CC_OFF_FSIZE_LO    12U   /* NDEF file size, low byte */
#define CC_OFF_RD_ACCESS   13U   /* 0x00 = open read */
#define CC_OFF_WR_ACCESS   14U   /* 0x00 = open write; 0xFF = no write */

/* CC write-access encoding */
#define CC_WR_ACCESS_OPEN  0x00U
#define CC_WR_ACCESS_NONE  0xFFU

/* SELECT FILE P2 values accepted by this service */
#define NDEF_SELECT_P2_FIRST_NO_FCI  0x0CU   /* no response data, first/only occurrence */
#define NDEF_SELECT_P2_FIRST_FCI     0x00U   /* FCI requested — accepted; we return 9000 only */
```

> **CC MLe / MLc values:** MLe = `(uint16_t)(CONFIG_NFC_NDEF_MAX_SIZE + 2U)` (maximum readable in one READ BINARY shot, covering NLEN + full message). MLc = `(uint16_t)(CONFIG_NFC_NDEF_MAX_SIZE)` (maximum writable data field). These are compile-time constant expressions stored directly in the `k_cc_file` array initializer as `uint8_t` casts. `CONFIG_NFC_NDEF_MAX_SIZE` range is 16–4096 (wave4 §1.6); max MLe = 4098 ≤ 0xFFFF — fits in 2 bytes.

### 1.2 `NDEF_SERVICE_MAX_SERIALIZED` (used by Wave 6 store)

```c
/* Maximum bytes the serialize hook may write. Wave 6 sizes its blob buffer from this. */
#define NDEF_SERVICE_MAX_SERIALIZED  ((uint32_t)(2U + CONFIG_NFC_NDEF_MAX_SIZE))
```

Serialize writes exactly `2 + NLEN_value` bytes (NLEN big-endian + NDEF message). Worst case is `2 + CONFIG_NFC_NDEF_MAX_SIZE` when NLEN equals the maximum message size.

### 1.3 `ndef_service_config_t`

```c
/**
 * @brief NDEF service configuration. Frozen after ndef_service_init().
 *
 * Pass NULL to ndef_service_init() to use compile-time defaults.
 */
typedef struct {
    /**
     * Whether UPDATE BINARY is accepted for the NDEF file.
     * true  (default) → write access open; CC write-access byte = 0x00.
     * false           → UPDATE BINARY returns NFC_SW_CONDITIONS_NOT_SATISFIED (6985);
     *                   CC write-access byte = 0xFF (no write).
     * The CC file is ALWAYS read-only regardless of this flag.
     */
    bool writable;
} ndef_service_config_t;

#define NDEF_SERVICE_CONFIG_DEFAULT \
    ((ndef_service_config_t){ .writable = true })
```

### 1.4 `ndef_service_stats_t`

```c
typedef struct {
    /* Dispatch lifecycle */
    uint32_t select_app_count;            /**< on_select invocations (NDEF AID matched)     */
    uint32_t deselect_count;              /**< on_deselect invocations                      */
    uint32_t field_off_count;             /**< on_field_off invocations                     */

    /* SELECT FILE path */
    uint32_t select_file_cc_count;        /**< CC file successfully selected (→ 9000)       */
    uint32_t select_file_ndef_count;      /**< NDEF file successfully selected (→ 9000)     */
    uint32_t select_file_notfound_count;  /**< Unknown file ID → 6A82                       */
    uint32_t select_file_badlen_count;    /**< Lc ≠ 2 → 6700                               */
    uint32_t select_file_badparam_count;  /**< P1/P2 not accepted → 6A86                   */

    /* READ BINARY path */
    uint32_t read_ok_count;               /**< READ BINARY → 9000                          */
    uint32_t read_nofile_count;           /**< No file selected → 6986                     */
    uint32_t read_offset_count;           /**< Offset ≥ file length → 6B00                 */

    /* UPDATE BINARY path */
    uint32_t update_ok_count;             /**< UPDATE BINARY → 9000                        */
    uint32_t update_nofile_count;         /**< No file selected → 6986                     */
    uint32_t update_readonly_count;       /**< CC write attempt or !writable → 6985        */
    uint32_t update_overflow_count;       /**< offset + Lc > NDEF_FILE_BUF_SIZE → 6700     */
    uint32_t update_badlen_count;         /**< Lc == 0 → 6700                              */

    /* Unknown / unsupported commands */
    uint32_t apdu_unknown_ins_count;      /**< INS not in {0xA4, 0xB0, 0xD6} → 6D00       */
    uint32_t apdu_bad_cla_count;          /**< CLA ≠ 0x00 → 6E00                          */

    /* Mandatory (CONVENTIONS §6) */
    uint32_t error_count;                 /**< STATS_ERROR increments this                 */
    int32_t  last_error_code;             /**< Last code passed to STATS_ERROR             */
} ndef_service_stats_t;
```

> **CONVENTIONS §6 compliance:** every reject/drop path has a named counter. There are no silent drops — every APDU receives a response — but named counters enable shell observability and protocol debugging.

### 1.5 `ndef_service_state_t` — Pattern A

```c
typedef enum {
    NDEF_SERVICE_STATE_UNINITIALIZED = 0,  /**< Zero-init default; before init()  */
    NDEF_SERVICE_STATE_INITIALIZED,        /**< After init(); APDU dispatch active */
} ndef_service_state_t;
```

Minimal lifecycle per CONVENTIONS §2: only `UNINITIALIZED` / `INITIALIZED`. No `STARTED`/`STOPPED` — the service has no concept of start; it is active from `init()` until `shutdown()`. The stack's STARTED state gates `set_content` and `serialize`/`deserialize` at the caller level.

---

## 2. Public API

All public functions are declared in `ndef_service.h`. Header includes: `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `router/service.h`.

### 2.1 Lifecycle

```c
/**
 * @brief Initialize the NDEF service.
 *
 * Resets internal state (NLEN=0, file=empty, file selection=none).
 * Builds the CC file from compile-time constants and s_config.writable.
 * Safe to call after shutdown() for re-initialization.
 *
 * @param cfg  Configuration; NULL → NDEF_SERVICE_CONFIG_DEFAULT.
 * @retval  0        Success.
 * @retval -EALREADY Already INITIALIZED (call shutdown() first).
 * @retval -EINVAL   cfg->writable is out of bool range (unreachable with correct types).
 * @caller_sync
 */
int ndef_service_init(const ndef_service_config_t *cfg);

/**
 * @brief Shut down the NDEF service.
 *
 * Resets state to UNINITIALIZED. Idempotent — safe to call when already
 * UNINITIALIZED. Does not clear stats (stats survive shutdown for post-mortem).
 *
 * @retval 0  Always.
 * @caller_sync
 */
int ndef_service_shutdown(void);
```

> **Wave 4 §5.2 compatibility:** Wave 4 §5.2 already specifies `ndef_service_init(const ndef_service_config_t *cfg)` (NULL → defaults). **DECISION-9:** `nfc_stack.c` calls `ndef_service_init(NULL)` and the service applies `NDEF_SERVICE_CONFIG_DEFAULT`. This plan is fully compatible with the wave4 locked surface as specified.

### 2.2 Vtable Getter

```c
/**
 * @brief Return the service vtable pointer for AID router registration.
 *
 * Returns a pointer to the file-static nfc_service_t singleton. Valid before
 * init() — the struct is statically initialised. Used by nfc_stack.c:
 *   aid_router_register(k_ndef_aid, sizeof(k_ndef_aid), ndef_service_get());
 *
 * @return  Non-NULL const pointer to the service vtable.
 * @isr_safe  (reads only a statically-initialised const struct)
 */
const nfc_service_t *ndef_service_get(void);
```

### 2.3 Module-Contract Getters

```c
/**
 * @brief Return a pointer to the frozen config struct.
 * @return  Never NULL. Returns the zero-initialized default before init().
 * @threadsafe  (reads file-static const after init; write-once before any callers)
 */
const ndef_service_config_t *ndef_service_get_config(void);

/**
 * @brief Copy the current stats into caller-owned buffer.
 * @param out  Destination; must not be NULL.
 * @retval  0       Success.
 * @retval -EINVAL  out is NULL.
 * @threadsafe  (STATS_COPY_OUT under s_stats_lock)
 */
int ndef_service_get_stats(ndef_service_stats_t *out);

/**
 * @brief Return the current lifecycle state.
 * @return  NDEF_SERVICE_STATE_UNINITIALIZED or NDEF_SERVICE_STATE_INITIALIZED.
 * @threadsafe  (reads plain enum; only mutated by @caller_sync lifecycle functions)
 */
ndef_service_state_t ndef_service_get_state(void);
```

### 2.4 Content-Injection API

**DECISION-1 (shape, contract, and concurrency constraint):**

```c
/**
 * @brief Inject NDEF message content into the service's NDEF file buffer.
 *
 * Replaces the NDEF file's NLEN field and message body atomically (no partial
 * writes visible to the reader). Used by the Ultralight service adapter to
 * render its page model as a T4T NDEF message before stack start.
 *
 * The injected bytes are the raw NDEF message (TLV payload or full message
 * record bytes). The service prepends NLEN automatically; callers do NOT
 * pre-pend NLEN.
 *
 * Concurrency constraint: this function is @caller_sync. It directly
 * overwrites s_ndef_file (the same buffer that on_apdu READ/UPDATE BINARY
 * handlers access on nfc_work_q). If called while the stack is STARTED, a
 * data race exists — undefined behaviour. The caller MUST ensure the stack is
 * not STARTED before calling (call during provisioning, before nfc_stack_start(),
 * or after nfc_stack_stop()). This mirrors the serialize/deserialize contract
 * (CONVENTIONS §3, spec v2 §6.5: -EBUSY while STARTED).
 *
 * No internal locking is provided. The service cannot enforce the caller's
 * stack-stopped precondition without a circular dependency on nfc_stack.
 *
 * @param msg  NDEF message bytes (message only, no NLEN prefix).
 *             If NULL and len == 0, the NDEF file is cleared (NLEN=0).
 * @param len  Length of msg; must be ≤ CONFIG_NFC_NDEF_MAX_SIZE.
 * @retval  0        Success; NLEN set to len, message copied.
 * @retval -EINVAL   msg is NULL and len > 0, or len > CONFIG_NFC_NDEF_MAX_SIZE.
 * @retval -ENODEV   Service not INITIALIZED.
 * @caller_sync — stack MUST NOT be STARTED (see above).
 */
int ndef_service_set_content(const uint8_t *msg, size_t len);
```

> **DECISION-1 rationale:** The name `ndef_service_set_content` is preferred over `ndef_service_load_content` (matches the "set" verb convention for direct-write operations). The no-internal-locking choice mirrors `serialize`/`deserialize` — both operate on a quiesced stack. Adding a lock would require the service to know the stack's run state (upward coupling, forbidden by STACK_SPEC). The caller (Ultralight init path, provisioning shell commands) is the correct enforcement point; these callers know the stack state.

---

## 3. Contracts

### 3.1 `ndef_service_init(cfg)`

| Precondition | Effect |
|---|---|
| `s_state == UNINITIALIZED` | Proceeds |
| `s_state == INITIALIZED` | Returns `-EALREADY`; `STATS_RESET` **not** called (stats survive) |

**Normal path (state == UNINITIALIZED):**
1. Copy `cfg` (or defaults if NULL) into `s_config`. Freeze.
2. `STATS_RESET(s_stats)` — after the `-EALREADY` guard, before any other mutation (CONVENTIONS §6).
3. Zero `s_ndef_file[0..NDEF_FILE_BUF_SIZE-1]` — NLEN=0, message empty.
4. Set `s_file_sel = NDEF_FILE_NONE`.
5. Build `s_cc_file[0..14]` from compile-time constants + `s_config.writable` (CC_WR_ACCESS byte).
6. Set `s_state = NDEF_SERVICE_STATE_INITIALIZED`.
7. Return `0`.

### 3.2 `ndef_service_shutdown()`

| State | Effect |
|---|---|
| `UNINITIALIZED` | No-op; return `0` |
| `INITIALIZED` | Set `s_state = UNINITIALIZED`; zero `s_file_sel`; return `0` |

Stats are **not** reset on shutdown (post-mortem diagnostics).

### 3.3 `ndef_service_get()`

Always returns `&k_ndef_svc` (file-static const `nfc_service_t`). Safe before `init()`.

### 3.4 `ndef_service_get_config()` / `_get_stats()` / `_get_state()`

Standard getters per CONVENTIONS §2. `get_config()` returns `&s_config` (pointer valid forever; frozen after init). `get_stats()` uses `STATS_COPY_OUT`. `get_state()` reads `s_state` without a lock (Pattern A: only mutated by `@caller_sync` lifecycle functions).

### 3.5 `ndef_service_set_content(msg, len)`

| Condition | Return |
|---|---|
| `s_state != INITIALIZED` | `-ENODEV` |
| `msg == NULL && len > 0` | `-EINVAL` |
| `len > CONFIG_NFC_NDEF_MAX_SIZE` | `-EINVAL` |
| `msg == NULL && len == 0` | Clear: `memset(s_ndef_file, 0, NDEF_FILE_BUF_SIZE)` → `0` |
| Valid | Set `s_ndef_file[0] = (uint8_t)(len >> 8U)`, `s_ndef_file[1] = (uint8_t)(len & 0xFFU)`, `memcpy(&s_ndef_file[2], msg, len)`, zero-fill `s_ndef_file[2+len..NDEF_FILE_BUF_SIZE-1]` → `0` |

### 3.6 Vtable Callback Contracts

#### 3.6.1 `on_select(aid, aid_len, user_ctx)` — AID matched

Called on `nfc_work_q` when `aid_router_dispatch` matches the NDEF AID (`D2 76 00 00 85 01 01`).

| Action | Detail |
|---|---|
| Reset file selection | `s_file_sel = NDEF_FILE_NONE` |
| Increment stat | `select_app_count` |
| Build response | `s_response_buf[] = { NFC_SW1(NFC_SW_OK), NFC_SW2(NFC_SW_OK) }` |
| Send response | `nfc_transport_send_response(s_response_buf, 2U)` |

The SELECT AID response is always `9000`. No FCI data is returned for the NDEF application. `aid_len` is not validated — the router only dispatches after a full AID match.

#### 3.6.2 `on_apdu(apdu, user_ctx)` — Non-SELECT APDUs

Called on `nfc_work_q` for all non-SELECT APDUs while this service is selected.

**Top-level CLA and INS dispatch:**

| CLA | INS | Handler |
|-----|-----|---------|
| `!= 0x00` (any) | any | → respond `6E00`; increment `apdu_bad_cla_count` |
| `0x00` | `0xA4` (`NFC_INS_SELECT`) | → SELECT FILE handler (§3.6.2.1) |
| `0x00` | `0xB0` (`NFC_INS_READ_BINARY`) | → READ BINARY handler (§3.6.2.2) |
| `0x00` | `0xD6` (`NFC_INS_UPDATE_BINARY`) | → UPDATE BINARY handler (§3.6.2.3) |
| `0x00` | anything else | → respond `6D00`; increment `apdu_unknown_ins_count` |

All responses are built into `s_response_buf` and sent via `nfc_transport_send_response`.

##### 3.6.2.1 SELECT FILE handler (`INS=0xA4`)

Full decision table:

| Condition | SW | Stat counter |
|---|---|---|
| `P1 != 0x00` | `6A86` | `select_file_badparam_count` |
| `P2 not in {0x00, 0x0C}` | `6A86` | `select_file_badparam_count` |
| `apdu->lc != 2U` | `6700` | `select_file_badlen_count` |
| `apdu->data == NULL` | `6700` | `select_file_badlen_count` |
| `file_id == NDEF_FILE_ID_CC (0xE103)` | `9000`; `s_file_sel = NDEF_FILE_CC` | `select_file_cc_count` |
| `file_id == NDEF_FILE_ID_NDEF (0xE104)` | `9000`; `s_file_sel = NDEF_FILE_NDEF` | `select_file_ndef_count` |
| any other file ID | `6A82` | `select_file_notfound_count` |

`file_id` is extracted as `(uint16_t)((apdu->data[0] << 8U) | apdu->data[1])`.

> **DECISION-4:** `P2 == 0x00` (FCI requested, first occurrence) is accepted in addition to `P2 == 0x0C` (no FCI). The service always returns only the status word for SELECT FILE — no FCI. This provides compatibility with readers that send P2=0x00. Strictly, T4T v2.0 §7.2 mandates only P2=0x0C; accepting P2=0x00 is a deliberate interoperability relaxation.

##### 3.6.2.2 READ BINARY handler (`INS=0xB0`)

| Condition | SW | Stat counter |
|---|---|---|
| `s_file_sel == NDEF_FILE_NONE` | `6986` | `read_nofile_count` |
| Compute `offset = (uint16_t)((apdu->p1 << 8U) | apdu->p2)` | — | — |
| `offset >= file_length` | `6B00` | `read_offset_count` |
| Compute `ne` (see Le semantics below) | — | — |
| `ne == 0U` after clamping | `6B00` | `read_offset_count` |
| All valid | Append data + `9000` | `read_ok_count` |

**File selection and length:**
- `NDEF_FILE_CC`: `file_ptr = s_cc_file`, `file_length = NDEF_CC_FILE_LEN` (15)
- `NDEF_FILE_NDEF`: `file_ptr = s_ndef_file`, `file_length = NDEF_FILE_BUF_SIZE` (2 + `CONFIG_NFC_NDEF_MAX_SIZE`)

**Le / Ne semantics** (from `wave2-framing.md` §1):

| `has_le` | `extended` | `apdu->le` | Computed `ne` |
|----------|-----------|-----------|---------------|
| `false` | — | — | `file_length - offset` (read all available) |
| `true` | `false` | `0` | `256U` |
| `true` | `true` | `0` | `65536U` |
| `true` | any | `> 0` | `(uint32_t)apdu->le` |

`actual_bytes = MIN(ne, file_length - offset)`. Always `> 0` because `offset < file_length` (checked above).

**Response assembly:**
```
s_response_buf[0..actual_bytes-1] = file_ptr[offset..offset+actual_bytes-1]
s_response_buf[actual_bytes]      = NFC_SW1(NFC_SW_OK)
s_response_buf[actual_bytes+1U]   = NFC_SW2(NFC_SW_OK)
nfc_transport_send_response(s_response_buf, actual_bytes + 2U)
```

`actual_bytes + 2U ≤ NDEF_RESP_BUF_SIZE` by construction (CC 15+2=17 ≤ `CONFIG_NFC_NDEF_MAX_SIZE+4`; NDEF `NDEF_FILE_BUF_SIZE + 2 == NDEF_RESP_BUF_SIZE` exactly).

> **DECISION-5:** Short READ BINARY with Le=0 (Ne=256) is capped to `min(256, available_bytes)`. Extended READ BINARY with Le=0 (Ne=65536) is capped to `min(65536, available_bytes)`. In both cases actual response may be shorter than Ne — this is correct ISO 7816-4 / T4T behaviour (short read, not an error).

##### 3.6.2.3 UPDATE BINARY handler (`INS=0xD6`)

| Condition | SW | Stat counter |
|---|---|---|
| `s_file_sel == NDEF_FILE_NONE` | `6986` | `update_nofile_count` |
| `s_file_sel == NDEF_FILE_CC` | `6985` | `update_readonly_count` |
| `!s_config.writable` | `6985` | `update_readonly_count` |
| `apdu->lc == 0U` | `6700` | `update_badlen_count` |
| `apdu->data == NULL` | `6700` | `update_badlen_count` |
| Compute `offset = (uint16_t)((apdu->p1 << 8U) | apdu->p2)` | — | — |
| `(uint32_t)offset + (uint32_t)apdu->lc > NDEF_FILE_BUF_SIZE` | `6700` | `update_overflow_count` |
| All valid | `memcpy(&s_ndef_file[offset], apdu->data, apdu->lc)` + `9000` | `update_ok_count` |

**Update BINARY is a raw byte-level write.** NLEN management is the reader's responsibility (T4T standard protocol: reader writes NLEN=0, writes message, then writes final NLEN). The service writes blindly. The CC file (`s_file_sel == NDEF_FILE_CC`) is **always** rejected with `6985` regardless of `writable` — the CC is structurally read-only.

> **DECISION-2:** CC write-access field in the CC file is set at init time from `s_config.writable` (0x00 = open, 0xFF = no write). The CC file itself is unconditionally read-only regardless of this field: on_apdu always returns 6985 for UPDATE BINARY targeting the CC. The CC write-access field is informational for the reader only.

> **DECISION-10 (live persist — LOCKED 2026-06-13):** After a successful NDEF-file
> `UPDATE BINARY` (`update_ok_count++`), the ndef service notifies the stack
> (callback registered in `nfc_stack.c`) so `nfc_stack` can call
> `nfc_store_on_dirty(ndef_service_get(), active_tag)` on `nfc_work_q`.
> Cloned/emulated NDEF cards therefore persist reader-written content like a normal
> writable tag. The ndef service does **not** `#include` the store layer — wiring
> stays in `nfc_stack.c` per CONVENTIONS §3. Applies to `NFC_PROFILE_NDEF` only;
> Ultralight adapter behaviour is unchanged (`DECISION-UL-3`).

#### 3.6.3 `on_deselect(user_ctx)`

Resets `s_file_sel = NDEF_FILE_NONE`. Increments `deselect_count`. No response sent (router already dispatched the new SELECT).

#### 3.6.4 `on_field_off(user_ctx)`

Resets `s_file_sel = NDEF_FILE_NONE`. Increments `field_off_count`. No response sent.

> **Rationale:** NDEF file contents are persistent across field-off events — only per-session state (file selection) is cleared. NLEN and message data survive until `shutdown()` or `deserialize()`.

### 3.7 Persistence Hook Contracts

#### 3.7.1 `serialize(out, max, out_len, user_ctx)`

| Condition | Return |
|---|---|
| `out == NULL \|\| out_len == NULL` | `-EINVAL` |
| Compute `nlen = (uint16_t)((s_ndef_file[0] << 8U) \| s_ndef_file[1])` | — |
| `nlen > CONFIG_NFC_NDEF_MAX_SIZE` | `-EBADMSG` (NLEN corrupt) |
| `max < (size_t)(2U + nlen)` | `-ENOSPC` |
| All valid | `memcpy(out, s_ndef_file, 2U + nlen)` ; `*out_len = 2U + nlen` ; return `0` |

Serializes only the live NDEF data: 2 bytes NLEN (big-endian) + NLEN message bytes. Never more than `NDEF_SERVICE_MAX_SERIALIZED`.

> **DECISION-3:** Serialize writes `2 + NLEN_value` bytes (not the full `NDEF_FILE_BUF_SIZE`). This minimises blob size for common small NDEF messages (e.g. a URI record is ~30 bytes; a full 4096-byte buffer need not be written). Wave 6 allocates its TLV body buffer using `NDEF_SERVICE_MAX_SERIALIZED` as the worst-case size.

#### 3.7.2 `deserialize(in, len, user_ctx)`

| Condition | Return |
|---|---|
| `in == NULL \|\| len == 0U` | `-EINVAL` |
| `len < 2U` | `-EBADMSG` (cannot read NLEN) |
| Extract `nlen = (uint16_t)((in[0] << 8U) \| in[1])` | — |
| `nlen > CONFIG_NFC_NDEF_MAX_SIZE` | `-EBADMSG` |
| `len < (size_t)(2U + nlen)` | `-EBADMSG` (truncated blob) |
| `s_state != INITIALIZED` | `-ENODEV` |
| All valid | `memcpy(s_ndef_file, in, 2U + nlen)` ; zero-fill `s_ndef_file[2+nlen..NDEF_FILE_BUF_SIZE-1]` ; return `0` |

---

## 4. Internal State

### 4.1 File-Static Variables (`ndef_service.c`)

```c
/* Module config — frozen after init() */
static ndef_service_config_t  s_config;

/* Stats + spinlock */
static ndef_service_stats_t   s_stats;
static struct k_spinlock      s_stats_lock;

/* Lifecycle state (Pattern A — caller_sync only) */
static ndef_service_state_t   s_state;

/* NDEF file buffer: s_ndef_file[0..1] = NLEN (big-endian); s_ndef_file[2..] = NDEF msg */
static uint8_t  s_ndef_file[NDEF_FILE_BUF_SIZE];

/* CC file — built during init(); treated as read-only after */
static uint8_t  s_cc_file[NDEF_CC_FILE_LEN];

/* Per-session file selection (owned by nfc_work_q exclusively during STARTED) */
static ndef_file_sel_t  s_file_sel;

/* Static response buffer — borrowed by HAL until next callback (CONVENTIONS §5) */
static uint8_t  s_response_buf[NDEF_RESP_BUF_SIZE];

/* Service vtable singleton */
static const nfc_service_t k_ndef_svc = {
    .on_select    = ndef_on_select,
    .on_apdu      = ndef_on_apdu,
    .on_deselect  = ndef_on_deselect,
    .on_field_off = ndef_on_field_off,
    .serialize    = ndef_serialize,
    .deserialize  = ndef_deserialize,
    .persist_id   = NFC_PERSIST_ID_NDEF,    /* 0x01 per wave3 §1.1 */
    .user_ctx     = NULL,
};
```

### 4.2 `ndef_file_sel_t` (internal type, `.c` only)

```c
typedef enum {
    NDEF_FILE_NONE = 0,   /* Application selected; no EF selected yet    */
    NDEF_FILE_CC,         /* CC file (E103) selected                      */
    NDEF_FILE_NDEF,       /* NDEF file (E104) selected                    */
} ndef_file_sel_t;
```

### 4.3 Lifecycle State Machine

```
UNINITIALIZED ──init()──→ INITIALIZED ──shutdown()──→ UNINITIALIZED
                                │
                       (on_select, on_apdu, on_deselect, on_field_off
                        are only invoked in INITIALIZED state — the
                        router only routes to registered services, and
                        aid_router_register is called after init())
```

**State ownership:** `s_state` is mutated only by `@caller_sync` functions (`init`, `shutdown`). It is read by `get_state()` (`@threadsafe`) and by the vtable callbacks on `nfc_work_q` (read-only check in `ndef_service_set_content`). No lock is needed for `s_state` because it is Pattern A (CONVENTIONS §2).

### 4.4 Per-Session State Machine (within INITIALIZED)

```
[NDEF_FILE_NONE]
    │ on_select (AID) → send 9000 → [NDEF_FILE_NONE]
    │ on_deselect     → [NDEF_FILE_NONE]
    │ on_field_off    → [NDEF_FILE_NONE]
    │
    │ on_apdu SELECT FILE (E103) → 9000
    ├──────────────────────────────→ [NDEF_FILE_CC]
    │ on_apdu SELECT FILE (E104) → 9000
    ├──────────────────────────────→ [NDEF_FILE_NDEF]
    │ on_apdu SELECT FILE (bad)  → 6A82/6700/6A86; state unchanged
    │
[NDEF_FILE_CC]
    │ READ BINARY   → data + 9000 (CC bytes)
    │ UPDATE BINARY → 6985 (always)
    │ on_select (AID) → 9000; → [NDEF_FILE_NONE]
    │ on_deselect / on_field_off → [NDEF_FILE_NONE]
    │ SELECT FILE (any) → 9000 or error; transition per §3.6.2.1
    │
[NDEF_FILE_NDEF]
    │ READ BINARY   → data + 9000 (NDEF file bytes)
    │ UPDATE BINARY → 9000 (if writable) or 6985 (if !writable)
    │ on_select (AID) → 9000; → [NDEF_FILE_NONE]
    │ on_deselect / on_field_off → [NDEF_FILE_NONE]
    │ SELECT FILE (any) → 9000 or error; transition per §3.6.2.1
```

**Concurrency:** `s_file_sel` is owned exclusively by `nfc_work_q` during STARTED (all vtable callbacks run there; CONVENTIONS §7). `ndef_service_set_content` is `@caller_sync` and must not run concurrently (see §2.4 DECISION-1).

---

## 5. Integration Points

### 5.1 Down — This Layer Calls

| Function | From | Contract |
|---|---|---|
| `nfc_transport_send_response(buf, len)` | `on_select`, `on_apdu` handlers | `buf = s_response_buf`; HAL borrows until next callback; must not overwrite before then. `len ≤ NDEF_RESP_BUF_SIZE ≤ NFC_TRANSPORT_MAX_RESPONSE_LEN`. Defined in `hal/nfc_transport.h`. |

No other downward calls. This service is a leaf — it calls only the HAL response function.

### 5.2 Up — Callers of This Layer

| Caller | Function called | When |
|---|---|---|
| `nfc_stack.c` | `ndef_service_init(NULL)` | During `nfc_stack_init()`, after HAL/framing/router init, before first `aid_router_register` call |
| `nfc_stack.c` | `ndef_service_shutdown()` | During `nfc_stack_shutdown()`, after `nfc_transport_stop()` |
| `nfc_stack.c` | `ndef_service_get()` | Passed to `aid_router_register(k_ndef_aid, 7, ndef_service_get())` for `NFC_PROFILE_NDEF` and `NFC_PROFILE_ULTRALIGHT` (wave4 DECISION-6) |
| `aid_router` | `k_ndef_svc.on_select` | On `nfc_work_q` when NDEF AID matched in SELECT-by-AID APDU |
| `aid_router` | `k_ndef_svc.on_apdu` | On `nfc_work_q` for every non-SELECT APDU while NDEF service is selected |
| `aid_router` | `k_ndef_svc.on_deselect` | On `nfc_work_q` when a different AID is selected |
| `aid_router` | `k_ndef_svc.on_field_off` | On `nfc_work_q` when NFC field goes away |
| `nfc_store` (Wave 6) | `k_ndef_svc.serialize` | On caller thread via `nfc_stack_save()` (only when stack stopped) |
| `nfc_store` (Wave 6) | `k_ndef_svc.deserialize` | On caller thread via `nfc_stack_load()` (only when stack stopped) |
| Ultralight service (Wave 5c) | `ndef_service_set_content(msg, len)` | `@caller_sync`, before `nfc_stack_start()` or after `nfc_stack_stop()` |
| Shell / test code | `ndef_service_get_config/stats/state` | Any thread |

### 5.3 AID Registration

The NDEF AID is registered in `nfc_stack.c` (wiring rule per CONVENTIONS §3):

```c
/* In nfc_stack.c — both NDEF and ULTRALIGHT profiles use the same registration */
static const uint8_t k_ndef_aid[] = { 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x01U };

/* During profile activation (NFC_PROFILE_NDEF or NFC_PROFILE_ULTRALIGHT): */
rc = aid_router_register(k_ndef_aid, sizeof(k_ndef_aid), ndef_service_get());
```

> **DECISION-7 (Ultralight integration):** Wave 4 DECISION-6 (TBC Wave 5) is hereby **confirmed**: `NFC_PROFILE_ULTRALIGHT` registers the NDEF AID via `ndef_service_get()`. The distinction between NDEF and Ultralight content is handled at the data-model level — Wave 5c's Ultralight service calls `ndef_service_set_content()` at provisioning time to inject the rendered NDEF message. From the router's perspective the two profiles are identical.

### 5.4 RESERVED — `ndef_poller` (Reader Role)

> **Not implemented in this slice.** When a reader-capable backend becomes available
> (`CONFIG_NFC_ROLE_READER` + ST25R3916/RFAL or PN7160), the NDEF protocol module gains
> an `ndef_poller` component that reads an NDEF Type-4 Tag from a physical card into the
> same data model (`s_ndef_file`) that this listener serves. The poller would then
> serialize the result with completeness flag `NFC_STORE_ENTRY_FLAG_READER_CAPTURED` (wave6-store.md §1.7 / spec v3 §4.5).
> **Header/interface placeholder only** — no `ndef_poller` API is declared or compiled
> in this slice. When `CONFIG_NFC_ROLE_READER` is added, the poller source file
> compiles under that gate, orthogonal to the listener.

---

## 6. Implementation Notes

### 6.1 CC File Construction

The CC file is a `uint8_t[15]` (not `const`) built during `ndef_service_init()` so the write-access byte can be set from `s_config.writable`. It is stored in `s_cc_file` (file-static, not `const` at declaration, but effectively immutable after init until re-init):

```c
static void build_cc_file(bool writable)
{
    uint16_t ndef_file_size = (uint16_t)NDEF_FILE_BUF_SIZE;
    uint16_t mle = ndef_file_size;   /* max readable in one shot */
    uint16_t mlc = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE; /* max writable */

    s_cc_file[CC_OFF_CC_LEN_HI]  = 0x00U;
    s_cc_file[CC_OFF_CC_LEN_LO]  = (uint8_t)NDEF_CC_FILE_LEN;   /* 0x0F */
    s_cc_file[CC_OFF_MAP_VER]    = 0x20U;                        /* v2.0 */
    s_cc_file[CC_OFF_MLE_HI]     = (uint8_t)(mle >> 8U);
    s_cc_file[CC_OFF_MLE_LO]     = (uint8_t)(mle & 0xFFU);
    s_cc_file[CC_OFF_MLC_HI]     = (uint8_t)(mlc >> 8U);
    s_cc_file[CC_OFF_MLC_LO]     = (uint8_t)(mlc & 0xFFU);
    s_cc_file[CC_OFF_TLV_T]      = 0x04U;
    s_cc_file[CC_OFF_TLV_L]      = 0x06U;
    s_cc_file[CC_OFF_FILE_ID_HI] = 0xE1U;
    s_cc_file[CC_OFF_FILE_ID_LO] = 0x04U;
    s_cc_file[CC_OFF_FSIZE_HI]   = (uint8_t)(ndef_file_size >> 8U);
    s_cc_file[CC_OFF_FSIZE_LO]   = (uint8_t)(ndef_file_size & 0xFFU);
    s_cc_file[CC_OFF_RD_ACCESS]  = CC_WR_ACCESS_OPEN;
    s_cc_file[CC_OFF_WR_ACCESS]  = writable ? CC_WR_ACCESS_OPEN : CC_WR_ACCESS_NONE;
}
```

`build_cc_file` is a `static` function at file scope (not a `static` local — MISRA Rule 8.7 / creed §9).

### 6.2 Response Buffer Safety

`s_response_buf` is file-static (BSS). After `nfc_transport_send_response(s_response_buf, n)` the HAL borrows the pointer until the next callback (CONVENTIONS §5). Since all callbacks run sequentially on `nfc_work_q` (one at a time), the buffer is safe to reuse in the next callback. The implementer must not call `send_response` twice in one callback invocation — only the final `send_response` call matters; there is no double-send here.

### 6.3 Offset and Bounds Arithmetic

All offset arithmetic is performed in `uint32_t` to prevent silent wrap-around:

```c
uint32_t offset32 = ((uint32_t)apdu->p1 << 8U) | (uint32_t)apdu->p2;
if (offset32 >= file_length) { /* → 6B00 */ }
uint32_t available = file_length - offset32;
uint32_t actual = MIN(ne, available);
```

`uint16_t` offset values from P1:P2 can reach 0xFFFF; `file_length` is at most `NDEF_FILE_BUF_SIZE ≤ 4098`. The widening to `uint32_t` prevents the `available` subtraction from wrapping. `MIN` is the Zephyr kernel `MIN` macro (safe for unsigned types).

### 6.4 APDU Data Pointer Lifetime

`apdu->data` points into the inbound `net_buf`. It is valid only for the duration of the `on_apdu` callback. In `UPDATE BINARY`, `memcpy(&s_ndef_file[offset], apdu->data, apdu->lc)` completes before `on_apdu` returns — no lifetime issue. No data pointer is retained past the callback.

### 6.5 Threading Summary

| Context | Access | Notes |
|---|---|---|
| `nfc_work_q` | `s_file_sel` (R/W), `s_ndef_file` (R/W for UPDATE BINARY, R for READ BINARY), `s_cc_file` (R), `s_response_buf` (W), `s_stats` (W via STATS_INC) | All dispatch callbacks |
| Caller thread | `s_config` (W once in init), `s_state` (W in init/shutdown), `s_ndef_file` (W in set_content, deserialize), `s_cc_file` (W in init) | @caller_sync only; must not run concurrently with nfc_work_q |
| Any thread | `s_stats` (R via STATS_COPY_OUT under lock) | get_stats() |

**No concurrent access between caller thread and `nfc_work_q` is possible when the stack is stopped** (serialize/deserialize/set_content precondition). When the stack is STARTED, caller-thread functions must not be called (documented constraint, same as store layer).

### 6.6 LOG_MODULE_REGISTER

```c
LOG_MODULE_REGISTER(ndef_service, CONFIG_NFC_LOG_LEVEL);
```

`CONFIG_NFC_LOG_LEVEL` is inherited from the parent NFC Kconfig (wave4 §1.6 or to be confirmed). If not defined, fall back to `LOG_LEVEL_INF`.

### 6.7 MISRA C:2012 Notes and DEV-ZEP Citations

| Rule / Deviation | Applies | Notes |
|---|---|---|
| **Rule 8.7** (objects not used outside translation unit shall be file-static) | `build_cc_file`, `ndef_on_select`, etc. | All vtable callback implementations and helpers are `static`. ✓ |
| **Rule 9.1** (automatic variables initialised before use) | All local variables | `offset32`, `actual`, etc. initialised at declaration. ✓ |
| **Rule 10.1 / 10.4** (essential type) | `P1 << 8U` shift | Shift of `uint8_t` via `(uint32_t)` explicit cast — compliant. ✓ |
| **Rule 14.4** (controlling expr boolean) | `if (offset32 >= file_length)` | `uint32_t >= uint32_t` is boolean. ✓ |
| **Rule 17.7** (return value checked) | `nfc_transport_send_response` | Return value checked; on non-zero, `STATS_ERROR` called. ✓ |
| **Rule 21.6 / Dir 4.9** (no dynamic alloc) | None | All buffers are file-static. ✓ |
| **DEV-ZEP-008** (`LOG_INF` / `LOG_ERR`) | `ndef_service.c` logging | Variadic LOG macros — pre-approved deviation. |
| **DEV-ZEP-009** (`shell_print`) | `ndef_service_shell_cmds.c` | Pre-approved deviation. |
| **Rule 16.4** (`switch` must have `default`) | `ndef_file_sel_t` switch | All switches over `ndef_file_sel_t` must include `default`. ✓ |
| **Rule 8.9** (objects defined at block scope if only used in one block) | `k_ndef_svc` | Module-level `const` struct shared across calls — file scope is correct. ✓ |
| **Rule 15.5** (function has single exit) | Dispatch handlers | Handlers use early-return pattern with error SW. **MISRA 15.5 Advisory** — acceptable per creed §4 guard-clause pattern; add suppression `misra-c2012-15.5:src/nfc/services/ndef/`. |

---

## 7. Conventions Compliance

| Checklist item (CONVENTIONS §12) | Status | Justification |
|---|---|---|
| Lifecycle matches §2 (minimal; `shutdown` not `deinit`) | ✓ | `init()`/`shutdown()` only. States: UNINITIALIZED / INITIALIZED. |
| `config_t` / `stats_t` / `state_t` + three getters | ✓ | §1.3–1.5 define all three; §2.3 provides getters. |
| State storage Pattern A per §2 | ✓ | Plain `ndef_service_state_t` enum; only mutated by `@caller_sync` lifecycle. |
| Coupling matches §3; callbacks follow §4 (`_fn`/`_cb`, `user_ctx`, NULL-check) | ✓ | Service is registered via `nfc_service_t` vtable; `user_ctx = NULL` (singleton); invoker NULL-checks per wave3 §3. `on_select` / `on_apdu` / `on_deselect` / `on_field_off` implement the exact typedef signatures from `service.h`. |
| Buffer model per §5 (net_buf inbound / static outbound) | ✓ | Inbound: `apdu->data` is a zero-copy view into the net_buf (no copy needed; data consumed before callback returns). Outbound: `s_response_buf` is file-static, borrowed by HAL until next callback. No net_buf use in outbound path. |
| Stats recipe per §6 (`s_stats` + `s_stats_lock`, copy-out getter, `STATS_RESET` after `-EALREADY` guard) | ✓ | §4.1 declares both with exact names. `STATS_RESET(s_stats)` is first statement after `-EALREADY` guard in `init()`. `get_stats` uses `STATS_COPY_OUT`. Named counter per reject path (§1.4). |
| Threading + annotations per §7 | ✓ | `@nfc_work_q` on all vtable callbacks; `@caller_sync` on lifecycle + `set_content` + serialize/deserialize; `@threadsafe` on getters; `@isr_safe` on `ndef_service_get()`. Thread-safety analysis in §6.5. |
| Error codes per §9; shell per §10 | ✓ | Only standard errno returned (`-EALREADY`, `-EINVAL`, `-ENODEV`, `-ENOSPC`, `-EBADMSG`). ISO 7816 SW never conflated with C return. Shell in dedicated `ndef_service_shell_cmds.c` with `config/stats/state` subcmds. |
| MISRA notes + DEV-ZEP citations per §11 | ✓ | §6.7 table covers all applicable rules and deviations. |

---

## 8. Tasks

All tasks target `qemu_cortex_m33` (or DK hardware) for pure-logic ztest. `native_sim` is Linux-CI-only in this repo (macOS blocked for arch/posix in this repo's tooling). Test path: `tests/unit/nfc_ndef/` (standalone `CMakeLists.txt` + `prj.conf` + `testcase.yaml`, built `--no-sysbuild`; Twister tag `writable_ndef.nfc.ndef`). Integration tests require HAL and nrfxlib; excluded from this wave's test scope.

- [ ] **Task 1 — Scaffolding** (`CMakeLists.txt`, `Kconfig`, empty `ndef_service.h`/`ndef_service.c`)
  - `CMakeLists.txt`:
    ```cmake
    # src/nfc/services/ndef/CMakeLists.txt
    if(CONFIG_NFC_SERVICE_NDEF)
      zephyr_library_named(nfc_ndef_service)
      zephyr_library_sources(
        ndef_service.c
        ndef_service_shell_cmds.c
      )
      zephyr_library_include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../   # src/nfc/ root for router/ and framing/ includes
      )
    endif()
    ```
  - `Kconfig`: empty file with comment:
    ```kconfig
    # services/ndef/Kconfig
    # No service-specific tunables — NFC_SERVICE_NDEF and NFC_NDEF_MAX_SIZE
    # are defined in src/nfc/Kconfig (wave4 §1.6).
    ```
  - Stub `ndef_service.h` with include guard; stub `ndef_service.c` with `LOG_MODULE_REGISTER`.

- [ ] **Task 2 — Types** (TDD: write type-check test first)
  - Define `ndef_service_config_t`, `ndef_service_stats_t`, `ndef_service_state_t` in `ndef_service.h`.
  - Define `NDEF_SERVICE_MAX_SERIALIZED`, `NDEF_FILE_BUF_SIZE`, `NDEF_RESP_BUF_SIZE`.
  - Define `ndef_file_sel_t` (internal, top of `ndef_service.c`).
  - Define all protocol `#define` constants in `ndef_service.c`.
  - ztest: `test_types.c` — `BUILD_ASSERT(NDEF_SERVICE_MAX_SERIALIZED == 2U + CONFIG_NFC_NDEF_MAX_SIZE)`, `BUILD_ASSERT(sizeof(ndef_service_stats_t) > 0U)`, etc.

- [ ] **Task 3 — File-static state + vtable singleton** (`ndef_service.c`)
  - Declare all file-static variables from §4.1.
  - Implement `ndef_service_get()` returning `&k_ndef_svc`.
  - Implement `ndef_service_get_state()`, `ndef_service_get_config()`, `ndef_service_get_stats()`.
  - **Commit point:** types, getters, vtable skeleton compile cleanly.

- [ ] **Task 4 — `ndef_service_init()` / `ndef_service_shutdown()`** (TDD)
  - ztest `test_lifecycle.c`:
    - `test_init_default`: `init(NULL)` → `get_state() == INITIALIZED`; `s_config.writable == true`; NLEN bytes == 0.
    - `test_init_ealready`: `init(NULL)` twice → second returns `-EALREADY`.
    - `test_shutdown_uninit`: `shutdown()` when uninit → `0`.
    - `test_init_after_shutdown`: shutdown then init → OK.
  - Implement `build_cc_file(bool writable)`.
  - Implement `ndef_service_init(cfg)` and `ndef_service_shutdown()` per §3.1–3.2.

- [ ] **Task 5 — `on_select` + SELECT FILE handler** (TDD)
  - ztest `test_select.c` (mock `nfc_transport_send_response`; capture last call args):
    - `test_on_select_sends_9000`: `on_select(aid, 7, NULL)` → last response == `{0x90, 0x00}`, `select_app_count == 1`.
    - `test_on_select_resets_file_sel`: set `s_file_sel = NDEF_FILE_NDEF` (via test backdoor); call `on_select`; file_sel reverts to `NDEF_FILE_NONE`.
    - `test_select_file_cc`: send `{CLA=0x00, INS=0xA4, P1=0x00, P2=0x0C, Lc=2, data={0xE1,0x03}}` → response `9000`, `select_file_cc_count++`.
    - `test_select_file_ndef`: similar for E104.
    - `test_select_file_notfound`: file ID `0xE105` → response `6A82`, `select_file_notfound_count++`.
    - `test_select_file_badlen`: Lc=1 → `6700`.
    - `test_select_file_badp1`: P1=0x04 → `6A86`.
    - `test_select_file_badp2`: P2=0x04 → `6A86`.
  - Implement `ndef_on_select`, `handle_select_file`.

- [ ] **Task 6 — READ BINARY handler** (TDD)
  - ztest `test_read_binary.c`:
    - `test_read_cc_full`: select CC, READ BINARY offset=0 Le=15 → response `{cc_bytes[0..14], 0x90, 0x00}`, `read_ok_count++`.
    - `test_read_cc_partial`: READ BINARY offset=2 Le=3 → response `{cc[2], cc[3], cc[4], 0x90, 0x00}`.
    - `test_read_ndef_empty`: select NDEF, READ offset=0 Le=0 (Ne=256) → response `{0,0, ... 0x90, 0x00}` (256 bytes of zeros + SW or all of NDEF_FILE_BUF_SIZE if < 256).
    - `test_read_nofile`: no file selected → `6986`.
    - `test_read_offset_beyond_eof`: offset >= file_length → `6B00`.
    - `test_read_le_zero_short`: Le=0 (short, has_le=true, extended=false) → Ne=256 capped to available.
    - `test_read_le_zero_ext`: extended=true, Le=0 → Ne=65536 capped to available.
    - `test_read_no_le`: has_le=false → read all available bytes.
  - Implement `handle_read_binary` with Le/Ne computation per §3.6.2.2.

- [ ] **Task 7 — UPDATE BINARY handler** (TDD)
  - ztest `test_update_binary.c`:
    - `test_update_ndef_ok`: write 4 bytes at offset 0 → 9000; verify `s_ndef_file[0..3]` updated.
    - `test_update_nlen_then_read`: write NLEN={0,5} at offset 0, then write 5 msg bytes at offset 2; READ BINARY reads back correctly.
    - `test_update_nofile`: no file selected → `6986`.
    - `test_update_cc`: file selected = CC → `6985`.
    - `test_update_readonly`: `init({.writable=false})`; select NDEF; UPDATE → `6985`.
    - `test_update_lc_zero`: Lc=0 → `6700`.
    - `test_update_overflow`: offset=0, Lc=NDEF_FILE_BUF_SIZE+1 → `6700`.
    - `test_update_at_boundary`: write at offset=NDEF_FILE_BUF_SIZE-1, Lc=1 → `9000` (exact boundary).
  - Implement `handle_update_binary`.

- [ ] **Task 8 — `on_deselect` / `on_field_off`** (TDD)
  - ztest: deselect after CC select → `s_file_sel == NDEF_FILE_NONE`; field_off after NDEF select → `NDEF_FILE_NONE`.
  - Implement `ndef_on_deselect`, `ndef_on_field_off`.

- [ ] **Task 9 — `ndef_service_set_content`** (TDD)
  - ztest `test_set_content.c`:
    - `test_set_content_ok`: 5-byte message → `s_ndef_file[0..1] == {0,5}`; `s_ndef_file[2..6] == msg[0..4]`.
    - `test_set_content_clear`: `set_content(NULL, 0)` → NLEN=0, buffer zeroed.
    - `test_set_content_max`: `len == CONFIG_NFC_NDEF_MAX_SIZE` → `0`.
    - `test_set_content_toobig`: `len == CONFIG_NFC_NDEF_MAX_SIZE + 1` → `-EINVAL`.
    - `test_set_content_null_nonzero`: `set_content(NULL, 1)` → `-EINVAL`.
    - `test_set_content_uninit`: call before `init()` → `-ENODEV`.
  - Implement `ndef_service_set_content`.

- [ ] **Task 10 — Serialize / Deserialize** (TDD)
  - ztest `test_persistence.c`:
    - `test_serialize_empty`: NLEN=0 → `out_len == 2`; `out == {0, 0}`.
    - `test_serialize_10byte_msg`: NLEN=10 → `out_len == 12`; `out[0..1] == {0, 10}`; `out[2..11] == msg`.
    - `test_serialize_enospc`: `max = 1` → `-ENOSPC`.
    - `test_deserialize_roundtrip`: serialize → deserialize → file buffer identical.
    - `test_deserialize_bad_nlen`: `in[0..1] == {0xFF, 0xFF}` (NLEN > CONFIG_NFC_NDEF_MAX_SIZE) → `-EBADMSG`.
    - `test_deserialize_truncated`: `len = NLEN + 2 - 1` → `-EBADMSG`.
    - `test_deserialize_uninit`: before `init()` → `-ENODEV`.
    - `test_serialize_null_out`: `out == NULL` → `-EINVAL`.
  - Implement `ndef_serialize`, `ndef_deserialize` per §3.7.

- [ ] **Task 11 — Shell commands** (`ndef_service_shell_cmds.c`)
  - Implement `cmd_ndef_config`, `cmd_ndef_stats`, `cmd_ndef_state` using `shell_print` (DEV-ZEP-009).
  - `config`: print `writable`.
  - `stats`: print all counters from `ndef_service_get_stats`.
  - `state`: print `ndef_service_get_state()` as string.
  - Register:
    ```c
    SHELL_STATIC_SUBCMD_SET_CREATE(ndef_service_cmds,
        SHELL_CMD(config, NULL, "Print NDEF service config", cmd_ndef_config),
        SHELL_CMD(stats,  NULL, "Print NDEF service stats",  cmd_ndef_stats),
        SHELL_CMD(state,  NULL, "Print NDEF service state",  cmd_ndef_state),
        SHELL_SUBCMD_SET_END);
    SHELL_CMD_REGISTER(ndef_service, &ndef_service_cmds, "NDEF T4T service control", NULL);
    ```

- [ ] **Task 12 — `on_apdu` top-level dispatch** (TDD)
  - ztest `test_on_apdu.c`:
    - `test_unknown_ins`: INS=0x01 → `6D00`; `apdu_unknown_ins_count++`.
    - `test_bad_cla`: CLA=0x80, INS=0xB0 → `6E00`; `apdu_bad_cla_count++`.
  - Implement `ndef_on_apdu` routing to `handle_select_file`, `handle_read_binary`, `handle_update_binary`.
  - Add `nfc_transport_send_response` return-value check with `STATS_ERROR`.

- [ ] **Task 13 — Integration wiring note**
  - Verify `ndef_service_init`, `ndef_service_shutdown`, `ndef_service_get` signatures match wave4 §5.2 exactly (no breaking changes — superset only).
  - Confirm `persist_id = NFC_PERSIST_ID_NDEF = 0x01U` (wave3 §1.1).
  - Confirm `NDEF_SERVICE_MAX_SERIALIZED` is exported in `ndef_service.h` (Wave 6 uses it).

- [ ] **Task 14 — `src/nfc/Kconfig` rsource addition** (Wave 4 update — minimal)
  - Add `rsource "services/ndef/Kconfig"` to the appropriate placeholder in `src/nfc/Kconfig`.
  - Add `add_subdirectory(services/ndef)` guarded by `if(CONFIG_NFC_SERVICE_NDEF)` in `src/nfc/CMakeLists.txt` or the services-level CMakeLists.

---

## DECISION Summary

| ID | Question | Decision |
|---|---|---|
| **DECISION-1** | Shape, name, and concurrency contract of the content-injection API | `int ndef_service_set_content(const uint8_t *msg, size_t len)` — `@caller_sync`; no internal locking; caller must ensure stack is not STARTED; mirrors serialize/deserialize constraint |
| **DECISION-2** | CC write-access byte and CC mutability | CC write-access field reflects `s_config.writable`; CC file is unconditionally read-only (UPDATE BINARY on CC always returns 6985) |
| **DECISION-3** | Serialize scope: live data only vs full buffer | Serialize writes `2 + NLEN_value` bytes (live portion only); `NDEF_SERVICE_MAX_SERIALIZED = 2 + CONFIG_NFC_NDEF_MAX_SIZE` for Wave 6 buffer sizing |
| **DECISION-4** | SELECT FILE P2 acceptance | Accept P2 in `{0x00, 0x0C}`; all other P2 → 6A86; response is always 9000 (no FCI returned) |
| **DECISION-5** | READ BINARY Le=0 semantics | Short Le=0 → Ne=256; extended Le=0 → Ne=65536; `!has_le` → read all available; actual bytes = MIN(Ne, available) |
| **DECISION-7** | Ultralight integration binding | Wave 4 DECISION-6 confirmed: `NFC_PROFILE_ULTRALIGHT` registers NDEF AID via `ndef_service_get()`; Ultralight injects content via `ndef_service_set_content()` |
| **DECISION-8** | Response buffer size | `NDEF_RESP_BUF_SIZE = CONFIG_NFC_NDEF_MAX_SIZE + 4` bytes; covers CC (17 B) and NDEF file + SW |
| **DECISION-9** | `ndef_service_init` signature vs Wave 4 expected | Wave 4 §5.2 already specifies `init(const ndef_service_config_t *cfg)`; `nfc_stack.c` passes NULL → defaults; no compatibility issue |
| **DECISION-10** | Live persist after reader UPDATE BINARY | Stack-wired `nfc_store_on_dirty()` on successful NDEF-file write; cloned NDEF = full writable emulation with persist (spec §1.1; wave6 DECISION-ST-14) |
