# Wave 2: Framing (apdu_assembler) Implementation Plan

**Status:** LOCKED — 2026-06-12 (harmonized v2)

**Layer:** framing/apdu_assembler
**Files produced:**
- `src/nfc/framing/apdu_types.h`
- `src/nfc/framing/apdu_assembler.h`
- `src/nfc/framing/apdu_assembler.c`
- `src/nfc/framing/apdu_assembler_shell_cmds.c`
- `src/nfc/framing/CMakeLists.txt`
- `src/nfc/framing/Kconfig`
- `tests/unit/apdu_assembler/` (ztest suite for the pure `apdu_parse` function; Twister tag `sigmation.nfc.apdu_assembler`)

**Depends on:** `docs/superpowers/plans/wave1-hal.md` (LOCKED — 2026-06-12)

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — binding for all decisions (read in full)
2. `docs/superpowers/plans/wave1-hal.md` — LOCKED Wave 1 ground truth: `nfc_transport_ops_t` shape, `on_apdu` callee-owns ref, `nfc_transport_send_response`, `nfc_work_q`, HAL duplicates 6700 locally
3. `docs/NFC_WAVE_PLANNING_GUIDE.md` — process (8 steps) and template
4. `docs/superpowers/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) — architecture spec; §6.2 (framing), §6.3 (router), §8 (threading); retained as card-mode/NFCT implementation detail
5. `docs/superpowers/specs/2026-06-12-nfc-stack-architecture.md` (v3) — **architecture-of-record**; §4.2 (ISO-DEP/APDU lane scope), §4.3 (protocol modules), §5 (Kconfig)
6. `docs/API_DESIGN_CREED.md` — lifecycle, Pattern A/B, workqueue, memory, shell, error codes
7. `docs/CALLBACK_REGISTRATION_GUIDE.md` — `_fn`/`_cb`, `user_ctx`, guard/NULL-clear pattern
8. `docs/STATS_API_DESIGN.md` — `s_stats`/`s_stats_lock`, copy-out getter, `STATS_*` macros
9. `docs/NETWORK_BUFFERS.md` — one-owner rule, FIXED pool, unref discipline
10. `docs/STACK_SPEC.md` — downward=direct call, upward=A/B/C/D; wiring in `nfc_stack.c`
11. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_RESET`, `STATS_COPY_OUT`
12. `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001 through DEV-ZEP-018
13. ISO 7816-4 C-APDU structure knowledge: Cases 1/2/3/4, short and extended Lc/Le

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (replaced prototype; patterns must not carry forward).

---

> **Architecture Context (spec v3 reframe — additive):**
> This plan is the **NFCT / card-first-slice** implementation of
> [architecture spec v3](../specs/2026-06-12-nfc-stack-architecture.md).
> All existing Type-4 behavior, function/type names, and contracts below are
> **unchanged**.
>
> **Lane scope (spec §4.2):** `apdu_assembler` (Wave 2) + `aid_router` (Wave 3)
> together form the **ISO-DEP / Type-4 dispatch lane** — one lane among several.
> Raw/native lanes (Type 2/3/15693) bypass framing and router entirely; those
> are reserved seams not implemented in this slice. Framing + router are
> **not** the universal spine.

---

## 1. Types and Constants

### 1.1 `apdu_types.h` — Shared ISO 7816 Vocabulary

This header is the single source of truth for ISO 7816-4 protocol vocabulary shared
across the framing, router, and service layers. It does **not** include any Zephyr
kernel headers — it is pure C and portable.

**Status word constants** (`uint16_t` values; use `NFC_SW1`/`NFC_SW2` for byte extraction):

```c
/* ISO 7816-4 response status words */
#define NFC_SW_OK                        0x9000U  /* Normal processing                        */
#define NFC_SW_WRONG_LENGTH              0x6700U  /* Wrong length (structural malform)         */
#define NFC_SW_CLA_NOT_SUPPORTED         0x6E00U  /* CLA not supported                        */
#define NFC_SW_INS_NOT_SUPPORTED         0x6D00U  /* INS not supported                        */
#define NFC_SW_FILE_NOT_FOUND            0x6A82U  /* File or application not found            */
#define NFC_SW_FUNC_NOT_SUPPORTED        0x6A81U  /* Function not supported                   */
#define NFC_SW_SECURITY_STATUS           0x6982U  /* Security status not satisfied            */
#define NFC_SW_CONDITIONS_NOT_SATISFIED  0x6985U  /* Conditions of use not satisfied          */
#define NFC_SW_COMMAND_NOT_ALLOWED       0x6986U  /* Command not allowed (no current EF/DF) — router: non-SELECT with no service selected */
#define NFC_SW_RECORD_NOT_FOUND          0x6A83U  /* Record not found (e.g. EMV READ RECORD out of range) */
#define NFC_SW_WRONG_P1P2                0x6B00U  /* Wrong parameters P1-P2                   */
#define NFC_SW_UNKNOWN                   0x6F00U  /* No precise diagnosis (generic error)     */

/* Extract SW1 (high byte) and SW2 (low byte) for building response buffers */
#define NFC_SW1(sw)  ((uint8_t)(((sw) >> 8U) & 0xFFU))
#define NFC_SW2(sw)  ((uint8_t)((sw) & 0xFFU))
```

**CLA byte constants** (those used by router for dispatch or by multiple services):

```c
#define NFC_CLA_ISO7816   0x00U  /* Standard interindustry class (ISO 7816-4)        */
#define NFC_CLA_PROP      0x80U  /* Proprietary class (Aliro AUTH0/AUTH1)             */
#define NFC_CLA_DESFIRE   0x90U  /* DeSFire native wrapped mode                       */
```

**INS byte constants** (those needed at or above the router boundary):

```c
#define NFC_INS_SELECT        0xA4U  /* SELECT (file or AID) — ISO 7816-4 §7.1.1       */
#define NFC_INS_READ_BINARY   0xB0U  /* READ BINARY — NDEF T4T §7.2                    */
#define NFC_INS_UPDATE_BINARY 0xD6U  /* UPDATE BINARY — NDEF T4T §7.3                  */
#define NFC_INS_GET_RESPONSE  0xC0U  /* GET RESPONSE — ISO 7816-4 §7.6.1               */
```

**P1 constants for SELECT**:

```c
#define NFC_SELECT_P1_BY_AID     0x04U  /* SELECT Application by DF Name (AID) */
#define NFC_SELECT_P1_BY_FILE_ID 0x00U  /* SELECT EF by File ID (NDEF)          */
```

**`nfc_apdu_t` — parsed view into a net_buf**:

```c
/**
 * @brief Parsed C-APDU view into a net_buf data region. Zero-copy.
 *
 * All pointer fields point DIRECTLY into the net_buf data — no copy is made.
 * Valid ONLY during the on_apdu dispatch call chain:
 *
 *   framing on_apdu → aid_router_dispatch() → svc->on_apdu() → returns
 *                                                              ↑
 *                                              net_buf_unref() happens AFTER here
 *
 * Do NOT hold or cache nfc_apdu_t.data past the return of on_apdu.
 * Services must copy any data they need to keep.
 *
 * Le semantics (ISO 7816-4):
 *   - Short Le = 0  → Ne = 256   (has_le == true, extended == false)
 *   - Ext   Le = 0  → Ne = 65536 (has_le == true, extended == true)
 *   - has_le == false → no Le field; Ne is "don't care" (send all data available)
 */
typedef struct {
    uint8_t        cla;       /**< CLA byte                                             */
    uint8_t        ins;       /**< INS byte                                             */
    uint8_t        p1;        /**< P1 byte                                              */
    uint8_t        p2;        /**< P2 byte                                              */
    const uint8_t *data;      /**< Command data field; NULL when lc == 0               */
    uint16_t       lc;        /**< Command data length; 0 for Cases 1 and 2            */
    uint16_t       le;        /**< Expected response length; 0 = maximum (see above)  */
    bool           has_le;    /**< true if a Le field was present in the APDU          */
    bool           extended;  /**< true if Lc or Le used extended (3-byte) encoding   */
} nfc_apdu_t;
```

> **DECISION-1 (nfc_apdu_t.extended field):** The spec v1 §6.2 `nfc_apdu_t` definition omitted an `extended` flag. Added here so the router and services can distinguish extended-format APDUs (needed for building extended R-APDUs where Le=0 means 65536 not 256); spec v2 §6.2 includes it. The field is kept and flows through to services — revisit at Wave 5 review.

> **DECISION-2 (nfc_apdu_t.data == NULL when lc == 0):** The spec is silent on the NULL case. This plan specifies `data == NULL` for Cases 1 and 2 (no data field) so that callers never dereference an uninitialised pointer. Callers must check `lc > 0U` before reading `data`.

### 1.2 `apdu_assembler_config_t`

```c
typedef struct {
    bool extended_apdu_support; /**< Parse extended-format (3-byte Lc/Le) APDUs.
                                  *  Default true when cfg == NULL in init().
                                  *  Set false to reject extended APDUs with 6700
                                  *  for builds that only use NDEF/EMV (short only). */
} apdu_assembler_config_t;

/** Default config applied when cfg == NULL is passed to apdu_assembler_init(). */
#define APDU_ASSEMBLER_CONFIG_DEFAULT \
    ((apdu_assembler_config_t){ .extended_apdu_support = IS_ENABLED(CONFIG_NFC_APDU_EXTENDED_SUPPORT) })
```

Config is frozen after `init()`. No runtime setter is needed.

### 1.3 `apdu_assembler_stats_t`

```c
typedef struct {
    uint32_t apdu_rx_count;           /**< Total APDUs received via on_apdu (all paths)     */
    uint32_t apdu_parse_ok_count;     /**< APDUs parsed successfully, dispatched to router  */
    uint32_t apdu_parse_reject_count; /**< APDUs rejected (structural malform); 6700 sent   */
    uint32_t apdu_extended_count;     /**< Extended-format APDUs parsed successfully         */
    uint32_t field_on_count;          /**< field_on events received from HAL                 */
    uint32_t field_off_count;         /**< field_off events received from HAL                */
    uint32_t error_count;             /**< Mandatory (STATS_ERROR increments this)           */
    int32_t  last_error_code;         /**< Mandatory (last code passed to STATS_ERROR)       */
} apdu_assembler_stats_t;
```

> **DECISION-3 (stats struct renamed and fields revised):** The spec v1 §6.2 defined `nfc_framing_stats_t` with fields `apdu_assembled_count`, `apdu_oversized_count`, `response_sent_count`, `error_count`, `last_error_code`. CONVENTIONS §2 requires `<module>_stats_t` naming → `apdu_assembler_stats_t`. Fields revised: (a) `apdu_assembled_count` → `apdu_rx_count` — framing *receives* assembled APDUs from HAL; "assembled" is a HAL stat; (b) `apdu_oversized_count` removed — oversized APDUs are 6700-rejected and dropped in the HAL ISR *before* reaching framing (CONVENTIONS §5, wave1-hal §4.4); (c) `response_sent_count` removed — framing sends at most parse-reject 6700 responses, already counted by `apdu_parse_reject_count`; (d) added `apdu_parse_ok_count`, `apdu_parse_reject_count`, `apdu_extended_count`, `field_on_count`, `field_off_count` per CONVENTIONS §6 named-counter rule.

### 1.4 `apdu_assembler_state_t`

```c
typedef enum {
    APDU_ASSEMBLER_STATE_UNINITIALIZED = 0, /**< Zero-init default; before init()  */
    APDU_ASSEMBLER_STATE_INITIALIZED,       /**< After init(); ops callbacks active */
} apdu_assembler_state_t;
```

Minimal lifecycle per CONVENTIONS §2: only UNINITIALIZED and INITIALIZED needed.

### 1.5 Internal parse-result type (`.c` only, not exported)

```c
/* Internal — apdu_assembler.c only */
typedef enum {
    APDU_PARSE_OK = 0,
    APDU_PARSE_REJECT_TOO_SHORT,          /* len < 4                              */
    APDU_PARSE_REJECT_LENGTH_MISMATCH,    /* claimed Lc doesn't match actual len  */
    APDU_PARSE_REJECT_EXTENDED_DISABLED,  /* extended format but config disables  */
    APDU_PARSE_REJECT_EXTENDED_LC_ZERO,   /* extended Lc == 0 when len > 7        */
} apdu_parse_result_t;
```

All non-OK results map to a `NFC_SW_WRONG_LENGTH` (6700) response.

### 1.6 Kconfig symbols (`src/nfc/framing/Kconfig`)

| Symbol | Type | Default | Purpose |
|--------|------|---------|---------|
| `NFC_APDU_EXTENDED_SUPPORT` | bool | `y` | Enable extended-format APDU parsing. Disable for NDEF/EMV-only builds needing smaller code. |

```kconfig
config NFC_APDU_EXTENDED_SUPPORT
    bool "Extended APDU support (ISO 7816-4 extended Lc/Le)"
    default y
    depends on NFC_STACK
    help
      Parse ISO 7816-4 extended-format APDUs where Lc and Le are encoded
      as 3 bytes, supporting data fields up to 65535 bytes (in practice
      bounded by CONFIG_NFC_APDU_BUF_SIZE, default 512). Disable to
      reduce code size when only short-format protocols (NDEF, EMV) are
      used. Extended APDUs received when this is disabled are rejected
      with SW 6700 (Wrong Length).
```

---

## 2. Public API

All declarations in `apdu_assembler.h`. Memory comment at top of header:

```c
/*
 * Memory: apdu_assembler_config_t     ~4 B  (file-static, frozen after init)
 *         apdu_assembler_stats_t     ~32 B  (file-static, spinlock-guarded)
 *         s_state                     ~4 B  (plain enum, Pattern A)
 *         s_stats_lock  (struct k_spinlock, 0–4 B platform-dependent)
 *         s_sw_reject_buf[2]           2 B  (file-static const)
 *         s_ops_impl (nfc_transport_ops_t) ~12 B (file-static const, link-time)
 * No threads, no pools (HAL owns nfc_apdu_pool and nfc_work_q).
 * lifecycle: init / shutdown only
 */
```

```c
/**
 * @brief Initialize the framing / APDU-assembler layer.
 *
 * Resets all stats, populates config, and transitions to INITIALIZED.
 * Does NOT start any thread, allocate any buffer, or register any callback.
 * Callback registration is performed by nfc_stack.c via apdu_assembler_get_ops().
 *
 * @param cfg  Config to apply; NULL → APDU_ASSEMBLER_CONFIG_DEFAULT.
 * @retval  0         Success: UNINITIALIZED → INITIALIZED.
 * @retval -EALREADY  Already INITIALIZED.
 * @caller_sync
 */
int apdu_assembler_init(const apdu_assembler_config_t *cfg);

/**
 * @brief Shut down the framing layer.
 *
 * Transitions to UNINITIALIZED. Idempotent. Always returns 0.
 * Must only be called after nfc_transport_stop() (ensuring no in-flight on_apdu
 * callbacks can fire against torn-down state).
 *
 * @retval 0  Always. Shutdown must never fail (creed §1).
 * @caller_sync
 */
int apdu_assembler_shutdown(void);

/**
 * @brief Return the nfc_transport_ops_t for HAL callback registration.
 *
 * Returns a pointer to the file-static const ops struct populated with
 * apdu_assembler's on_field_on, on_field_off, and on_apdu handler functions.
 * nfc_stack.c chains its own wrapper ops handlers to these at runtime
 * (wave4-stack.md §6 DECISION-1); the handlers are invoked with user_ctx == NULL.
 *
 * Safe to call before init() — the ops struct is a link-time constant.
 *
 * @retval Non-NULL always (static const backing).
 * @isr_safe
 */
const nfc_transport_ops_t *apdu_assembler_get_ops(void);

/**
 * @brief Return the frozen config.
 * @retval Non-NULL always (file-static backing; safe before init()).
 * @isr_safe
 */
const apdu_assembler_config_t *apdu_assembler_get_config(void);

/**
 * @brief Copy current stats under spinlock into caller-supplied buffer.
 * @param out  Destination; must not be NULL.
 * @retval  0       Success.
 * @retval -EINVAL  out is NULL.
 * @threadsafe
 */
int apdu_assembler_get_stats(apdu_assembler_stats_t *out);

/**
 * @brief Return current lifecycle state.
 *
 * Pattern A plain enum — not atomic_t. Read from caller thread only.
 * Do not call from ISR or nfc_work_q.
 *
 * @retval apdu_assembler_state_t — never fails.
 * @caller_sync
 */
apdu_assembler_state_t apdu_assembler_get_state(void);
```

> **DECISION-4 (ops struct getter form):** Framing exports a single `apdu_assembler_get_ops()` returning `const nfc_transport_ops_t *`. This encapsulates all three callbacks as a unit, preventing nfc_stack from accidentally wiring them partially or in inconsistent combinations. The alternative — exporting `apdu_assembler_on_field_on`, `apdu_assembler_on_field_off`, `apdu_assembler_on_apdu` as individually named public functions that nfc_stack assembles into an ops struct — was rejected because it exposes internal handler symbols unnecessarily and allows partial registration.

> **DECISION-5 (user_ctx = NULL):** The framing layer is a singleton; all state is file-static. nfc_stack's wrapper handlers chain to `apdu_assembler_get_ops()->on_*` passing `NULL` as `user_ctx` (wave4-stack.md §4.6). The three handler implementations receive `user_ctx == NULL` and ignore it via `ARG_UNUSED(user_ctx)`. This is the correct Pattern A singleton usage.

> **DECISION-6 (get_state is @caller_sync, not @isr_safe):** CONVENTIONS §2 fixes framing as Pattern A (plain enum, not `atomic_t`). No ISR or `nfc_work_q` reader of `s_state` exists in the steady state — lifecycle is caller-only and the defensive check in `on_apdu` is a belt-and-suspenders guard (see §4.4). If the defensive check ever needs `@isr_safe` guarantees, escalate to Pattern B (`atomic_t`).

---

## 3. Contracts

### `apdu_assembler_init(cfg)`

- **Pre:** `s_state == APDU_ASSEMBLER_STATE_UNINITIALIZED`. `@caller_sync`.
- **Post (0):**
  - Check for `-EALREADY` **before** `STATS_RESET` (do not reset stats on double-init).
  - `STATS_RESET(s_stats)` — **first statement after the EALREADY guard**.
  - `s_config` populated from `cfg` (or `APDU_ASSEMBLER_CONFIG_DEFAULT` if `cfg == NULL`).
  - `s_state = APDU_ASSEMBLER_STATE_INITIALIZED`.
  - No threads started, no buffers allocated, no callbacks invoked.
  - The ops struct (`s_ops_impl`) is a compile-time constant; `init()` does not modify it.
- **Errors:**
  - `-EALREADY` — `s_state` is already INITIALIZED.

### `apdu_assembler_shutdown()`

- **Pre:** any state.
- **Post (0):**
  - `s_state = APDU_ASSEMBLER_STATE_UNINITIALIZED`.
  - Idempotent: UNINITIALIZED input → 0 and no-op.
  - No `net_buf` refs are held by framing at shutdown time (each `on_apdu` call completes synchronously on `nfc_work_q`, releasing its ref before returning; `nfc_transport_stop()` guarantees the WQ is drained before shutdown is reached).
- **Errors:** None. Always returns 0.

### `apdu_assembler_get_ops()`

- **Pre:** none (may be called before `init()`).
- **Post:** returns `&s_ops_impl` (compile-time constant). Never NULL.

### Getters

- `get_config` — `&s_config`; never NULL; valid before `init()`. `@isr_safe`.
- `get_stats` — `STATS_COPY_OUT(&s_stats_lock, s_stats, out)`; `-EINVAL` if `out == NULL`; 0 on success. `@threadsafe`.
- `get_state` — plain `s_state` read; never fails. `@caller_sync`.

### HAL ops handler: `on_apdu(buf, user_ctx)` (nfc_work_q)

Invoked by the HAL's `apdu_handler` work item on `nfc_work_q`. `buf` holds exactly one complete C-APDU assembled by the HAL ISR. Framing **owns** the ref on entry.

**Pre:**
- `buf != NULL` (HAL null-checks `s_ops->on_apdu` before calling).
- All bytes in `buf->data[0..buf->len-1]` are valid (HAL immediate-copied from nrfxlib).
- `buf->len ≤ CONFIG_NFC_APDU_BUF_SIZE` (HAL drops oversize with 6700 before enqueue).
- Executing on `nfc_work_q` only.

**Sequence:**

1. `STATS_INC(&s_stats_lock, s_stats, apdu_rx_count)`.
2. **Defensive state check:** if `s_state != APDU_ASSEMBLER_STATE_INITIALIZED` → `net_buf_unref(buf); return;` (belt-and-suspenders; should not occur under correct nfc_stack usage).
3. **Parse:** `nfc_apdu_t apdu = { 0 };` (stack-local, initialized at declaration per MISRA Rule 9.1). Call `apdu_parse(buf->data, (size_t)buf->len, s_config.extended_apdu_support, &apdu)` → `result`.
4. **On parse failure** (`result != APDU_PARSE_OK`):
   - `(void)nfc_transport_send_response(s_sw_reject_buf, 2U);` — send 6700. `(void)` cast is mandatory (Rule 17.7; no recovery possible on send failure at this layer).
   - `STATS_INC(&s_stats_lock, s_stats, apdu_parse_reject_count)`.
   - `net_buf_unref(buf)`.
   - `return;`
5. **On parse success** (`result == APDU_PARSE_OK`):
   - `STATS_INC(&s_stats_lock, s_stats, apdu_parse_ok_count)`.
   - If `apdu.extended == true`: `STATS_INC(&s_stats_lock, s_stats, apdu_extended_count)`.
   - `aid_router_dispatch(&apdu);` — the locked contract: parsed form, direct call (see §5.2 / DECISION-7). At Wave-2 implementation time this line is the `/* TODO(Wave 3): aid_router_dispatch(&apdu); */` marker of task 10 so Wave 2 builds standalone; Wave 3's integration task replaces it with the call.
   - `net_buf_unref(buf);`
   - `return;`

**Net_buf ownership discipline (invariants):**
- Framing owns `buf` for the entire duration of `on_apdu`.
- `net_buf_unref(buf)` is called **exactly once** on **every** code path (step 4 and step 5).
- `buf` is not touched after `net_buf_unref`.
- `apdu.data` (pointing into `buf->data`) is valid during `aid_router_dispatch` and invalid after `net_buf_unref`. Framing does not pass `apdu.data` to a persistent structure.

**Post:** `buf` has been unreffed; pool slot is available for reuse.

### HAL ops handler: `on_field_on(user_ctx)` (nfc_work_q)

- `ARG_UNUSED(user_ctx)`.
- `STATS_INC(&s_stats_lock, s_stats, field_on_count)`.
- No other action. Field-on session prep is the responsibility of the router and services.

### HAL ops handler: `on_field_off(user_ctx)` (nfc_work_q)

- `ARG_UNUSED(user_ctx)`.
- `STATS_INC(&s_stats_lock, s_stats, field_off_count)`.
- `aid_router_field_off();` — the locked contract: direct call (see §5.2). At Wave-2 implementation time this line is the `/* TODO(Wave 3): aid_router_field_off(); */` marker of task 9 so Wave 2 builds standalone; Wave 3's integration task replaces it with the call.

### ISO 7816-4 Parse Case Matrix (full; all cases and all reject classes)

`apdu_parse(data, len, extended_ok, out)` is a pure static function in `apdu_assembler.c`. It has no side effects, no Zephyr kernel calls, and no module state reads — making it fully testable without hardware (`qemu_cortex_m33` or `native_sim`). Note: `native_sim` is Linux-CI-only in this repo; use `qemu_cortex_m33` locally.

**Symbols used in table:** `lc_s = data[4]` (short Lc, uint8_t); `lc_e = (data[5]<<8U)|data[6]` (extended Lc, uint16_t); `le_s = data[4]` (short Le); `le_e = (data[5]<<8U)|data[6]` (extended Le, uint16_t, 0 → 65536); `data_e = &data[7]` (extended data start).

| Condition | Case | `out` fields | Result |
|-----------|------|-------------|--------|
| `len < 4` | — | — | `REJECT_TOO_SHORT` → 6700 |
| `len == 4` | **1** | `lc=0, data=NULL, le=0, has_le=false, extended=false` | OK |
| `len == 5` (any `data[4]`) | **2S** | `lc=0, data=NULL, le=le_s (0→256), has_le=true, extended=false` | OK |
| `len == 6, data[4] == 0` | Malformed | — | `REJECT_LENGTH_MISMATCH` → 6700 (extended incomplete: needs ≥7) |
| `len == 5 + lc_s, lc_s != 0` | **3S** | `lc=lc_s, data=&data[5], le=0, has_le=false, extended=false` | OK |
| `len == 6 + lc_s, lc_s != 0` | **4S** | `lc=lc_s, data=&data[5], le=data[5+lc_s] (0→256), has_le=true, extended=false` | OK |
| `len > 5, data[4] != 0, len ≠ 5+lc_s and len ≠ 6+lc_s` | Malformed | — | `REJECT_LENGTH_MISMATCH` → 6700 |
| `len == 7, data[4] == 0` | **2E** | `lc=0, data=NULL, le=le_e (0→65536), has_le=true, extended=true` | OK if `extended_ok`; else `REJECT_EXTENDED_DISABLED` → 6700 |
| `len == 7, data[4] == 0, !extended_ok` | — | — | `REJECT_EXTENDED_DISABLED` → 6700 |
| `len > 7, data[4] == 0, lc_e == 0` | Malformed | — | `REJECT_EXTENDED_LC_ZERO` → 6700 (Lc=0 in Lc-present context is anomalous; Case 2E already handled by `len==7`) |
| `len > 7, data[4] == 0, lc_e != 0, len == 7 + (size_t)lc_e` | **3E** | `lc=lc_e, data=data_e, le=0, has_le=false, extended=true` | OK if `extended_ok`; else `REJECT_EXTENDED_DISABLED` → 6700 |
| `len > 9, data[4] == 0, lc_e != 0, len == 7 + (size_t)lc_e + 2` | **4E** | `lc=lc_e, data=data_e, le=(data[7+lc_e]<<8U)\|data[7+lc_e+1] (0→65536), has_le=true, extended=true` | OK if `extended_ok`; else `REJECT_EXTENDED_DISABLED` → 6700 |
| `len > 7, data[4] == 0, lc_e != 0, len ≠ 7+lc_e and len ≠ 7+lc_e+2` | Malformed | — | `REJECT_LENGTH_MISMATCH` → 6700 |

**What framing does NOT validate** (router/service responsibility):
- CLA byte value (is it a known class?).
- INS byte value.
- P1/P2 semantics.
- Data content correctness.

**Reject response:** All non-OK parse results → send `NFC_SW_WRONG_LENGTH` (0x6700) via `nfc_transport_send_response(s_sw_reject_buf, 2U)`. A single status word covers all structural malforms; no per-class differentiation.

> **DECISION-9 (Extended Lc=0 with len > 7 is malformed):** When `data[4]==0` and `len > 7`, the first two bytes after the leading 0x00 (`data[5..6]`) encode the extended Lc. If those bytes are both 0x00 (Lc=0) but `len > 7`, this cannot be Case 2E (already handled by `len==7`) and represents trailing data with no announced payload — anomalous. Rejected with 6700. This is the safest conservative choice; the spec does not address this edge case.

---

## 4. Internal State

### 4.1 File-Static State (`apdu_assembler.c`)

```c
/* Lifecycle — Pattern A (plain enum; WQ thread only for data path;
 * lifecycle written by @caller_sync; read defensively on WQ — see §4.4) */
static apdu_assembler_state_t s_state = APDU_ASSEMBLER_STATE_UNINITIALIZED;

/* Required module statics */
static apdu_assembler_config_t s_config = APDU_ASSEMBLER_CONFIG_DEFAULT;
static apdu_assembler_stats_t  s_stats;
static struct k_spinlock        s_stats_lock;  /* NOT K_SPINLOCK_DEFINE (NCS v3.2.4) */

/* Reject response buffer — file-static const.
 * Borrow-until-next-callback lifetime trivially satisfied (static const). */
static const uint8_t s_sw_reject_buf[2] = {
    NFC_SW1(NFC_SW_WRONG_LENGTH),
    NFC_SW2(NFC_SW_WRONG_LENGTH)
};

/* Ops struct — compile-time constant; function pointers initialised at link time.
 * Forward declarations of the three handlers required above this initialiser. */
static void apdu_assembler_on_field_on_handler(void *user_ctx);
static void apdu_assembler_on_field_off_handler(void *user_ctx);
static void apdu_assembler_on_apdu_handler(struct net_buf *apdu, void *user_ctx);

static const nfc_transport_ops_t s_ops_impl = {
    .on_field_on  = apdu_assembler_on_field_on_handler,
    .on_field_off = apdu_assembler_on_field_off_handler,
    .on_apdu      = apdu_assembler_on_apdu_handler,
};
```

### 4.2 Lifecycle State Machine (Pattern A)

| From | Event | To | Action |
|------|-------|----|----|
| UNINITIALIZED | `init(cfg)` | INITIALIZED | check not EALREADY; `STATS_RESET(s_stats)`; populate `s_config`; `s_state = INITIALIZED` |
| INITIALIZED | `init(cfg)` | INITIALIZED | return `-EALREADY` (STATS_RESET NOT called) |
| any | `shutdown()` | UNINITIALIZED | `s_state = UNINITIALIZED`; return 0 |

No STARTED / STOPPED / ERROR states (minimal lifecycle). Transitions are plain assignments inside `@caller_sync` lifecycle functions.

### 4.3 Per-APDU State: None

There is no persistent session state in the framing layer. Per-APDU parse state (`nfc_apdu_t`, `apdu_parse_result_t`) is entirely stack-local inside `apdu_assembler_on_apdu_handler`. This is correct: spec v2 §6.2 confirms framing's job is purely structural validation and dispatch — no session memory. The HAL has already assembled fragments; framing never accumulates partial APDUs.

### 4.4 Concurrency Table

| Variable | Written by | Read by | Synchronization |
|----------|-----------|---------|----------------|
| `s_state` | `init`/`shutdown` (caller thread, `@caller_sync`) | `on_apdu` handler (nfc_work_q, defensive check only) | None required (Pattern A; lifecycle and WQ serialised by nfc_stack usage). Defensive read is non-critical. |
| `s_stats.*` | ops handlers (nfc_work_q), lifecycle (caller) | `get_stats` (any thread) | `s_stats_lock` (via `STATS_*` macros) |
| `s_config` | `init()` (caller, once; frozen thereafter) | ops handlers (nfc_work_q), `get_config` (any) | None needed after init (frozen, read-only) |
| `s_ops_impl` | compile-time constant | `get_ops` (any), nfc_stack | None (read-only) |
| `s_sw_reject_buf` | compile-time constant | `on_apdu` handler (nfc_work_q) | None (read-only) |

**Pattern A note:** `on_apdu` reads `s_state` without a lock as a defensive guard. Under correct nfc_stack usage, `init()` precedes `nfc_transport_start()` and `apdu_assembler_shutdown()` follows `nfc_transport_stop()` (which drains the WQ). The race window does not exist at runtime. If future usage changes this invariant, escalate to Pattern B (`atomic_t`).

---

## 5. Integration Points

### 5.1 Down — HAL Functions Consumed (Signatures Verbatim from wave1-hal.md)

```c
/* ─── Registration (called by nfc_stack.c) ─────────────────────────────────── */
/* @caller_sync — register AFTER apdu_assembler_init() and nfc_transport_init(),
 * BEFORE nfc_transport_start(). CONVENTIONS §3 wiring rule. */
int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops, void *user_ctx);
    /* nfc_stack registers its OWN wrapper ops (&s_transport_ops, NULL) with the
     * HAL and chains each handler to apdu_assembler_get_ops()->on_* at runtime
     * (wave4-stack.md §6 DECISION-1). get_ops() is the public surface nfc_stack
     * consumes; framing never registers itself. */

/* ─── Parse-reject response (called by on_apdu handler on failure) ────────── */
/* @threadsafe — borrows buf until next t4t callback; s_sw_reject_buf is static
 * const so its borrow lifetime is the module lifetime. Return value (void)-cast. */
int nfc_transport_send_response(const uint8_t *buf, size_t len);
    /* framing calls:
     *   (void)nfc_transport_send_response(s_sw_reject_buf, 2U); */

/* ─── Buffer release (called once per APDU, after dispatch returns) ─────── */
/* net_buf_unref is void; no return-value check needed (Rule 17.7 n/a). */
void net_buf_unref(struct net_buf *buf);
    /* framing calls after aid_router_dispatch() returns (success) or
     * after nfc_transport_send_response() (failure). */
```

No other HAL functions are called directly by the framing layer.

### 5.2 Up — Router and nfc_stack (Locked Signatures)

```c
/* ─── Normal APDU dispatch ──────────────────────────────────────────────────── */
/* PARSED form (wave3-router.md DECISION-7): framing forwards its validated
 * nfc_apdu_t parse view directly. The router does NOT re-parse — it reads
 * apdu->cla/ins/p1/lc/data for SELECT detection and passes the same view
 * unchanged to the service vtable. */
void aid_router_dispatch(const nfc_apdu_t *apdu);
    /* framing calls: aid_router_dispatch(&apdu); — apdu is the stack-local
     * nfc_apdu_t produced by apdu_parse(). */
    /* IMPORTANT: apdu.data points into buf->data, valid during this call;
     * net_buf_unref happens AFTER. */

/* ─── Field-off notification ────────────────────────────────────────────────── */
void aid_router_field_off(void);
    /* framing calls: aid_router_field_off(); from on_field_off handler. */
```

> **DECISION-7 (router dispatch API signature — parsed form):** Spec v1 §6.2 said "Forward the **parsed** APDU to `aid_router_dispatch()`" while spec v1 §6.3 defined a raw-bytes form `(const uint8_t *, size_t)` that would force the router to re-parse CLA/INS/P1/P2 — duplicate cycles and duplicate logic for no benefit. The locked signature is the parsed form, `void aid_router_dispatch(const nfc_apdu_t *apdu)` (wave3-router.md DECISION-7; spec v2 §6.2/§6.3 state it everywhere): framing's validated parse view is passed through unchanged, matching the exact type the service vtable requires. Framing's call site is `aid_router_dispatch(&apdu)`.

> **DECISION-8 (framing calls nfc_transport_send_response directly):** CONVENTIONS §3 coupling map shows `service → HAL (response)` for the response path. The framing layer is below services in the stack, but for parse-reject cases, framing IS the terminal handler — the APDU cannot be routed. Calling `nfc_transport_send_response` directly from framing is architecturally correct for structural error responses and matches spec v2 §6.2 intent ("respond 6700 via nfc_transport_send_response and drop"). The `s_sw_reject_buf` is file-static const; its borrow-until-next-callback lifetime is trivially satisfied. Flagged for human review.

### 5.3 nfc_stack (Wave 4) Lifecycle and Wiring Expectations

```c
/* Initialization order in nfc_stack.c: */
(void)apdu_assembler_init(NULL);
(void)nfc_transport_init(NULL);
/* Register nfc_stack's wrapper ops as HAL consumer — AFTER both inits, BEFORE
 * transport_start(). Each wrapper handler chains to the corresponding
 * apdu_assembler_get_ops()->on_* at runtime (wave4-stack.md §6 DECISION-1): */
(void)nfc_transport_register_callbacks(&s_transport_ops, NULL);
(void)nfc_transport_start(&initial_uid);

/* Teardown order in nfc_stack.c: */
(void)nfc_transport_stop();     /* drains nfc_work_q; no more on_apdu calls */
(void)apdu_assembler_shutdown();
(void)nfc_transport_shutdown();
```

The registration call lives in `nfc_stack.c` per CONVENTIONS §3 (wiring rule: "all cross-layer callback registration happens in nfc_stack.c"). Framing's deliverable is `apdu_assembler_get_ops()` — nfc_stack's wrapper consumes it; framing itself registers nothing.

### 5.4 Shell (`apdu_assembler_shell_cmds.c`, CONVENTIONS §10)

```c
SHELL_STATIC_SUBCMD_SET_CREATE(apdu_assembler_cmds,
    SHELL_CMD(config, NULL, "Print config", cmd_apdu_assembler_config),
    SHELL_CMD(stats,  NULL, "Print stats",  cmd_apdu_assembler_stats),
    SHELL_CMD(state,  NULL, "Print state",  cmd_apdu_assembler_state),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(apdu_assembler, &apdu_assembler_cmds,
                   "NFC framing / APDU assembler control", NULL);
```

`config` — `apdu_assembler_get_config()` → `shell_print` `extended_apdu_support`.
`stats` — `apdu_assembler_get_stats(&snap)` → `shell_print` all 8 fields including `error_count`/`last_error_code`.
`state` — `apdu_assembler_get_state()` → `switch` with explicit `default` → `shell_print` string.

---

## 6. Implementation Notes

### Zero-Copy Parse View Validity Window

The lifetime of pointers derived from `buf->data` (including `nfc_apdu_t.data`) is bounded by the `net_buf_unref` call in `on_apdu`. The synchronous call chain guarantees safety:

```
on_apdu(buf) called — framing owns buf (ref count = 1)
  │
  ├─ apdu_parse(buf->data, buf->len, ...)  [nfc_apdu_t.data → &buf->data[offset]]
  │
  ├─ aid_router_dispatch(&apdu)                      ← buf->data still valid
  │     └─ router forwards the same nfc_apdu_t view (no re-parse; Wave 3 DECISION-7)
  │           └─ svc->on_apdu(&apdu, ctx)             ← buf->data still valid
  │                 └─ nfc_transport_send_response(...)
  │                 returns
  │           returns
  │     returns
  │
  └─ net_buf_unref(buf)   ← ONLY HERE: ref → 0; pool slot reclaimed
                            ALL pointers into buf->data are INVALID after this line
```

**Consequence for services:** Any service that caches `apdu->data` past the return of its `on_apdu` callback has a dangling pointer bug. Services must copy data they need to retain (into their own static response buffer or a work item's fields). This constraint must be documented in `router/service.h` (Wave 3).

### net_buf Ownership: Exactly One Unref on Every Path

Two code paths in `on_apdu_handler`:

| Path | Unref point |
|------|------------|
| Parse failure | After `nfc_transport_send_response()`, before `return` |
| Parse success | After `aid_router_dispatch()` returns, before `return` |

No early return may be added without a preceding `net_buf_unref`. The `NETWORK_BUFFERS.md` checklist item "does every early-return path call unref before the ownership-transfer call?" applies here.

### `nfc_transport_send_response` Return Value

`nfc_transport_send_response` returns `int`. On parse-reject, framing has no recovery action if the send fails (the APDU has already been deemed unprocessable). The return value is explicitly `(void)`-cast to satisfy MISRA Rule 17.7:

```c
(void)nfc_transport_send_response(s_sw_reject_buf, 2U);
```

This is not a `STATS_ERROR` path for framing — the parse failure is already counted. An underlying HAL send error would be counted by the HAL's own `STATS_ERROR`.

### Extended APDU Support Decision

`CONFIG_NFC_APDU_BUF_SIZE = 512` (default) supports:
- Short Case 3S/4S: up to 255-byte data field. Sufficient for all short-protocol needs (NDEF, EMV, short DeSFire commands).
- Extended Case 2E: 7-byte total. Always within pool.
- Extended Case 3E: up to 505-byte data field (512 − 7 byte header). Covers DeSFire/Aliro extended payloads.
- Extended Case 4E: up to 503-byte data field (512 − 7 − 2 byte Le trailer).

If `CONFIG_NFC_APDU_EXTENDED_SUPPORT=n`, all extended-format APDUs → 6700 at the framing layer. Appropriate for NDEF/EMV-only builds.

### Integer Arithmetic Safety (MISRA Rule 10.3 / 10.7)

`lc_e` is declared `uint16_t` (max 65535). When computing `7U + (size_t)lc_e` in the extended case comparison:
- On 32-bit ARM: `SIZE_MAX = 0xFFFFFFFF`; `7 + 65535 = 65542` → no overflow.
- Use `(size_t)` cast explicitly: `7U + (size_t)lc_e`.
- Compare against `len` which is `size_t` (same type after cast).

Similarly `5U + (size_t)lc_s` and `6U + (size_t)lc_s` where `lc_s` is `uint8_t` (max 255) → max 261, trivially in range.

### `s_sw_reject_buf` Separate from HAL's `s_sw_wrong_length`

The HAL's `s_sw_wrong_length[2]` (wave1-hal §1.6) is a file-static const in `nfc_transport_nrfx.c`. It was deliberately NOT put in `apdu_types.h` to preserve layering (HAL must not include framing headers). The framing layer's `s_sw_reject_buf[2]` is a separate file-static const in `apdu_assembler.c`. The 2-byte duplication of `{0x67U, 0x00U}` is intentional and layering-correct — both HAL and framing own their own copy.

### MISRA C:2012 Notes + DEV-ZEP Citations

1. **`LOG_*` macros** (`LOG_MODULE_REGISTER`, `LOG_ERR`, `LOG_DBG`) in `apdu_assembler.c` → **DEV-ZEP-008** (Rule 20.1 / Rule 19.10; variadic logging macro).
2. **`shell_print` variadic macro** in `apdu_assembler_shell_cmds.c` → **DEV-ZEP-009** (Rule 20.1 / Rule 19.10).
3. **`ARG_UNUSED(user_ctx)`** in all three ops handler functions (signature fixed by `nfc_transport_ops_t`) → **DEV-ZEP-016** (Rule 2.7; unused parameter in framework callback).
4. **`IS_ENABLED(CONFIG_NFC_APDU_EXTENDED_SUPPORT)`** in `APDU_ASSEMBLER_CONFIG_DEFAULT`: Zephyr utility macro that safely evaluates a Kconfig symbol to 0 or 1. If flagged by analyser under Rule 20.1 → cover under **DEV-ZEP-008** class.
5. **No `CONTAINER_OF`**: framing does not recover parent structs from work items (HAL owns the WQ and work structs); DEV-ZEP-001 is not needed.
6. **No `k_fifo_put/get`**: framing does not touch the APDU fifo (HAL-owned); DEV-ZEP-005 not needed.
7. **`K_SPINLOCK` scoped macro** is NOT used; `STATS_*` macros use explicit `k_spin_lock/unlock` pairs. DEV-ZEP-014 not needed.
8. **MISRA Rule 7.2** (`U` suffix): all unsigned literals have `U` suffix — `2U`, `0x67U`, `0x00U`, `0xFFU`, `4U`, `5U`, `6U`, `7U`.
9. **MISRA Rule 9.1** (initialized at declaration): `nfc_apdu_t apdu = { 0 };` in the handler; `apdu_parse_result_t result = APDU_PARSE_REJECT_TOO_SHORT;` as default before conditional assignment (or set inside `apdu_parse` and returned).
10. **MISRA Rule 14.4** (Boolean controlling expressions): use `(result != APDU_PARSE_OK)` not `(!result)`; `(lc_e == 0U)` not `(!lc_e)`.
11. **MISRA Rule 16.4** (`switch` default): the `switch` on `apdu_assembler_state_t` in the shell `state` command and any switch on `apdu_parse_result_t` must include `default` arms.
12. **MISRA Rule 17.7** (check return values): `nfc_transport_send_response` return → explicit `(void)` cast as shown above. `aid_router_dispatch` return: if Wave 3 declares it `void`, no check needed; if `int`, must check or `(void)`-cast.
13. **MISRA Dir 4.12** (no dynamic allocation): framing allocates nothing at runtime. `nfc_apdu_t` is stack-local. The net_buf pool is defined and owned by the HAL. Fully compliant.
14. **MISRA Rule 10.3/10.7** (integer types in expressions): explicit `(size_t)` casts for all Lc arithmetic as detailed above.

---

## 7. Conventions Compliance (CONVENTIONS §12)

- [x] **Lifecycle matches §2** — Minimal (init/shutdown only); `shutdown` not `deinit`; `apdu_assembler_state_t` has `UNINITIALIZED` and `INITIALIZED` only (no STARTED/STOPPED/ERROR per minimal lifecycle).
- [x] **`config_t`/`stats_t`/`state_t` + three getters defined (§2)** — `apdu_assembler_config_t`, `apdu_assembler_stats_t`, `apdu_assembler_state_t` all defined (§1.2–1.4). `get_config` never NULL; `get_stats` copy-out via `STATS_COPY_OUT`; `get_state` never fails.
- [x] **State storage Pattern A per §2** — `static apdu_assembler_state_t s_state`; WQ thread reads defensively only; lifecycle writes on caller thread; no ISR state reads; no `atomic_t` needed.
- [x] **Coupling matches §3; callbacks follow §4** — Framing provides ops struct to HAL (Pattern A); wiring done in `nfc_stack.c` not inside framing (CONVENTIONS §3 wiring rule); `user_ctx` named correctly (NULL for singleton); ops struct NULL-checked by HAL before dispatch (wave1-hal §4.5); dispatch thread (`nfc_work_q`) documented in header; `_fn` typedef rule: no new `_fn` typedefs introduced (ops struct function pointers are inline per creed §7 Pattern A).
- [x] **Buffer model §5** — Framing holds inbound net_buf ref (transferred from HAL `apdu_handler`), unrefs exactly once after `aid_router_dispatch` returns (or after reject-send on failure path); zero-copy `nfc_apdu_t.data` valid only during dispatch chain; no outbound net_buf (reject response uses file-static const `s_sw_reject_buf`; outbound response is service-owned static buffer per §5).
- [x] **Stats recipe §6** — `static apdu_assembler_stats_t s_stats`; `static struct k_spinlock s_stats_lock` (NOT `K_SPINLOCK_DEFINE`, NCS v3.2.4); `STATS_RESET(s_stats)` as first statement after `-EALREADY` guard in `init()`; `STATS_INC` for every named counter; `STATS_COPY_OUT` in getter; `error_count` + `last_error_code` present.
- [x] **Threading + annotations §7** — `@caller_sync` for `init`/`shutdown`; `@threadsafe` for `get_stats`; `@isr_safe` for `get_config`, `get_ops`; `@caller_sync` for `get_state` (Pattern A); ops handlers run exclusively on `nfc_work_q` (documented in header comment and Doxygen); no sleep, no alloc, no blocking in handlers.
- [x] **Error codes §9; shell §10** — Standard errno only (`-EALREADY`, `-EINVAL`); ISO 7816 `NFC_SW_WRONG_LENGTH` is a protocol response sent via `nfc_transport_send_response`, never a C return code; `apdu_assembler` shell with `config`/`stats`/`state` in dedicated `apdu_assembler_shell_cmds.c`.
- [x] **MISRA notes + DEV-ZEP citations §11** — DEV-ZEP-008/009/016 cited; DEV-ZEP-001/005/014 noted as not needed; all relevant MISRA rules (7.2, 9.1, 10.3, 10.7, 14.4, 16.4, 17.7, Dir 4.12) addressed in §6.

---

## 8. Tasks

> Granularity: 2–5 min/step. TDD applies to `apdu_parse` — it is a **pure static function** with no Zephyr dependencies, fully testable by compiling `apdu_assembler.c` into the ztest suite. Test path: `tests/unit/apdu_assembler/` (standalone `CMakeLists.txt` + `prj.conf` + `testcase.yaml`, built `--no-sysbuild`; Twister tag `sigmation.nfc.apdu_assembler`). `native_sim` is Linux-CI-only; use `qemu_cortex_m33` locally. Commit after each numbered group.

- [ ] **1. Scaffolding.**
  Create `src/nfc/framing/CMakeLists.txt`:
  ```cmake
  if(CONFIG_NFC_STACK)
    target_sources(app PRIVATE
      apdu_assembler.c
      apdu_assembler_shell_cmds.c)
    target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  ```
  Create `src/nfc/framing/Kconfig` with `NFC_APDU_EXTENDED_SUPPORT` (bool, default y, depends on `NFC_STACK`). Create `tests/unit/apdu_assembler/` directory with minimal `CMakeLists.txt` + `prj.conf` + `testcase.yaml` (built `--no-sysbuild`). **Commit.**

- [ ] **2. `apdu_types.h`.**
  Write `src/nfc/framing/apdu_types.h` with include guard `SRC_NFC_FRAMING_APDU_TYPES_H`. Contents: `#include <stdint.h>` + `#include <stdbool.h>`; all `NFC_SW_*` constants with `U` suffix; `NFC_SW1`/`NFC_SW2` macros; `NFC_CLA_*`; `NFC_INS_*`; `NFC_SELECT_P1_*`; `nfc_apdu_t` struct with full Doxygen (zero-copy ownership warning, `data == NULL` when `lc == 0`, Le=0 semantics for short vs extended). No kernel headers included.
  **Commit.**

- [ ] **3. `apdu_assembler.h`.**
  Write `src/nfc/framing/apdu_assembler.h` with include guard. Includes: `apdu_types.h`, `hal/nfc_transport.h` (for `nfc_transport_ops_t`), `zephyr/net/buf.h`. Memory comment at top. Define `apdu_assembler_config_t` + `APDU_ASSEMBLER_CONFIG_DEFAULT`; `apdu_assembler_stats_t` (8 fields); `apdu_assembler_state_t`; all 6 public prototypes with `@`-annotations and Doxygen including net_buf ownership note on `on_apdu`.
  **Commit.**

- [ ] **4. (TDD) `apdu_parse` — Cases 1 and 2S (base cases).**
  In `apdu_assembler.c`, write the internal `apdu_parse_result_t` enum and the pure static function:
  ```c
  static apdu_parse_result_t apdu_parse(const uint8_t *data, size_t len,
                                         bool extended_ok, nfc_apdu_t *out);
  ```
  Implement the `len < 4` reject and Case 1 (`len == 4`) and Case 2S (`len == 5`) branches only. In `tests/unit/apdu_assembler/test_apdu_parse.c` (compiles `apdu_assembler.c` directly):
  - `test_case1_ok`: len=4, arbitrary CLA/INS/P1/P2 → OK, `lc==0, data==NULL, has_le==false`.
  - `test_case2s_le_nonzero`: len=5, `buf[4]=0x7FU` → OK, `le==0x7FU, has_le==true, extended==false`.
  - `test_case2s_le_zero`: len=5, `buf[4]=0x00U` → OK, `le==0U, has_le==true` (Le=0 means 256).
  - `test_too_short_0` through `test_too_short_3`: len=0..3 → `APDU_PARSE_REJECT_TOO_SHORT`.
  All pass before continuing. **Commit.**

- [ ] **5. (TDD) Short Lc — Cases 3S and 4S.**
  Extend `apdu_parse` with short Lc branches and malform detection:
  - Case 3S: `lc_s = data[4], lc_s != 0, len == 5U + (size_t)lc_s`.
  - Case 4S: `lc_s != 0, len == 6U + (size_t)lc_s`, short Le = `data[5 + lc_s]`.
  - Malform: `lc_s != 0, len != 5+lc_s and len != 6+lc_s` → `REJECT_LENGTH_MISMATCH`.
  - Edge: `len == 6, data[4] == 0` → `REJECT_LENGTH_MISMATCH`.
  Ztests:
  - `test_case3s_lc1`: 6-byte APDU, `data[4]=1U` → OK, `lc==1U, &data[5]`.
  - `test_case3s_lc255`: 260-byte APDU, `data[4]=255U` → OK, `data==&buf[5], lc==255U`.
  - `test_case4s_lc1_le0`: 7-byte, `data[4]=1U, data[6]=0U` → OK, `le==0U, has_le==true`.
  - `test_case4s_lc2_le0x80`: 9-byte, `lc=2, le=0x80U` → OK, `le==0x80U`.
  - `test_short_lc_malform_over`: `len=8, data[4]=2U` → `REJECT_LENGTH_MISMATCH` (expects 7 or 8, but len=8 matches Case 4S? Check: `6+2=8` → actually Case 4S OK! Ensure test is not a false reject. Pick len=9, lc=2 → malform).
  - `test_len6_buf4_zero`: len=6, `data[4]=0U` → `REJECT_LENGTH_MISMATCH`.
  All pass. **Commit.**

- [ ] **6. (TDD) Extended Cases 2E / 3E / 4E + disabled path.**
  Extend `apdu_parse` with extended branch (guarded by `extended_ok`):
  - Case 2E: `len==7, data[4]==0`.
  - `len>7, data[4]==0, lc_e==0` → `REJECT_EXTENDED_LC_ZERO`.
  - Case 3E: `lc_e!=0, len == 7U + (size_t)lc_e`.
  - Case 4E: `lc_e!=0, len == 7U + (size_t)lc_e + 2U`.
  - All extended when `!extended_ok` → `REJECT_EXTENDED_DISABLED`.
  - Malform: `data[4]==0, len>=8, does not match any extended case` → `REJECT_LENGTH_MISMATCH`.
  Ztests:
  - `test_case2e_le0`: len=7, `{CLA,INS,P1,P2,0,0,0}` → OK, `le==0U, has_le==true, extended==true`.
  - `test_case2e_le_max`: len=7, `buf[5..6]={0xFF,0xFF}` → OK, `le==0xFFFFU`.
  - `test_case3e_lc1`: len=8, lc_e=1 → OK, `data==&buf[7], lc==1U, extended==true`.
  - `test_case4e_lc2_le0`: len=11, lc_e=2, `{0,0}` Le → OK, `le==0U, has_le==true, extended==true`.
  - `test_extended_disabled_case2e`: len=7, `buf[4]=0`, `extended_ok=false` → `REJECT_EXTENDED_DISABLED`.
  - `test_extended_disabled_case3e`: any case 3E, `extended_ok=false` → `REJECT_EXTENDED_DISABLED`.
  - `test_extended_lc_zero`: len=8, `{CLA,INS,P1,P2,0,0,0,0xFF}` → `REJECT_EXTENDED_LC_ZERO`.
  - `test_extended_len_mismatch`: len=9, lc_e=1 → expects len 8 or 10 → `REJECT_LENGTH_MISMATCH`.
  All pass. **Commit.**

- [ ] **7. Module statics + lifecycle.**
  In `apdu_assembler.c`:
  - Define all §4.1 statics (in order: `s_state`, `s_config`, `s_stats`, `s_stats_lock`, `s_sw_reject_buf`, forward-declares for handlers, `s_ops_impl`).
  - `LOG_MODULE_REGISTER(apdu_assembler, CONFIG_LOG_DEFAULT_LEVEL)`.
  - Implement `apdu_assembler_init(cfg)`:
    1. `if (s_state != APDU_ASSEMBLER_STATE_UNINITIALIZED) { return -EALREADY; }`
    2. `STATS_RESET(s_stats);`
    3. `s_config = (cfg != NULL) ? *cfg : APDU_ASSEMBLER_CONFIG_DEFAULT;`
    4. `s_state = APDU_ASSEMBLER_STATE_INITIALIZED;`
    5. `return 0;`
  - Implement `apdu_assembler_shutdown()`: `s_state = APDU_ASSEMBLER_STATE_UNINITIALIZED; return 0;`
  **Commit.**

- [ ] **8. Getters.**
  Implement `apdu_assembler_get_config` (`return &s_config`), `apdu_assembler_get_stats` (`return STATS_COPY_OUT(&s_stats_lock, s_stats, out)`), `apdu_assembler_get_state` (plain `return s_state`), `apdu_assembler_get_ops` (`return &s_ops_impl`).
  **Commit.**

- [ ] **9. `on_field_on` and `on_field_off` handlers.**
  Implement `apdu_assembler_on_field_on_handler(void *user_ctx)`:
  - `ARG_UNUSED(user_ctx);`
  - `STATS_INC(&s_stats_lock, s_stats, field_on_count);`

  Implement `apdu_assembler_on_field_off_handler(void *user_ctx)`:
  - `ARG_UNUSED(user_ctx);`
  - `STATS_INC(&s_stats_lock, s_stats, field_off_count);`
  - `/* TODO(Wave 3): aid_router_field_off(); */` — build-order marker only; the locked contract (§3) is the direct call. Wave 3's integration task (wave3-router.md task 14) replaces this marker AND the task-10 dispatch marker with the real calls.
  **Commit.**

- [ ] **10. `on_apdu` handler.**
  Implement `apdu_assembler_on_apdu_handler(struct net_buf *apdu_buf, void *user_ctx)` per §3 contract:
  - `ARG_UNUSED(user_ctx);`
  - `STATS_INC(&s_stats_lock, s_stats, apdu_rx_count);`
  - Defensive state check: `if (s_state != APDU_ASSEMBLER_STATE_INITIALIZED) { net_buf_unref(apdu_buf); return; }`
  - `nfc_apdu_t apdu = { 0 };`
  - `apdu_parse_result_t result = apdu_parse(apdu_buf->data, (size_t)apdu_buf->len, s_config.extended_apdu_support, &apdu);`
  - Failure path: `(void)nfc_transport_send_response(...); STATS_INC(...parse_reject...); net_buf_unref(apdu_buf); return;`
  - Success path: `STATS_INC(...parse_ok...); if (apdu.extended) STATS_INC(...extended...); /* TODO(Wave 3): aid_router_dispatch(&apdu); */ net_buf_unref(apdu_buf);` — the dispatch line is a build-order marker only; the locked contract (§3) is the direct parsed-form call. Wave 3's integration task (wave3-router.md task 14) replaces this marker AND the task-9 field-off marker with the real calls.
  **Commit.**

- [ ] **11. Shell.**
  Write `apdu_assembler_shell_cmds.c`:
  - Include `apdu_assembler.h`, `<zephyr/shell/shell.h>`.
  - `cmd_apdu_assembler_config`: `get_config()` → `shell_print` `extended_apdu_support`.
  - `cmd_apdu_assembler_stats`: `get_stats(&snap)` → `shell_print` all 8 fields; print `last_error_code` as signed.
  - `cmd_apdu_assembler_state`: `get_state()` → `switch` with string map and `default` arm → `shell_print`.
  - `SHELL_STATIC_SUBCMD_SET_CREATE` + `SHELL_CMD_REGISTER` per §5.4.
  `ARG_UNUSED(argc)` and `ARG_UNUSED(argv)` in each handler → DEV-ZEP-016.
  **Commit.**

- [ ] **12. Build + lint.**
  Build with `CONFIG_NFC_STACK=y, CONFIG_NFC_APDU_EXTENDED_SUPPORT=y` on NCS v3.2.4 (`nrf54l15dk/nrf54l15/cpuapp`). Run the ztest suite (`qemu_cortex_m33`; `native_sim` is Linux-CI-only). Run:
  ```
  cppcheck --enable=all --addon=misra.py \
           --suppressions-list=misra-suppressions.txt \
           src/nfc/framing/
  ```
  Verify DEV-ZEP-008/009/016 suppressions cover all flagged kernel-macro uses. Confirm no new deviations are required beyond those cited in §6. Document the `nfc_work_q` stack budget contribution from this layer in a note for the HAL README (framing's per-APDU stack frame is ~`sizeof(nfc_apdu_t)` + return frame ≈ 32 B; negligible vs the 2048 B stack). **Commit.**

---

### DECISION Flags Raised (for Human Review)

| # | Description | Section |
|---|-------------|---------|
| DECISION-1 | `nfc_apdu_t.extended` field added (absent from spec v1 §6.2; present in spec v2); needed for Le=0 disambiguation. Kept; flows through to services — revisit at Wave 5 review. | §1.1 |
| DECISION-2 | `nfc_apdu_t.data == NULL` when `lc == 0`; spec is silent; prevents dangling-pointer dereference. | §1.1 |
| DECISION-3 | `apdu_assembler_stats_t` fields revised from spec `nfc_framing_stats_t` — renamed to match CONVENTIONS §2; fields reworked to reflect what framing actually tracks. | §1.3 |
| DECISION-4 | Ops struct getter form (`apdu_assembler_get_ops()` returning `const nfc_transport_ops_t *`) chosen over exporting individual named handler functions. | §2 |
| DECISION-5 | `user_ctx = NULL` for framing ops registration — singleton, file-static state, no context pointer needed. | §2 |
| DECISION-6 | `get_state` is `@caller_sync` (not `@isr_safe`) — Pattern A plain enum; no ISR state reads in steady state. | §2 |
| DECISION-7 | Router dispatch API locked to the parsed form `aid_router_dispatch(const nfc_apdu_t *)` (wave3-router.md DECISION-7; spec v2 §6.2/§6.3). Framing's call site: `aid_router_dispatch(&apdu)`. | §5.2 |
| DECISION-8 | Framing calls `nfc_transport_send_response` directly for parse-reject 6700 — not in CONVENTIONS §3 coupling map which shows `service → HAL` only. Architecturally correct as terminal error handler. | §5.2 / §6 |
| DECISION-9 | Extended Lc=0 with `len > 7` treated as malformed (6700). Spec is silent; conservative rejection chosen. | §3 parse table |
| DECISION-10 | `CONFIG_NFC_APDU_EXTENDED_SUPPORT` Kconfig symbol introduced here; now part of the spec v2 §9 symbol table (defined in `src/nfc/framing/Kconfig`, `rsource`d from `src/nfc/Kconfig` per wave4-stack.md §1.6). | §1.6 |
