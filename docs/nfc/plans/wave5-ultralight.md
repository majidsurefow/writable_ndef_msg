# Wave 5c: Ultralight Service Implementation Plan

**Status:** DRAFT — 2026-06-12

**Layer:** `services/ultralight` — Ultralight page data-model adapter  
**Files produced:**
- `src/nfc/services/ultralight/ultralight_service.h`
- `src/nfc/services/ultralight/ultralight_service.c`
- `src/nfc/services/ultralight/ultralight_service_shell_cmds.c`
- `src/nfc/services/ultralight/CMakeLists.txt`
- `src/nfc/services/ultralight/Kconfig`
- `tests/unit/nfc_ultralight/test_ultralight_tlv.c` (ztest — TLV parser, pure logic; Twister tag `sigmation.nfc.ultralight`)
- `tests/unit/nfc_ultralight/test_ultralight_ser.c` (ztest — serialize/deserialize round-trip)
- `tests/unit/nfc_ultralight/CMakeLists.txt`

**Depends on:**
- `docs/superpowers/plans/wave1-hal.md` (LOCKED — 2026-06-12)
- `docs/superpowers/plans/wave2-framing.md` (LOCKED — 2026-06-12)
- `docs/superpowers/plans/wave3-router.md` (LOCKED — 2026-06-12) — `service.h` vtable, `NFC_PERSIST_ID_ULTRALIGHT = 0x03U`
- `docs/superpowers/plans/wave4-stack.md` (LOCKED — 2026-06-12) — DECISION-6, profile save/load array
- `docs/superpowers/plans/wave5-ndef.md` (LOCKED — 2026-06-12) — `int ndef_service_set_content(const uint8_t *msg, size_t len)` locked per its DECISION-1

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 lifecycle, §3 coupling, §4 callback pattern, §5 buffer model, §6 stats recipe, §7 threading, §9 errno, §10 shell, §11 MISRA, §12 compliance checklist)
2. `docs/superpowers/plans/wave3-router.md` §1 — `nfc_service_t` vtable VERBATIM, `NFC_PERSIST_ID_ULTRALIGHT 0x03U`, `nfc_service_serialize_fn` / `nfc_service_deserialize_fn` typedefs
3. `docs/superpowers/plans/wave4-stack.md` §1.5 + §4.5 + §5.3 — DECISION-6 (ULTRALIGHT profile registers NDEF AID using `ndef_service_get()`), save/load array construction for ULTRALIGHT profile, `ultralight_service_init/shutdown` call sites
4. `docs/superpowers/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) §6.4.3, §6.5, §13 — Ultralight scope ("NDEF wrapper only, T2T commands impossible on ISO-DEP hardware"), store TLV-blob format, hardware constraints table
5. `flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight.h` — `MfUltralightType` enum, `MfUltralightPage`, `MfUltralightVersion`, `MfUltralightCounter`, `MfUltralightConfigPages`, page count table (`MfUltralightTypeOrigin`=16, `MfUltralightTypeUL11`=20, `MfUltralightTypeUL21`=41); page-count values re-expressed in service-owned constants, GPL code not reproduced
6. `flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight.c` — `mf_ultralight_features[]` table for total_pages per variant (UL11=20, UL21=41, Origin=16); data format version 2 reference
7. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_RESET`, `STATS_COPY_OUT` macro signatures
8. `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001 through DEV-ZEP-018
9. `/Users/majidfaroud/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012 rules

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (replaced prototype). Flipper Zero GPL listener implementation (`mf_ultralight_listener.c`) referenced for context only — no code reproduced.

---

> **Architecture Framing — spec v3 (2026-06-12):** This service is the
> **NFCT/card-role first-slice** of the **Ultralight** protocol module as defined by the
> [NFC Stack Architecture v3](../../specs/2026-06-12-nfc-stack-architecture.md) (§4.3).
> A protocol module owns: data model (`s_pages[]`) · (de)serialize ·
> **listener** · **poller** (reader role — RESERVED, not implemented in this slice).
> In this NFCT-only slice, the T4T listener role is fulfilled via the NDEF service
> adapter (wave4 DECISION-6: `NFC_PROFILE_ULTRALIGHT` registers the NDEF AID + NDEF
> service; this service owns the data model + serialize and injects content into the
> NDEF service on `deserialize()`). **Lane:** `NFC_LANE_ISO_DEP` — Type-4, dispatched
> via APDU framing (Wave 2) + AID router (Wave 3). **Kconfig:** `NFC_SERVICE_ULTRALIGHT`
> enables this protocol module; the listener compiles under `CONFIG_NFC_ROLE_CARD`
> (orthogonal, Wave 1 — here satisfied by the NDEF service). Existing symbol names are
> unchanged. **Serialize completeness:** output classified
> `NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED`
> (wave6 canonical C symbols — wave6-store.md §1.7 / spec v3 §4.5).

---

## Architecture Resolution (Binding — Do Not Redesign)

This section summarises the resolved architecture that constrains all decisions below.

**The nRF NFCT peripheral is ISO-DEP / T4T only for this slice.** Type 2 Tag (T2T) commands (`READ 0x30`, `WRITE 0xA2`, etc.) are not reachable on this hardware via the `nfc_t4t_lib` path used in Wave 1. The Ultralight service is **not** a protocol handler in this slice.

> **OPEN — native T2T emulation pending verification (spec v3 §4.3):**
> The NFCT also has `nfc_t2t_lib` (Type-2 emulation). Whether `nfc_t2t_lib` exposes
> the WRITE/command handling needed for full Ultralight emulation is **to be verified**.
> Until verified, the NDEF-Type-4-adapter approach below **stands unchanged**.

**Routing:** For `NFC_PROFILE_ULTRALIGHT`, `stack_register_profile()` (wave4 §4.5, DECISION-6) registers `ndef_service_get()` under the NDEF AID (`D2 76 00 00 85 01 01`). The ultralight service is **never** registered with `aid_router_register()`.

**Ultralight's role:** Own a MIFARE Ultralight page data model (pages 0..N-1, each 4 bytes); parse the NDEF message out of the Type 2 TLV structure in those pages; inject it into the NDEF service via the content-injection API being locked by `wave5-ndef.md`.

**Data flow — provisioning path (primary):**
```
nfc_stack_load(tag)
  └─ nfc_store_load(tag, svcs, n)
       └─ ultralight_service.deserialize(blob, len, user_ctx)
            ├─ restore s_pages[n][4]
            ├─ ultralight_extract_ndef_tlv(s_pages, pages_total, &msg, &msg_len)
            └─ ndef_service_set_content(msg, msg_len)   /* locked: wave5-ndef.md DECISION-1 */
```

**Data flow — save path (debug capture):**
```
nfc_stack_save(tag)
  └─ nfc_store_save(tag, svcs, n)
       └─ ultralight_service.serialize(out, max, &out_len, user_ctx)
            └─ writes s_pages[n][4] + metadata → out[]
               (does NOT query ndef_service for updated content)
```

---

## 1. Types and Constants

### 1.1 Module-Scope Constants

```c
/* src/nfc/services/ultralight/ultralight_service.h */

/**
 * Maximum bytes produced by serialize() for the default variant
 * (UL21, 41 pages with version + signature + counters + tearing flags).
 *
 * Layout: 4-byte header + pages_total*4 + 8 (version) + 32 (signature) +
 *         9 (3 counters × 3 bytes) + 3 (tearing flags) = max 220 bytes.
 * 256 provides headroom. Must be ≥ actual serialized size; a static
 * assertion in ultralight_service.c guards this for the default page count.
 *
 * Increase this constant if CONFIG_NFC_ULTRALIGHT_PAGES > 41.
 */
#define ULTRALIGHT_SERVICE_MAX_SERIALIZED  256U

/** 4 bytes per page (T2T page size, NXP MIFARE Ultralight datasheet). */
#define ULTRALIGHT_PAGE_SIZE               4U

/** Capability Container page index (page 3 in all T2T variants). */
#define ULTRALIGHT_CC_PAGE                 3U

/** T2T CC magic byte (NFC Forum T2T §6.3). */
#define ULTRALIGHT_CC_MAGIC                0xE1U

/** T2T NDEF TLV tag. */
#define ULTRALIGHT_TLV_NDEF                0x03U

/** T2T Terminator TLV tag. */
#define ULTRALIGHT_TLV_TERMINATOR          0xFEU

/** T2T NULL TLV tag (skip). */
#define ULTRALIGHT_TLV_NULL                0x00U

/** Long-form length escape byte (0xFF → next 2 bytes are big-endian length). */
#define ULTRALIGHT_TLV_LONG_FORM           0xFFU

/** First data page index (pages 0-3 are header/OTP/CC). */
#define ULTRALIGHT_DATA_START_PAGE         4U

/** Signature size in bytes (EV1 ECDSA signature over UID). */
#define ULTRALIGHT_SIGNATURE_SIZE          32U

/** Counter size in bytes (3-byte little-endian counter). */
#define ULTRALIGHT_COUNTER_SIZE            3U

/** Number of counters (EV1: 3). */
#define ULTRALIGHT_COUNTER_NUM             3U

/** Tearing flag count (EV1: 3). */
#define ULTRALIGHT_TEARING_FLAG_NUM        3U

/** Version struct size in bytes (NXP GET_VERSION response). */
#define ULTRALIGHT_VERSION_SIZE            8U

/** Internal serialized blob format version — increment on breaking changes. */
#define ULTRALIGHT_BLOB_FORMAT_VERSION     0x01U
```

### 1.2 Variant Type Enum

> **DECISION-UL-2 (variant choice — emulated type and page count):** The service defaults to emulating a **MIFARE Ultralight EV1 MF0UL21** (41 pages). Rationale: (a) 41 pages × 4 = 164 bytes raw; data pages 4-36 = 33 × 4 = 132 bytes of user memory, sufficient for typical URI/text NDEF payloads within `CONFIG_NFC_NDEF_MAX_SIZE`; (b) EV1 is more widely recognised by modern readers than the 16-page Origin variant; (c) version and counter fields are available for faithful card cloning; (d) the page count is Kconfig-tunable via `CONFIG_NFC_ULTRALIGHT_PAGES` (range 16..235, default 41) so callers can provision classic (16) or NTAG-sized data. The service does NOT emulate NTAG variants by default — NTAG introduces different CC semantics and protection registers that are outside this scope.

```c
/**
 * @brief Ultralight variant — determines default page count, CC bytes,
 *        and which optional fields (version/signature/counters) are present
 *        in the serialized blob.
 *
 * Values are stable — do not renumber. Used in the serialized blob format.
 */
typedef enum {
    UL_VARIANT_ORIGIN = 0U,   /**< Classic MF Ultralight, 16 pages, no version */
    UL_VARIANT_UL11   = 1U,   /**< EV1 MF0UL11, 20 pages, 1 counter, version present */
    UL_VARIANT_UL21   = 2U,   /**< EV1 MF0UL21, 41 pages, 3 counters, version present */
    UL_VARIANT_UNKNOWN = 0xFFU, /**< Unrecognised; treat as ORIGIN for page scanning */
} ultralight_variant_t;
```

### 1.3 Module Contract Types

```c
/**
 * @brief Ultralight service configuration (frozen after init()).
 * NULL cfg passed to init() → built-in default.
 */
typedef struct {
    /** Variant to emulate. Governs CC byte generation and page count default. */
    ultralight_variant_t variant;
} ultralight_service_config_t;

#define ULTRALIGHT_SERVICE_CONFIG_DEFAULT \
    ((ultralight_service_config_t){ .variant = UL_VARIANT_UL21 })

/**
 * @brief Ultralight service statistics.
 *
 * error_count and last_error_code are mandatory (CONVENTIONS §6).
 */
typedef struct {
    uint32_t deserialize_count;        /**< Successful deserialize() calls */
    uint32_t serialize_count;          /**< Successful serialize() calls */
    uint32_t tlv_parse_error_count;    /**< Malformed TLV in page data (graceful) */
    uint32_t ndef_inject_count;        /**< Successful ndef_service_set_content() calls */
    uint32_t ndef_inject_error_count;  /**< ndef_service_set_content() returned non-zero */
    uint32_t error_count;              /**< Mandatory — STATS_ERROR increments this */
    int32_t  last_error_code;          /**< Mandatory — last code passed to STATS_ERROR */
} ultralight_service_stats_t;

/**
 * @brief Ultralight service lifecycle state (Pattern A, CONVENTIONS §2).
 */
typedef enum {
    ULTRALIGHT_SERVICE_STATE_UNINITIALIZED = 0,  /**< Zero-init default; before init() */
    ULTRALIGHT_SERVICE_STATE_INITIALIZED,         /**< After init(); ready for use */
} ultralight_service_state_t;
```

### 1.4 Serialized Blob Format

> **DECISION-UL-7 (serialized blob format):** The blob is a compact binary format tagged with a format-version byte for future extensibility. Fixed-size optional fields are guarded by a `has_*` byte so a round-trip through the store does not lose fidelity when some fields are unavailable. The store layer uses `persist_id = NFC_PERSIST_ID_ULTRALIGHT = 0x03U` as the outer TLV tag; the inner format is service-owned.

```
Blob layout (all bytes; little-endian multi-byte values):
  Byte  0:     format_version (= ULTRALIGHT_BLOB_FORMAT_VERSION = 0x01)
  Byte  1:     variant (ultralight_variant_t, u8)
  Bytes 2-3:   pages_total (uint16_t LE; 1..CONFIG_NFC_ULTRALIGHT_PAGES)
  Bytes 4..(4 + pages_total*4 - 1):
               raw page data, page[0][0..3] .. page[pages_total-1][0..3]
  Byte  4 + pages_total*4:
               has_version (0x00 = absent, 0x01 = present)
  [if has_version]  8 bytes: MfUltralightVersion-compatible struct
  Byte  5 + pages_total*4 [+ 8 if has_version]:
               has_signature (0x00 / 0x01)
  [if has_signature]  32 bytes: ECDSA signature
  [next byte]: counter_count (0 or ULTRALIGHT_COUNTER_NUM)
  [if counter_count > 0]  counter_count * 3 bytes: raw counter data (LE)
  [next byte]: tearing_flag_count (0 or ULTRALIGHT_TEARING_FLAG_NUM)
  [if > 0]  tearing_flag_count bytes: tearing flags
```

`ULTRALIGHT_SERVICE_MAX_SERIALIZED = 256U` is sufficient for all defined variants up to 41 pages (actual max = 220 bytes). A `BUILD_ASSERT` in `ultralight_service.c` guards:

```c
BUILD_ASSERT(
    4U + (CONFIG_NFC_ULTRALIGHT_PAGES * ULTRALIGHT_PAGE_SIZE) +
    1U + ULTRALIGHT_VERSION_SIZE +
    1U + ULTRALIGHT_SIGNATURE_SIZE +
    1U + (ULTRALIGHT_COUNTER_NUM * ULTRALIGHT_COUNTER_SIZE) +
    1U + ULTRALIGHT_TEARING_FLAG_NUM
    <= ULTRALIGHT_SERVICE_MAX_SERIALIZED,
    "ULTRALIGHT_SERVICE_MAX_SERIALIZED too small for CONFIG_NFC_ULTRALIGHT_PAGES"
);
```

---

## 2. Public API

```c
/* src/nfc/services/ultralight/ultralight_service.h */

#include "router/service.h"   /* nfc_service_t, nfc_service_serialize_fn, etc. */

/* ── Lifecycle ──────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize the Ultralight service.
 *
 * Clears module stats, sets the configuration, initialises the page buffer
 * to a default card state (CC bytes for the selected variant at page 3,
 * all other pages zeroed), and attempts NDEF TLV extraction from the default
 * page content. Because the default pages contain no NDEF TLV, no
 * ndef_service_set_content() call is made during bare init — the caller is
 * expected to follow with nfc_stack_load() to provision real card data.
 *
 * cfg == NULL → ULTRALIGHT_SERVICE_CONFIG_DEFAULT.
 *
 * @param cfg  Configuration; frozen after return. May be NULL.
 * @retval  0        Success; state → INITIALIZED.
 * @retval -EALREADY Already INITIALIZED (stats not wiped; see CONVENTIONS §6).
 * @caller_sync
 */
int ultralight_service_init(const ultralight_service_config_t *cfg);

/**
 * @brief Shut down the Ultralight service.
 *
 * Clears the page buffer, resets state to UNINITIALIZED. Always returns 0
 * (creed §1 — shutdown must not fail).
 *
 * @retval 0  Always.
 * @caller_sync
 */
int ultralight_service_shutdown(void);

/* ── Module contract getters ─────────────────────────────────────────────────── */

/**
 * @brief Return a pointer to the frozen configuration.
 * Valid before init(). Never NULL.
 * @isr_safe (read-only after init)
 */
const ultralight_service_config_t *ultralight_service_get_config(void);

/**
 * @brief Copy out the current stats under spinlock.
 * @param out  Caller-owned buffer; must not be NULL.
 * @retval  0        Success.
 * @retval -EINVAL   out is NULL.
 * @threadsafe
 */
int ultralight_service_get_stats(ultralight_service_stats_t *out);

/**
 * @brief Return the current lifecycle state. Never fails.
 * @caller_sync
 */
ultralight_service_state_t ultralight_service_get_state(void);

/* ── Service vtable accessor ─────────────────────────────────────────────────── */

/**
 * @brief Return a pointer to the static nfc_service_t vtable.
 *
 * Used by nfc_stack_save() / nfc_stack_load() to build the service pointer
 * array for the ULTRALIGHT profile. The returned pointer is file-static and
 * valid for the lifetime of the application.
 *
 * The vtable has:
 *   on_select/on_apdu/on_deselect/on_field_off = defensive no-op stubs
 *   (never invoked — service is never router-registered; see §4.1)
 *   serialize  = ultralight_serialize   (non-NULL)
 *   deserialize = ultralight_deserialize (non-NULL)
 *   persist_id = NFC_PERSIST_ID_ULTRALIGHT (0x03)
 *   user_ctx   = NULL (singleton; file-static state)
 *
 * @retval  Pointer to static nfc_service_t; never NULL.
 * @caller_sync
 */
const nfc_service_t *ultralight_service_get(void);
```

---

## 3. Contracts

### 3.1 `ultralight_service_init(cfg)`

**Pre:**
- Module state is `UNINITIALIZED` (else `-EALREADY`).
- May be called from any single thread that owns lifecycle; no other thread accesses module state concurrently at init time.

**Post (0):**
- State == `INITIALIZED`.
- `s_config` frozen to `*cfg` (or default if `cfg == NULL`).
- `s_pages[0..pages_total-1]` zeroed.
- Page 3 (CC page) set to default CC bytes for the configured variant (see §6.2 for exact values).
- `s_pages_total` == `CONFIG_NFC_ULTRALIGHT_PAGES`.
- `s_variant` == `cfg->variant` (or `UL_VARIANT_UL21` if NULL cfg).
- No NDEF TLV found in default pages → `ndef_service_set_content()` NOT called (no call on empty init).
- Stats reset (CONVENTIONS §6 ordering: reset is the first statement after the `-EALREADY` guard).

**Post (-EALREADY):** State unchanged; stats NOT reset.

**Error codes:**
| Code | Condition |
|------|-----------|
| `-EALREADY` | `s_state != UNINITIALIZED` |

### 3.2 `ultralight_service_shutdown()`

**Pre:** Any state (idempotent; calling from UNINITIALIZED is a no-op, returns 0).

**Post:** State == `UNINITIALIZED`. `s_pages` zeroed. Always returns 0.

### 3.3 `ultralight_service_get_config()` / `get_stats()` / `get_state()`

- `get_config`: Returns `&s_config`; valid before `init()` (zero-init default); never NULL.
- `get_stats`: Returns copy of `s_stats` under `s_stats_lock`. Returns `-EINVAL` if `out == NULL`.
- `get_state`: Returns `s_state` without synchronization (plain read; Pattern A, caller-sync lifecycle).

### 3.4 `ultralight_service_get()`

**Pre:** Any state.  
**Post:** Returns `&s_vtable` (file-static). Never NULL. The caller must not modify the returned struct.

### 3.5 serialize / deserialize (vtable callbacks — `@caller_sync`)

**serialize(`out, max, out_len, user_ctx`):**

- **Pre:** `s_state == INITIALIZED`. `out != NULL`. `out_len != NULL`. `max >= ULTRALIGHT_SERVICE_MAX_SERIALIZED`.
- **Post (0):** `out[0..*out_len-1]` contains the serialized blob (see §1.4 format). `*out_len <= max`.
- **Error codes:**
  | Code | Condition |
  |------|-----------|
  | `-EINVAL` | `out == NULL` or `out_len == NULL` |
  | `-ENOSPC` | `max < ULTRALIGHT_SERVICE_MAX_SERIALIZED` |
  | `-ENODEV` | `s_state != INITIALIZED` |

**deserialize(`in, len, user_ctx`):**

- **Pre:** `s_state == INITIALIZED`. `in != NULL`. `len > 0`.
- **Post (0):** `s_pages` restored from blob. If a valid NDEF TLV was found, `ndef_service_set_content()` was called (signature locked in `wave5-ndef.md` DECISION-1). `STATS_INC(deserialize_count)`.
- **Post (0, malformed TLV):** Pages restored; TLV scanning failed → `tlv_parse_error_count++`; `ndef_service_set_content()` NOT called; return 0.
  > **DECISION-UL-4 (malformed TLV = graceful deserialize):** Blob-level corruption returns `-EBADMSG` (blob is unreadable, no pages loaded). TLV-level malformation after successful blob parsing returns 0 (pages are valid data; NDEF structure may be absent in non-NDEF cards, partial writes, etc.). Failing deserialize on TLV errors would prevent loading valid card data. The `tlv_parse_error_count` stat makes the condition observable.

- **Error codes:**
  | Code | Condition |
  |------|-----------|
  | `-EINVAL` | `in == NULL` or `len == 0` |
  | `-EBADMSG` | Blob format_version unknown, pages_total > `CONFIG_NFC_ULTRALIGHT_PAGES`, or blob truncated (blob itself is corrupt) |
  | `-ENODEV` | `s_state != INITIALIZED` |

---

## 4. Internal State

### 4.1 Module-Level Statics

```c
/* ultralight_service.c — all file-static */

static ultralight_service_state_t  s_state;       /* Pattern A — @caller_sync lifecycle */
static ultralight_service_config_t s_config;       /* frozen after init() */
static ultralight_service_stats_t  s_stats;        /* protected by s_stats_lock */
static struct k_spinlock           s_stats_lock;   /* NOT K_SPINLOCK_DEFINE — CONVENTIONS §6 */

/**
 * Page buffer: s_pages[page_index][byte_within_page].
 * Index range 0..s_pages_total-1; s_pages_total <= CONFIG_NFC_ULTRALIGHT_PAGES.
 * Only written by init() (zeroed), deserialize() (restored from blob), and
 * shutdown() (zeroed). All writes are @caller_sync; no concurrent dispatch
 * because the stack is never STARTED during store operations (CONVENTIONS §3).
 */
static uint8_t  s_pages[CONFIG_NFC_ULTRALIGHT_PAGES][ULTRALIGHT_PAGE_SIZE];

/** Actual number of pages stored (from the most recent deserialize or init). */
static uint8_t  s_pages_total;

/** Variant for the currently loaded card model. */
static ultralight_variant_t  s_variant;

/** Optional EV1 version struct (8 bytes). Valid if s_has_version == true. */
static uint8_t  s_version[ULTRALIGHT_VERSION_SIZE];
static bool     s_has_version;

/** Optional EV1 ECDSA signature (32 bytes). */
static uint8_t  s_signature[ULTRALIGHT_SIGNATURE_SIZE];
static bool     s_has_signature;

/** Optional counters (up to 3 × 3 bytes). */
static uint8_t  s_counters[ULTRALIGHT_COUNTER_NUM][ULTRALIGHT_COUNTER_SIZE];
static uint8_t  s_counter_count;

/** Optional tearing flags (up to 3 bytes). */
static uint8_t  s_tearing_flags[ULTRALIGHT_TEARING_FLAG_NUM];
static uint8_t  s_tearing_flag_count;

/**
 * The nfc_service_t vtable for this service.
 * This service is NEVER registered with aid_router_register() (wave4
 * DECISION-6: the ULTRALIGHT profile registers the NDEF service). The
 * dispatch callbacks are nonetheless non-NULL — wave3 service.h states
 * "NULL not permitted" for the four core callbacks, so defensive no-op
 * stubs are used instead of NULL (DECISION-UL-1). If the vtable is ever
 * registered by mistake, the stubs bump misdispatch_count and respond
 * 6D00 (on_apdu) instead of crashing or staying silent.
 */
static void ultralight_on_select_stub(void *user_ctx);              /* counts misdispatch */
static void ultralight_on_apdu_stub(const nfc_apdu_t *apdu,
                                    void *user_ctx);                /* counts + sends 6D00 */
static void ultralight_on_deselect_stub(void *user_ctx);            /* no-op */
static void ultralight_on_field_off_stub(void *user_ctx);           /* no-op */

static const nfc_service_t s_vtable = {
    .on_select    = ultralight_on_select_stub,
    .on_apdu      = ultralight_on_apdu_stub,
    .on_deselect  = ultralight_on_deselect_stub,
    .on_field_off = ultralight_on_field_off_stub,
    .serialize    = ultralight_serialize,
    .deserialize  = ultralight_deserialize,
    .persist_id   = NFC_PERSIST_ID_ULTRALIGHT,  /* 0x03U */
    .user_ctx     = NULL,
};
```

### 4.2 State Machine

Minimal lifecycle — Pattern A (plain enum, `@caller_sync`):

```
UNINITIALIZED  ──[init() == 0]──►  INITIALIZED
INITIALIZED    ──[shutdown()]──►   UNINITIALIZED
```

Transitions that must not occur (guard with `-EALREADY` / `-ENODEV`):
- `init()` while INITIALIZED → `-EALREADY`; stats NOT reset.
- `serialize()`/`deserialize()` while UNINITIALIZED → `-ENODEV`.

No STARTED/STOPPED states: this service has no independent runtime. Serialization / deserialization are `@caller_sync` only (the stack is not STARTED during store operations — CONVENTIONS §3, wave3 §5.5).

### 4.3 Concurrency Table

| Variable | Written by | Read by | Sync |
|----------|-----------|---------|------|
| `s_state` | `init`/`shutdown` (caller) | `serialize`/`deserialize` (caller) | None (Pattern A; all caller-sync) |
| `s_config` | `init` (once; frozen) | `get_config` (any) | None (frozen read-only) |
| `s_stats.*` | `serialize`/`deserialize`/`init` (caller, hot path N/A) | `get_stats` (any) | `s_stats_lock` via STATS_* macros |
| `s_pages`, `s_pages_total`, `s_variant`, `s_version`, `s_signature`, `s_counters`, `s_tearing_flags` | `init`/`deserialize`/`shutdown` (caller) | `serialize` (caller) | None (all caller-sync; no WQ access) |

**The `-EBUSY` gating in `nfc_stack_save/load` (wave3 §5.5, wave4 §5.3)** means serialize and deserialize are never invoked while `nfc_work_q` dispatch is running. No spinlock is needed between the page buffer and the vtable callbacks.

---

## 5. Integration Points

### 5.1 Down — Dependencies Called by This Service

| Called API | Where defined | When called | Notes |
|------------|--------------|-------------|-------|
| `ndef_service_set_content(msg, len)` | `services/ndef/ndef_service.h` | Inside `deserialize()` after successful TLV extraction | Signature **locked** by `wave5-ndef.md` DECISION-1: `int ndef_service_set_content(const uint8_t *msg, size_t len)` |

> **DECISION-UL-5 (cross-service coupling — push model):** `ultralight_service.c` `#include`s `services/ndef/ndef_service.h` to call `ndef_service_set_content()`. This is a one-directional peer call (ultralight pushes into NDEF; NDEF does NOT include ultralight). The spec v2 §6.4.3 describes a pull model (`ultralight_service_get_ndef()` called by NDEF); the task-scope resolved design overrides this with a push model to eliminate a callback registration requirement and match the store-provisioning data flow. The ndef service must be initialized before deserialize is called; in wave4's init sequence, ndef is initialized before ultralight.

> **`ndef_service_set_content(NULL, 0)` on empty TLV:** If TLV scanning finds no NDEF message, `ndef_service_set_content()` is NOT called (no injection, preserving whatever the NDEF service already holds). If TLV parsing encounters a malformed structure, same: no call.

### 5.2 Up — Consumers of This Service

| Consumer | API used | Context | Notes |
|----------|---------|---------|-------|
| `nfc_stack_init()` | `ultralight_service_init(cfg)` | Caller thread, init sequence step 14 | wave4 §4.2 init sequence |
| `nfc_stack_shutdown()` | `ultralight_service_shutdown()` | Caller thread, shutdown step 7 (reverse order) | wave4 §4.3 |
| `nfc_stack_save/load(tag)` | via vtable: `s_vtable.serialize` / `s_vtable.deserialize` | Caller thread, @caller_sync | wave4 §5.3; stack must not be STARTED |
| `nfc_stack.c` | `ultralight_service_get()` | Caller thread, building svcs[] array | Returns `&s_vtable` for ULTRALIGHT profile save/load |
| Shell commands | `ultralight_service_get_config/stats/state()` | Shell thread | Read-only; copy-out under lock for stats |

**`nfc_stack.c` interaction — ULTRALIGHT profile save/load array (wave4 §5.3):**

> **DECISION-UL-9 (save/load array for ULTRALIGHT profile):** The service-pointer array passed to `nfc_store_save/load()` for `NFC_PROFILE_ULTRALIGHT` contains **only the ultralight service instance** (`n=1`, `svcs = {ultralight_service_get()}`). The NDEF service is not separately persisted for this profile because ultralight's `deserialize()` is responsible for both restoring the page model and injecting content into the NDEF service. Reader writes to the T4T NDEF file (which update the NDEF service's internal buffer) are **not** back-propagated into the ultralight page model at serialize time (see DECISION-UL-3). The updated NDEF content is therefore not persisted by a save under the ULTRALIGHT profile — this is acceptable given the save path's "debug capture" scope (wave4 §5.3). If the caller needs persistence of reader-written NDEF content, they should switch to `NFC_PROFILE_NDEF` and use that service's serialize path.

**Profile registration does NOT involve this service's vtable** — wave4 `stack_register_profile(NFC_PROFILE_ULTRALIGHT)` calls `aid_router_register(k_ndef_aid, ..., ndef_service_get())`. This plan does not change that; it is already locked in wave4 DECISION-6.

### 5.3 Internal Pure Helper — TLV Scanner

This function is the testable core of the service. It takes a flat byte view of the user-data area and returns the NDEF message location.

```c
/**
 * @brief Scan T2T TLV structure in a flat byte array.
 *
 * Processes the byte stream starting at index 0, handling:
 *   - 0x00 (NULL TLV): skip 1 byte
 *   - 0x03 (NDEF TLV): decode length (short/long form), set *msg_out/*msg_len_out
 *   - 0xFE (Terminator): stop
 *   - Other: decode length and skip the TLV body
 *
 * Scanning stops at the first NDEF TLV found, at the Terminator, or at
 * the end of data[].
 *
 * @param data      Flat byte array (pages[ULTRALIGHT_DATA_START_PAGE..] concatenated).
 * @param data_len  Length of data in bytes.
 * @param msg_out   [out] Pointer into data[] at start of NDEF message payload.
 *                  Set to NULL if no NDEF TLV found or TLV is malformed.
 * @param msg_len_out [out] Length of NDEF message in bytes. 0 if not found.
 * @retval  0   Success (NDEF found or not found — caller checks msg_out).
 * @retval -EINVAL  data, msg_out, or msg_len_out is NULL.
 * @retval -ENODATA TLV structure is malformed (overflow, truncated length, etc.).
 *
 * @note Pure function — no side effects, no globals accessed. Safe to unit-test
 *       without the rest of the stack (qemu_cortex_m33 or native_sim on Linux CI).
 * @caller_sync
 */
static int ultralight_scan_ndef_tlv(const uint8_t *data, size_t data_len,
                                    const uint8_t **msg_out, size_t *msg_len_out);
```

**TLV length decoding rules (short vs. long form):**

```
Short form:  length_byte in [0x00..0xFE]  → length = length_byte
Long form:   length_byte == 0xFF          → read next 2 bytes (big-endian) as uint16_t
             long form length must be in [0x00FF..0xFFFE] (values ≤ 0xFE in long form
             are permitted by T2T spec but unusual; accept them)
```

**Bounds enforcement in the scanner:**
- After reading a length, verify `offset + length ≤ data_len` before advancing. If violated: `msg_out = NULL`, `msg_len_out = 0`, return `-ENODATA`.
- If the end of `data` is reached before a Terminator is found: acceptable (end of user area acts as implicit terminator); return 0 with whatever was found.
- `length == 0` for NDEF TLV: valid (empty NDEF); `ndef_service_set_content` supports it (wave5-ndef.md §3.5: valid `msg` with `len == 0` sets NLEN to 0) — inject as 0-byte message.

### 5.4 Internal Pure Helper — CC Page Builder

```c
/**
 * @brief Write default Capability Container bytes to page 3.
 *
 * CC layout (NFC Forum T2T §6.3):
 *   Byte 0: 0xE1 (magic)
 *   Byte 1: 0x10 (version 1.0 for EV1 variants)
 *   Byte 2: (size of user memory in units of 8 bytes)
 *   Byte 3: 0x00 (read-write)
 *
 * @param pages       Page buffer (must have at least ULTRALIGHT_CC_PAGE+1 pages).
 * @param pages_total Total page count; determines CC2 user-memory size field.
 * @param variant     Selects the version byte (0x10 for EV1; 0x10 for Origin).
 */
static void ultralight_write_default_cc(
    uint8_t pages[][ULTRALIGHT_PAGE_SIZE],
    uint8_t pages_total,
    ultralight_variant_t variant);
```

CC2 value for each variant:
- Origin (16 pages): data pages 4-15 = 12 × 4 = 48 bytes → CC2 = `0x06U` (48/8 = 6)
- UL11  (20 pages): data pages 4-15 = 12 × 4 = 48 bytes (pages 16-19 are config, excluded from CC2) → CC2 = `0x06U`
- UL21  (41 pages): data pages 4-36 = 33 × 4 = 132 bytes (pages 37-40 config, excluded) → CC2 = `0x10U` (128/8 = 16; conservative rounding; NXP AN11022 gives 0x12 but 0x10 is safe for readers)

> **DECISION-UL-6b (CC2 value for UL21):** Use `0x10U` (128 bytes) rather than `0x12U` (144 bytes) for CC2 when writing default CC bytes on init. The slightly conservative value avoids reader assumptions that all 144 bytes are available. When deserializing from a real card blob, the CC bytes from the blob are preserved as-is (page 3 bytes are part of `s_pages[3]`); default CC is only written during bare `init()`.

### 5.5 RESERVED — `ultralight_poller` (Reader Role)

> **Not implemented in this slice.** When a reader-capable backend becomes available
> (`CONFIG_NFC_ROLE_READER` + ST25R3916/RFAL or PN7160), the Ultralight protocol
> module gains an `ultralight_poller` component that reads a physical Ultralight tag's
> raw pages via the **`NFC_LANE_RAW`** lane (wave3-router.md §1.1 — native Type-2 Tag
> commands: READ `0x30`, WRITE `0xA2`, etc.) into `s_pages[]`, then calls
> `ndef_service_set_content()` with the extracted NDEF payload. The result would
> serialize with completeness flag `NFC_STORE_ENTRY_FLAG_READER_CAPTURED`
> (wave6 canonical C symbol — wave6-store.md §1.7 / spec v3 §4.5).
> Note: `NFC_LANE_RAW` is itself a reserved seam; the NFCT backend supports
> ISO-DEP only — a reader-capable external controller is required for raw T2T access.
> **Header/interface placeholder only** — no `ultralight_poller` API is declared or
> compiled in this slice.

---

## 6. Implementation Notes

### 6.1 Resync Decision

> **DECISION-UL-3 (no resync at serialize time):** When `serialize()` is called, the service writes `s_pages[]` verbatim — it does NOT query `ndef_service_get_content()` (or any NDEF service API) to re-render pages from current NDEF content. Consequences:
> - If a reader wrote new NDEF data to the T4T file (via the NDEF service), that change is NOT reflected in the ultralight page model.
> - A subsequent `nfc_stack_save()` for the ULTRALIGHT profile will capture the original (provisioned) pages, not the reader-updated NDEF.
> Rationale: (a) adding a read-back path would couple ultralight to the NDEF service in both directions (circular); (b) T2T page-level rendering from an arbitrary NDEF message requires re-wrapping in TLV + CC + padding — non-trivial and error-prone; (c) save is documented as a "debug capture facility" (wave4 §5.3), not the primary write path. The limitation is documented here and in the shell stats output (`serialize_count` vs `ndef_inject_count` mismatch indicates the card was written by a reader).

### 6.2 Default CC Bytes per Variant

| Variant | CC Page 3 bytes | CC2 user mem |
|---------|----------------|-------------|
| Origin  | `E1 10 06 00`  | 48 bytes    |
| UL11    | `E1 10 06 00`  | 48 bytes    |
| UL21    | `E1 10 10 00`  | 128 bytes   |

Page 3 bytes 0-1 (lock bytes, part of page 2 and page 3) are zeroed in default init (no locks set). When a real card blob is deserialized, the CC page is restored from the blob unchanged.

### 6.3 ndef_service_set_content Dependency

The NDEF injection API signature is locked (`wave5-ndef.md` DECISION-1):

```c
int rc = ndef_service_set_content(msg_ptr, msg_len);
```

If wave5-ndef.md locks a different signature or a NULL-safe variant, only these call sites change. No other part of this service touches the NDEF service.

### 6.4 MISRA C:2012 Compliance Notes

| Concern | Rule | Resolution |
|---------|------|------------|
| `LOG_INF` / `LOG_ERR` variadic macros | MISRA 17.1 (variadic) | DEV-ZEP-008: Zephyr logging system; pre-approved. |
| `shell_print` variadic | MISRA 17.1 | DEV-ZEP-009: shell subsystem; pre-approved. |
| `BUILD_ASSERT` macro | MISRA 20.1 | DEV-ZEP-017 covers `BIT`/Zephyr compile-time assertion macros; acceptable. |
| `memset` in `STATS_RESET` | MISRA 21.6 (avoid `<string.h>` functions selectively) | `memset` is used solely for zero-filling a stats struct of known size; this is the project's established stats-reset pattern (wave3, wave4) and is explicitly part of the STATS API design. No UB risk. |
| Fixed-width types | MISRA 6.1 / 7.2 | All fields use `uint8_t` / `uint16_t` / `uint32_t` / `int32_t`. `U` suffix on all unsigned literals (e.g. `0xE1U`, `4U`). |
| Pointer arithmetic in TLV scanner | MISRA 18.4 | Use array indexing (`data[offset]`) rather than pointer arithmetic; `offset` is a `size_t` that is bounds-checked before each access. |
| `switch` without `default` | MISRA 16.4 | Every `switch` (variant enum, TLV tag scan) includes `default`. |
| Boolean-controlling expression | MISRA 14.4 | `if (s_state != ULTRALIGHT_SERVICE_STATE_INITIALIZED)` — explicit comparison, not implicit cast. |
| Variable not initialized at declaration | MISRA 9.1 | All locals initialized at declaration (`size_t offset = 0U;`, `const uint8_t *msg = NULL;`, etc.). |
| No dynamic allocation | MISRA 21.3 | All buffers are file-static or stack-allocated within bounded call chains. No `malloc`/`free`. |
| `bool` type | MISRA 6.1 | Use `bool` from `<stdbool.h>` for flag fields (`s_has_version`, `s_has_signature`). |

### 6.5 Threading Notes

All public API functions are `@caller_sync`. No function in this service is called from `nfc_work_q` (no dispatch callbacks). The `s_stats_lock` spinlock is the only synchronization primitive; it guards only the stats struct, consistent with the project recipe.

The `@caller_sync` annotation on `serialize`/`deserialize` is inherited from the vtable contract (wave3 §5.5): "Called from `nfc_stack_save/load()` on the caller thread; never while the stack is STARTED."

### 6.6 Static Buffer Note (CONVENTIONS §5)

The ultralight service uses no `net_buf` (no ISR or WQ data path). The page buffer `s_pages[]` is a BSS file-static array. `ULTRALIGHT_SERVICE_MAX_SERIALIZED` worth of output is written into the `out` buffer provided by the store (the store owns that buffer per its own design; this service does not own or cache it).

---

## 7. Conventions Compliance

- [x] **Lifecycle** — Minimal (`init`/`shutdown`); `shutdown` always returns 0 (CONVENTIONS §2).
- [x] **`config_t` / `stats_t` / `state_t` + three getters** — `ultralight_service_config_t`, `ultralight_service_stats_t`, `ultralight_service_state_t` + `get_config` / `get_stats` / `get_state` defined (§2, §3).
- [x] **State storage Pattern A** — plain enum `ultralight_service_state_t`, `@caller_sync` lifecycle (§2 table).
- [x] **Coupling** — Downward direct call to `ndef_service_set_content()` (peer-service push, DECISION-UL-5). No upward callbacks registered. No cross-layer wiring in this file — wiring happens in `nfc_stack.c` (CONVENTIONS §3).
- [x] **Callback pattern** — vtable callbacks (`serialize`/`deserialize`) are `_fn`-typed; `user_ctx` is NULL (singleton). No registration setter needed (vtable is static). Router NULL-checks before calling (CONVENTIONS §4).
- [x] **Buffer model** — No `net_buf` (no ISR/WQ path). Static BSS page buffer. Response buffer not applicable (no `on_apdu`). (CONVENTIONS §5)
- [x] **Stats recipe** — `s_stats` + `s_stats_lock` (NOT `K_SPINLOCK_DEFINE`); `STATS_RESET` as first statement after `-EALREADY` guard; copy-out getter; `STATS_INC`/`STATS_ERROR` on every path; named drop counter `tlv_parse_error_count`. (CONVENTIONS §6)
- [x] **Threading + annotations** — All functions `@caller_sync`. `get_stats` is `@threadsafe` (spinlock copy-out). `get_config` is `@isr_safe` (frozen read-only after init). (CONVENTIONS §7)
- [x] **Error codes** — Standard errno only: `-EINVAL`, `-EALREADY`, `-ENODEV`, `-ENOSPC`, `-EBADMSG`, `-ENODATA`. (CONVENTIONS §9)
- [x] **Shell** — `ultralight_service_shell_cmds.c` with `config`/`stats`/`state` subcommands; `SHELL_CMD_REGISTER`. (CONVENTIONS §10)
- [x] **MISRA notes** — DEV-ZEP-008, DEV-ZEP-009 cited; array indexing in TLV scanner; `switch` default; fixed-width types; `U` literals; no dynamic alloc. (CONVENTIONS §11)
- [x] **`NFC_SERVICE_ULTRALIGHT` enable symbol** — lives in `src/nfc/Kconfig` (wave4 §1.6, locked). `CONFIG_NFC_ULTRALIGHT_PAGES` tunable lives in `src/nfc/services/ultralight/Kconfig`.

---

## 8. Tasks

Tasks follow TDD where a pure/testable function exists. Commit points are marked `[COMMIT]`. Total estimate: ~90 minutes.

---

- [ ] **Task 1 — Kconfig (5 min)**

  Create `src/nfc/services/ultralight/Kconfig`:

  ```kconfig
  config NFC_ULTRALIGHT_PAGES
      int "Number of emulated Ultralight pages (4 bytes each)"
      default 41
      range 16 235
      depends on NFC_SERVICE_ULTRALIGHT
      help
        Total page count for the emulated Ultralight card.
        16 = classic MF Ultralight (Origin).
        20 = EV1 MF0UL11.
        41 = EV1 MF0UL21 (default — max NDEF ≈ 132 bytes user memory).
        Must be large enough to hold the provisioned card data.
        Increase ULTRALIGHT_SERVICE_MAX_SERIALIZED if this value exceeds 41.
  ```

  Verify `src/nfc/Kconfig` already has `rsource "services/ultralight/Kconfig"` (added by wave4 §1.6 under `NFC_SERVICE_ULTRALIGHT`; if absent, add it). `[COMMIT]`

---

- [ ] **Task 2 — CMakeLists.txt (3 min)**

  Create `src/nfc/services/ultralight/CMakeLists.txt`:

  ```cmake
  # src/nfc/services/ultralight/CMakeLists.txt
  if(CONFIG_NFC_SERVICE_ULTRALIGHT)
      target_sources(app PRIVATE
          ultralight_service.c
          ultralight_service_shell_cmds.c
      )
      target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  ```

  `[COMMIT]`

---

- [ ] **Task 3 — `ultralight_service.h` (10 min)**

  Create `src/nfc/services/ultralight/ultralight_service.h` with:
  - Include guard `SRC_NFC_SERVICES_ULTRALIGHT_ULTRALIGHT_SERVICE_H`.
  - `#include <stdint.h>`, `#include <stddef.h>`, `#include <stdbool.h>`.
  - `#include "router/service.h"`.
  - All `#define` constants from §1.1.
  - `ultralight_variant_t` enum (§1.2).
  - `ultralight_service_config_t`, `ultralight_service_stats_t`, `ultralight_service_state_t` (§1.3).
  - `ULTRALIGHT_SERVICE_CONFIG_DEFAULT` macro.
  - All six public function declarations from §2 with full Doxygen `@param`/`@retval`/`@caller_sync` annotations.

  `[COMMIT]`

---

- [ ] **Task 4 — TLV scanner (RED test first) (15 min)**

  **4a.** Create `tests/unit/nfc_ultralight/test_ultralight_tlv.c`. Write failing ztest cases FIRST:

  | Test case | Input | Expected |
  |-----------|-------|----------|
  | `test_null_tlv_skip` | `{0x00, 0x03, 0x03, 'N','F','C', 0xFE}` | msg found, len=3 |
  | `test_ndef_short_form` | `{0x03, 0x05, 'H','e','l','l','o', 0xFE}` | msg@offset 2, len=5 |
  | `test_ndef_long_form` | `{0x03, 0xFF, 0x00, 0x04, 'A','B','C','D', 0xFE}` | msg@offset 5, len=4 |
  | `test_terminator_only` | `{0xFE}` | msg=NULL, len=0, rc=0 |
  | `test_malformed_overflow` | `{0x03, 0x10, 'X'}` (only 1 data byte, len claims 16) | rc=-ENODATA |
  | `test_malformed_long_truncated` | `{0x03, 0xFF, 0x00}` (long form, only 1 byte follows) | rc=-ENODATA |
  | `test_unknown_tlv_skip` | `{0x42, 0x02, 0x00, 0x00, 0x03, 0x01, 'X', 0xFE}` | msg found, len=1 |
  | `test_empty_ndef_tlv` | `{0x03, 0x00, 0xFE}` | msg found, len=0 |
  | `test_no_data` | `{}` (empty array) | msg=NULL, len=0, rc=0 |
  | `test_null_args` | NULLs | rc=-EINVAL |

  Run tests → RED (function does not exist yet). `[COMMIT RED]`

  **4b.** Implement `ultralight_scan_ndef_tlv()` in `ultralight_service.c` (as a `static` function; expose via a `#ifdef CONFIG_ZTEST` friend-test declaration in the header). Run tests → GREEN. `[COMMIT GREEN]`

---

- [ ] **Task 5 — CC builder and `init()` / `shutdown()` (10 min)**

  Implement `ultralight_write_default_cc()` (§5.4 spec). Add a unit test:
  - `test_cc_ul21`: verify `pages[3] == {0xE1, 0x10, 0x10, 0x00}` after call with UL21.
  - `test_cc_origin`: verify `pages[3] == {0xE1, 0x10, 0x06, 0x00}` after call with Origin.

  Implement `ultralight_service_init()`:
  ```c
  int ultralight_service_init(const ultralight_service_config_t *cfg)
  {
      if (s_state != ULTRALIGHT_SERVICE_STATE_UNINITIALIZED) {
          return -EALREADY;           /* stats NOT reset per CONVENTIONS §6 */
      }
      STATS_RESET(s_stats);           /* first statement after guard */
      s_config = (cfg != NULL) ? *cfg : ULTRALIGHT_SERVICE_CONFIG_DEFAULT;
      s_variant = s_config.variant;
      s_pages_total = (uint8_t)CONFIG_NFC_ULTRALIGHT_PAGES;
      (void)memset(s_pages, 0, sizeof(s_pages));
      ultralight_write_default_cc(s_pages, s_pages_total, s_variant);
      s_has_version = false;
      s_has_signature = false;
      s_counter_count = 0U;
      s_tearing_flag_count = 0U;
      /* Default pages contain no NDEF TLV — no ndef_service_set_content() call */
      s_state = ULTRALIGHT_SERVICE_STATE_INITIALIZED;
      LOG_INF("ultralight_service: initialized (variant=%u, pages=%u)",
              (unsigned)s_variant, (unsigned)s_pages_total);  /* DEV-ZEP-008 */
      return 0;
  }
  ```

  Implement `ultralight_service_shutdown()` (zero pages, reset state, return 0).
  Implement three getters (§2) per pattern.
  Implement `ultralight_service_get()` (return `&s_vtable`).
  Add `BUILD_ASSERT` from §1.4.

  Run tests → GREEN. `[COMMIT]`

---

- [ ] **Task 6 — `serialize()` (10 min)**

  Implement `ultralight_serialize()` per §1.4 blob format.

  Write tests in `tests/unit/nfc_ultralight/test_ultralight_ser.c`:
  - `test_serialize_min_size`: call with `max < ULTRALIGHT_SERVICE_MAX_SERIALIZED` → `-ENOSPC`.
  - `test_serialize_round_trip_origin`: init with Origin variant, deserialize a known blob, re-serialize → byte-equal.
  - `test_serialize_null_args`: NULL `out` or `out_len` → `-EINVAL`.
  - `test_serialize_uninitialized`: call before `init()` → `-ENODEV`.

  Run → GREEN. `[COMMIT]`

---

- [ ] **Task 7 — `deserialize()` (15 min)**

  Implement `ultralight_deserialize()`:

  ```c
  static int ultralight_deserialize(const uint8_t *in, size_t len, void *user_ctx)
  {
      ARG_UNUSED(user_ctx);

      if (s_state != ULTRALIGHT_SERVICE_STATE_INITIALIZED) {
          return -ENODEV;
      }
      if ((in == NULL) || (len == 0U)) {
          STATS_ERROR(&s_stats_lock, s_stats, -EINVAL);
          return -EINVAL;
      }

      /* Validate format_version */
      size_t offset = 0U;
      if (in[offset] != ULTRALIGHT_BLOB_FORMAT_VERSION) {
          STATS_ERROR(&s_stats_lock, s_stats, -EBADMSG);
          return -EBADMSG;
      }
      offset++;

      /* ... parse variant, pages_total, pages, optional fields per §1.4 ... */
      /* Bounds-check every read before advancing offset */
      /* On truncated blob: STATS_ERROR; return -EBADMSG */

      /* TLV scan on data pages (pages[ULTRALIGHT_DATA_START_PAGE..pages_total-1]) */
      /* Build flat byte view of user-data area */
      /* ... */

      const uint8_t *ndef_msg = NULL;
      size_t ndef_len = 0U;
      int tlv_rc = ultralight_scan_ndef_tlv(data_area, data_area_len,
                                             &ndef_msg, &ndef_len);
      if (tlv_rc == -ENODATA) {
          /* Malformed TLV — graceful per DECISION-UL-4 */
          STATS_INC(&s_stats_lock, s_stats, tlv_parse_error_count);
          /* No NDEF injection */
      } else if ((tlv_rc == 0) && (ndef_msg != NULL)) {
          /* signature locked in wave5-ndef.md DECISION-1 */
          int inject_rc = ndef_service_set_content(ndef_msg, ndef_len);
          if (inject_rc != 0) {
              STATS_INC(&s_stats_lock, s_stats, ndef_inject_error_count);
              STATS_ERROR(&s_stats_lock, s_stats, inject_rc);
              /* Non-fatal: pages loaded, NDEF injection failed */
          } else {
              STATS_INC(&s_stats_lock, s_stats, ndef_inject_count);
          }
      }
      /* else tlv_rc == 0 && ndef_msg == NULL: no NDEF TLV found — acceptable */

      STATS_INC(&s_stats_lock, s_stats, deserialize_count);
      return 0;
  }
  ```

  Write tests:
  - `test_deserialize_valid_ndef`: blob with known NDEF TLV → `ndef_service_set_content` called with correct bytes (use a stub/mock for `ndef_service_set_content` via a test-only function pointer shim or `CONFIG_ZTEST` weak override).
  - `test_deserialize_no_ndef`: valid blob, data pages have only NULL TLVs → no inject, rc=0.
  - `test_deserialize_malformed_tlv`: valid blob, TLV overflow → `tlv_parse_error_count++`, rc=0.
  - `test_deserialize_bad_format_version`: blob byte 0 != 0x01 → `-EBADMSG`.
  - `test_deserialize_truncated_blob`: len shorter than declared pages_total → `-EBADMSG`.
  - `test_deserialize_null_in`: → `-EINVAL`.

  Run → GREEN. `[COMMIT]`

---

- [ ] **Task 8 — Shell commands (10 min)**

  Create `src/nfc/services/ultralight/ultralight_service_shell_cmds.c`:

  ```c
  #include <zephyr/shell/shell.h>
  #include "ultralight_service.h"

  static int cmd_ultralight_service_config(const struct shell *sh,
                                            size_t argc, char **argv)
  {
      ARG_UNUSED(argc); ARG_UNUSED(argv);
      const ultralight_service_config_t *cfg = ultralight_service_get_config();
      shell_print(sh, "variant: %u", (unsigned)cfg->variant);  /* DEV-ZEP-009 */
      shell_print(sh, "pages:   %u", (unsigned)CONFIG_NFC_ULTRALIGHT_PAGES);
      return 0;
  }

  static int cmd_ultralight_service_stats(const struct shell *sh,
                                           size_t argc, char **argv)
  {
      ARG_UNUSED(argc); ARG_UNUSED(argv);
      ultralight_service_stats_t st;
      if (ultralight_service_get_stats(&st) != 0) { return -EINVAL; }
      shell_print(sh, "deserialize_count:      %u", (unsigned)st.deserialize_count);
      shell_print(sh, "serialize_count:        %u", (unsigned)st.serialize_count);
      shell_print(sh, "tlv_parse_error_count:  %u", (unsigned)st.tlv_parse_error_count);
      shell_print(sh, "ndef_inject_count:      %u", (unsigned)st.ndef_inject_count);
      shell_print(sh, "ndef_inject_error_count:%u", (unsigned)st.ndef_inject_error_count);
      shell_print(sh, "error_count:            %u", (unsigned)st.error_count);
      shell_print(sh, "last_error_code:        %d", (int)st.last_error_code);
      return 0;
  }

  static int cmd_ultralight_service_state(const struct shell *sh,
                                           size_t argc, char **argv)
  {
      ARG_UNUSED(argc); ARG_UNUSED(argv);
      shell_print(sh, "state: %u", (unsigned)ultralight_service_get_state());
      return 0;
  }

  SHELL_STATIC_SUBCMD_SET_CREATE(ultralight_service_cmds,
      SHELL_CMD(config, NULL, "Print Ultralight service config",
                cmd_ultralight_service_config),
      SHELL_CMD(stats,  NULL, "Print Ultralight service stats",
                cmd_ultralight_service_stats),
      SHELL_CMD(state,  NULL, "Print Ultralight service state",
                cmd_ultralight_service_state),
      SHELL_SUBCMD_SET_END);
  SHELL_CMD_REGISTER(nfc_ultralight, &ultralight_service_cmds,
                     "NFC Ultralight service control", NULL);
  ```

  `[COMMIT]`

---

- [ ] **Task 9 — `tests/` CMakeLists and prj.conf (5 min)**

  Create `tests/unit/nfc_ultralight/CMakeLists.txt`:

  ```cmake
  cmake_minimum_required(VERSION 3.20.0)
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(test_ultralight_service)

  target_sources(app PRIVATE
      test_ultralight_tlv.c
      test_ultralight_ser.c
      # Include the service under test directly (not via library) for white-box access
      ${CMAKE_SOURCE_DIR}/src/nfc/services/ultralight/ultralight_service.c
  )
  target_include_directories(app PRIVATE
      ${CMAKE_SOURCE_DIR}/src
  )
  ```

  Create `tests/unit/nfc_ultralight/prj.conf`:
  ```
  CONFIG_ZTEST=y
  CONFIG_NFC_ULTRALIGHT_PAGES=41
  CONFIG_NFC_SERVICE_ULTRALIGHT=y
  # Stub ndef_service_set_content via weak override in test files
  ```

  Run `west twister -T tests/unit/nfc_ultralight --platform qemu_cortex_m33 --no-sysbuild`. All tests GREEN. (`native_sim` is Linux-CI-only; use `qemu_cortex_m33` or DK locally.) `[COMMIT]`

---

- [ ] **Task 10 — Integration cross-check with wave4 (5 min)**

  Verify `nfc_stack.c` (wave4) compiles with:
  - `#include "services/ultralight/ultralight_service.h"`
  - `ultralight_service_init(NULL)` at init step 14.
  - `ultralight_service_shutdown()` at shutdown step 7.
  - `ultralight_service_get()` in the ULTRALIGHT profile svcs array.

  Grep for all `TBC Wave 5` markers in wave4-stack.md concerning ultralight and verify each is resolved:
  - `ultralight_service_init` ✓ (this plan)
  - `ultralight_service_shutdown` ✓ (this plan)
  - `ultralight_service_get` ✓ (this plan)
  - `ndef_service_get()` for ULTRALIGHT profile AID registration: **unchanged** (wave4 DECISION-6 locked; not this service's concern)
  - Save/load array for ULTRALIGHT profile: resolved by DECISION-UL-9 (`{ultralight_service_get()}`, n=1)

  Verify `ReadLints` shows no issues in new files. `[COMMIT]`

---

## DECISION Summary

| ID | Description | §Ref |
|----|-------------|------|
| **DECISION-UL-1** | Dispatch callbacks are defensive no-op stubs, not NULL — wave3 service.h forbids NULL core callbacks, and stubs keep that contract intact even though this service is never router-registered. The on_apdu stub counts the misdispatch and responds 6D00. | §4.1 |
| **DECISION-UL-2** | Emulated variant: EV1 MF0UL21 (41 pages) by default; Kconfig-tunable `CONFIG_NFC_ULTRALIGHT_PAGES` (range 16..235). Chosen for NDEF capacity, reader compatibility, EV1 version/counter fields. | §1.2 |
| **DECISION-UL-3** | No resync at serialize time: serialize writes `s_pages[]` verbatim; reader-written NDEF updates in the NDEF service are NOT back-propagated to ultralight page model. | §6.1 |
| **DECISION-UL-4** | Malformed TLV = graceful: blob-level corruption → `-EBADMSG`; TLV-level malformation after successful blob parse → `tlv_parse_error_count++`, no NDEF injection, return 0. | §3.5 |
| **DECISION-UL-5** | Push model: ultralight includes `ndef_service.h` and calls `ndef_service_set_content()` directly. One-directional coupling; NDEF does not include ultralight. Overrides spec v2 §6.4.3 pull model. | §5.1 |
| **DECISION-UL-6** | `ULTRALIGHT_SERVICE_MAX_SERIALIZED = 256U`; covers UL21 with all optional fields (actual max 220 bytes). Guarded by `BUILD_ASSERT`. | §1.4 |
| **DECISION-UL-6b** | CC2 byte for UL21 default = `0x10U` (128 bytes, conservative); CC bytes from a real card blob are preserved verbatim on deserialize. | §6.2 |
| **DECISION-UL-7** | Serialized blob format: version byte + variant + pages_total + raw pages + optional version/signature/counters/tearing flags, each guarded by a `has_*` byte. Service-owned format, outer TLV framed by the store using `persist_id = 0x03`. | §1.4 |
| **DECISION-UL-8** | On bare `init()`: zeroed pages, CC written for configured variant, no `ndef_service_set_content()` call (no NDEF in default pages). Real card data comes via `deserialize()`. | §3.1 |
| **DECISION-UL-9** | Save/load array for `NFC_PROFILE_ULTRALIGHT` contains only the ultralight service (n=1). The NDEF service is not separately persisted for this profile; ultralight's `deserialize()` drives the NDEF inject. | §5.2 |
| **DECISION-6** (wave4, confirmed) | `NFC_PROFILE_ULTRALIGHT` registers the NDEF AID using `ndef_service_get()`. Ultralight never calls `aid_router_register()`. This plan does not change wave4's locked `stack_register_profile()`. | §Arch |
