# Wave 5d: EMV Service Implementation Plan

**Layer:** `services/emv`
**Files produced:**
- `src/nfc/services/emv/emv_service.h`
- `src/nfc/services/emv/emv_service.c`
- `src/nfc/services/emv/emv_service_shell_cmds.c`
- `src/nfc/services/emv/CMakeLists.txt`
- `src/nfc/services/emv/Kconfig`
- `tests/unit/nfc_emv/` (ztest — pure TLV helpers, response builders; Twister tag `sigmation.nfc.emv`; `native_sim` is Linux-CI-only — use `qemu_cortex_m33` or DK locally)

**Depends on (all LOCKED 2026-06-12):**
- `docs/superpowers/plans/wave1-hal.md` — `nfc_transport_send_response` contract, `@threadsafe`
- `docs/superpowers/plans/wave2-framing.md` — `nfc_apdu_t`, `apdu_types.h` SW vocabulary
- `docs/superpowers/plans/wave3-router.md` — `nfc_service_t` vtable (verbatim + persistence), persist-ID table, routing rules
- `docs/superpowers/plans/wave4-stack.md` — profile enum, `NFC_PROFILE_EMV` registration (one AID only — PPSE), `emv_service_get()` surface

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 lifecycle/state, §3 coupling, §4 callbacks, §5 buffer model, §6 stats recipe, §7 threading, §8 profile switch, §9 errno, §10 shell, §11 MISRA, §12 checklist)
2. `docs/superpowers/plans/wave3-router.md` §1 — `nfc_service_t` vtable VERBATIM; persist-ID table (EMV = `0x04`)
3. `docs/superpowers/plans/wave4-stack.md` §1.5, §5.2 — `NFC_PROFILE_EMV` registers **one AID only** (`k_emv_aid` = PPSE 14 bytes); expected surface `emv_service_init/shutdown/get` confirmed; routing gap analysed — see DECISION-1
4. `docs/nfc/archive/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) §6.4.4, §6.5, §13 — scope, commands, store seam, non-goals
5. `docs/superpowers/plans/wave1-hal.md` §2 — `nfc_transport_send_response(buf, len)`: borrows `buf` until next callback; `@threadsafe`; returns 0 / `-EINVAL` / `-ENODEV` / `-EIO`; cast return to `(void)` per MISRA Rule 17.7 at this layer
6. `docs/superpowers/plans/wave2-framing.md` §1 — `nfc_apdu_t` fields (`cla`, `ins`, `p1`, `p2`, `data`, `lc`, `le`, `has_le`, `extended`); SW constants in `apdu_types.h`; `NFC_SW_OK 0x9000`, `NFC_SW_INS_NOT_SUPPORTED 0x6D00`, `NFC_SW_FILE_NOT_FOUND 0x6A82`, `NFC_SW_CONDITIONS_NOT_SATISFIED 0x6985`, `NFC_SW1`/`NFC_SW2` macros
7. `flipperzero/applications/main/nfc/helpers/nfc_emv_parser.c/.h` — Flipper reader-side AID/country/currency name lookup using `Storage*`, `FuriString*`, `flipper_format`; **non-portable to Zephyr** — see DECISION-2; no EMV protocol directory found at `flipperzero/lib/nfc/protocols/emv/` (does not exist in this repo)
8. `src/stats.h` — `STATS_RESET`, `STATS_INC`, `STATS_ERROR`, `STATS_COPY_OUT`, `STATS_SCOPE` macros
9. `~/.claude/rules/misra-coding-standards-misra-c-2012.md` and `misra-coding-standards-zephyr-misra-deviations.md` — MISRA C:2012 + DEV-ZEP-001 through DEV-ZEP-018
10. EMV Contactless Specifications (Book B, Annex B/D) — PPSE FCI TLV structure, App FCI, GPO Format 1, READ RECORD tag set; expressed as original prose, not copied

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h`.

---

> **Architecture Framing — spec v3 (2026-06-12):** This service is the
> **NFCT/card-role first-slice** of the **EMV** protocol module as defined by the
> [NFC Stack Architecture v3](../specs/2026-06-12-nfc-stack-architecture.md) (§4.3).
> A protocol module owns: data model (`emv_card_image_t`) · (de)serialize ·
> **listener** (this file, built under `CONFIG_NFC_ROLE_CARD`) · **poller**
> (reader role — RESERVED, not implemented in this slice).
> **Lane:** `NFC_LANE_ISO_DEP` — Type-4, dispatched via APDU framing (Wave 2) + AID
> router (Wave 3). **Kconfig:** `NFC_SERVICE_EMV` enables this protocol module; the
> listener compiles under `CONFIG_NFC_ROLE_CARD` (orthogonal, Wave 1). Existing symbol
> names are unchanged. **Serialize completeness:** output classified
> `NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED` —
> the full card image (PAN, track2, records) is available for replay; a future poller
> blob would carry `NFC_STORE_ENTRY_FLAG_READER_CAPTURED`
> (wave6 canonical C symbols — wave6-store.md §1.7 / spec v3 §4.5).

---

## DECISION Log

> **DECISION-1: Payment application AID routing — Resolved by Wave 4.**
>
> Wave 4 §4.5 (`nfc_stack_register_profile_aids()`) registers **both** AIDs for
> `NFC_PROFILE_EMV`: the 14-byte PPSE `32 50 41 59 2E 53 59 53 2E 44 44 46 30 31`
> **and** the payment application AID via a second
> `aid_router_register(emv_service_get_app_aid(), emv_service_get_app_aid_len(), emv_service_get())`
> call. Tests expect exactly 2 registrations in the EMV profile.
>
> The card image `app_aid` field must equal `k_emv_app_aid`. `deserialize()` returns
> `-EBADMSG` if the stored `app_aid_len`/`app_aid` does not match the compile-time
> constant (a mismatch would cause a PPSE FCI that lists an AID the router doesn't
> serve). Future extension: dynamic re-registration on AID change is out of scope.

---

> **DECISION-2: No Flipper reuse — purpose-built TLV helpers only.**
>
> `flipperzero/applications/main/nfc/helpers/nfc_emv_parser.c` is a
> **reader-side** Flipper OS utility that looks up AID/country/currency names
> from SD card files. It uses `Storage*`, `FuriString*`, and `flipper_format` —
> all Flipper OS primitives with no Zephyr equivalent. It cannot be compiled into
> NCS/Zephyr and serves an entirely different purpose from card emulation.
> The Flipper EMV protocol directory cited in the planning instructions
> (`flipperzero/lib/nfc/protocols/emv/`) does not exist in this repository.
>
> **Resolution:** All TLV construction uses a purpose-built, file-private
> `tlv_writer_t` helper (§1.3) with per-byte bounds checking. No `sprintf`,
> `strlcpy`, or dynamic allocation. All tag writes are explicit; constructing
> tags are wrapped using a size-aware open/patch/close idiom.

---

> **DECISION-3: No cryptogram generation — protocol-walk emulation only.**
>
> AIP = `0x00 0x00`: no SDA, no DDA, no CDA, no cardholder verification, no
> offline auth. GPO returns a static Format-1 response (tag `80`; no `77` template
> required). No ARQC, no GENERATE AC, no dynamic SDA records, no ICC public key.
> This implementation is sufficient to walk a reader/phone through the full
> PPSE → app SELECT → GPO → READ RECORD exchange and present a static card image.
> **It is not a payment instrument and must not be used with live payment systems.**
> A production EMV implementation requires SDA/DDA/CDA material, offline auth
> records, and ICC key infrastructure; that is explicitly out of scope.

---

> **DECISION-4: TLV responses pre-built into static caches at init/deserialize.**
>
> PPSE FCI, App FCI, and GPO response blobs are built once into module-static
> cache buffers (`s_ppse_fci_buf`, `s_app_fci_buf`, `s_gpo_buf`) by
> `emv_rebuild_caches(const emv_card_image_t *)` called from both `init()` and
> the `deserialize()` hook. Records are similarly pre-built into
> `s_card_image.record_data[]` at init. Dispatch path is then a bounded
> `memcpy + SW append + send_response` — no TLV arithmetic on `nfc_work_q`.
> `emv_rebuild_caches()` returns `-ENOSPC` if any pre-built blob exceeds its cache
> buffer; `init()` treats this as `-EIO` and fails.

---

> **DECISION-5: Session state as a plain enum — no SMF.**
>
> CONVENTIONS §2 locks `services/emv` as Pattern A / minimal lifecycle with
> no behavioral FSM. The EMV flow has four states (`IDLE`, `PPSE_SELECTED`,
> `APP_SELECTED`, `GPO_DONE`) implemented as a plain `emv_session_state_t`
> enum (file-static, `nfc_work_q`-only access). No `smf.h` dependency.

---

> **DECISION-6: GPO PDOL data ignored — static response always returned.**
>
> The GPO command (CLA=`80` INS=`A8`) may carry a PDOL-derived data object
> (terminal capabilities, amount, etc.) in its data field. We accept any GPO
> command regardless of `lc` / data content and return the pre-built static
> Format-1 response. Rationale: static card emulation has no need for terminal
> data; the response is entirely pre-computed from the card image's AIP and AFL.

---

> **DECISION-7: READ RECORD routing and error codes.**
>
> `P2 = (SFI << 3) | 0x04` per EMV Book 1 §11.3.5; `P1` = 1-based record number.
> The AFL entry in the card image encodes the SFI and record range. The service
> extracts `sfi = (apdu->p2 >> 3U) & 0x1FU` and `rec_no = apdu->p1`.
> Response rules:
> - `P2 & 0x07 != 4`: invalid P2 format → `6B00` (Wrong Parameters P1/P2).
> - SFI ≠ AFL SFI: `6A82` (File Not Found).
> - `rec_no < 1` or `rec_no > record_count`: `6A83` (Record Not Found).
>   `NFC_SW_RECORD_NOT_FOUND 0x6A83U` is part of the `apdu_types.h` vocabulary
>   (wave2-framing §1.1).
> - Valid record: return `record_data[rec_no - 1U]` (pre-built tag-70 TLV) + `9000`.

---

> **DECISION-8: Test card image — clearly fake, static data.**
>
> Default card image baked into `emv_service.c` via a file-static constant
> `k_emv_default_card`. PAN is `9999 9999 9999 9999` (sixteen 9s — not a valid
> ISO/IEC 7812 PAN and will never pass a Luhn check; clearly a test card).
> Expiry `99/12` (December 2099). Name `TEST/CARD`. App AID: Visa test RID
> `A0 00 00 00 03 10 10` (7 bytes). ATC `0001`. No CVV2/iCVV.
> The default image is loaded via `emv_service_init()` before a store load; a
> store `nfc_stack_load()` call will overwrite it via `deserialize()`.

---

## 1. Types and Constants

All public types in `emv_service.h`. Internal types in `emv_service.c`.

### 1.1 Size Constants and EMV Tags (public — `emv_service.h`)

```c
/* ── Wire constants ─────────────────────────────────────────────────────────── */

/** PPSE DF Name: "2PAY.SYS.DDF01" (14 bytes) */
#define EMV_PPSE_AID     \
    { 0x32U,0x50U,0x41U,0x59U,0x2EU,0x53U,0x59U,0x53U, \
      0x2EU,0x44U,0x44U,0x46U,0x30U,0x31U }
#define EMV_PPSE_AID_LEN  14U

/**
 * Test payment-application AID (Visa test RID A0 00 00 00 03, application 10 10).
 * Exported so Wave 4 can register a second router entry — see DECISION-1.
 * Card image app_aid MUST equal this; deserialize() rejects mismatches.
 */
#define EMV_SERVICE_APP_AID      \
    { 0xA0U,0x00U,0x00U,0x00U,0x03U,0x10U,0x10U }
#define EMV_SERVICE_APP_AID_LEN  7U

/* ── Card-image sizing ──────────────────────────────────────────────────────── */
#define EMV_PAN_BYTES           8U   /**< BCD PAN: ≤16 digits, right-nibble-padded 0xF */
#define EMV_EXPIRY_BYTES        3U   /**< BCD YYMMDD (Year-Month-Day) */
#define EMV_NAME_BYTES         26U   /**< ISO/IEC 7813 cardholder name, space-padded */
#define EMV_TRACK2_MAX_BYTES   19U   /**< Track 2 equivalent data: ≤37 nibbles packed */
#define EMV_AIP_BYTES           2U   /**< Application Interchange Profile */
#define EMV_AFL_BYTES           4U   /**< Application File Locator (one SFI entry) */
#define EMV_APP_AID_MAX_BYTES  16U   /**< Max AID length per ISO 7816-4 §8.2 */

/**
 * Maximum serialized size in bytes (version byte + fixed fields + record blobs).
 * Formula: 84 + CONFIG_NFC_EMV_MAX_RECORDS * (1 + CONFIG_NFC_EMV_RECORD_SIZE).
 * At Kconfig defaults (2 records × 64 bytes each): 84 + 130 = 214 bytes.
 * A BUILD_ASSERT in emv_service.c checks this fits within the store's buffer.
 */
#define EMV_SERVICE_MAX_SERIALIZED \
    ((size_t)(84U) + \
     (size_t)(CONFIG_NFC_EMV_MAX_RECORDS) * (1U + (size_t)(CONFIG_NFC_EMV_RECORD_SIZE)))

/** Response buffer — large enough for any single response blob + 2-byte SW. */
#define EMV_RESP_BUF_SIZE  256U

/* NFC_SW_RECORD_NOT_FOUND (0x6A83) and NFC_SW_WRONG_P1P2 (0x6B00) come from
 * apdu_types.h (wave2-framing §1.1) — no local SW definitions in this service. */

/* ── TLV tag constants (wire values) ───────────────────────────────────────── */
#define EMV_TAG_FCI_TEMPLATE          0x6FU
#define EMV_TAG_DF_NAME               0x84U
#define EMV_TAG_FCI_PROP              0xA5U   /* constructed */
#define EMV_TAG_FCI_ISSUER_DISC_HIGH  0xBFU   /* first byte of 2-byte tag BF0C */
#define EMV_TAG_FCI_ISSUER_DISC_LOW   0x0CU   /* second byte */
#define EMV_TAG_DIR_ENTRY             0x61U   /* constructed */
#define EMV_TAG_AID                   0x4FU
#define EMV_TAG_APP_LABEL             0x50U
#define EMV_TAG_APP_PRIORITY          0x87U
#define EMV_TAG_RESP_TMPL_FMT1        0x80U   /* GPO Format 1 */
#define EMV_TAG_RECORD_TMPL           0x70U   /* constructed */
#define EMV_TAG_TRACK2                0x57U
#define EMV_TAG_PAN_HIGH              0x5AU   /* 1-byte tag */
#define EMV_TAG_EXPIRY_HIGH           0x5FU   /* first byte of 2-byte tags 5F20, 5F24 */
#define EMV_TAG_EXPIRY_LOW            0x24U   /* second byte of 5F24 */
#define EMV_TAG_NAME_LOW              0x20U   /* second byte of 5F20 */
#define EMV_TAG_ATC_HIGH              0x9FU   /* first byte of 9F36 */
#define EMV_TAG_ATC_LOW               0x36U   /* second byte */
```

### 1.2 Card Image (public — `emv_service.h`)

```c
/**
 * @brief Static EMV card image — the full data model for one emulated card.
 *
 * All fields are fixed-width arrays suitable for memcpy serialization.
 * Records are pre-built TLV blobs (tag-70 template) written at init or
 * after deserialize (DECISION-4). The card image is never modified while
 * the stack is STARTED (store -EBUSY gating — CONVENTIONS §3 / spec §6.5).
 */
typedef struct {
    uint8_t  pan[EMV_PAN_BYTES];            /**< BCD PAN, right-nibble-padded 0xFF */
    uint8_t  pan_len;                       /**< Digit count (≤16) */
    uint8_t  expiry[EMV_EXPIRY_BYTES];      /**< BCD YYMMDD */
    uint8_t  name[EMV_NAME_BYTES];          /**< Cardholder name, space-padded */
    uint8_t  track2[EMV_TRACK2_MAX_BYTES];  /**< Track 2 equivalent, nibble-padded 0xF */
    uint8_t  track2_len;                    /**< Byte count of track2 */
    uint8_t  aip[EMV_AIP_BYTES];            /**< AIP: 0x0000 = no SDA/DDA/CDA (DECISION-3) */
    uint8_t  afl[EMV_AFL_BYTES];            /**< AFL: one SFI entry [SFI<<3|4, first, last, 0] */
    uint8_t  app_aid[EMV_APP_AID_MAX_BYTES];/**< Payment app AID (must equal EMV_SERVICE_APP_AID) */
    uint8_t  app_aid_len;                   /**< Byte length of app_aid */
    uint8_t  record_count;                  /**< Number of valid records (≤CONFIG_NFC_EMV_MAX_RECORDS) */
    /**
     * Pre-built record TLV blobs: record_data[i] is a complete tag-70 TLV
     * (length given by record_len[i]), ready to transmit + 9000.
     * Built at init or after deserialize by emv_rebuild_caches().
     * Array dimensions are compile-time Kconfig constants.
     */
    uint8_t  record_data[CONFIG_NFC_EMV_MAX_RECORDS][CONFIG_NFC_EMV_RECORD_SIZE];
    uint8_t  record_len[CONFIG_NFC_EMV_MAX_RECORDS];
} emv_card_image_t;
```

### 1.3 Module Contract Types (public — `emv_service.h`)

```c
/** @brief Minimal-lifecycle config struct (frozen after init). */
typedef struct {
    /* No runtime-tunable fields — card image is the data model, not config.
     * Struct exists to satisfy the per-module contract (CONVENTIONS §2). */
    uint8_t _reserved;
} emv_service_config_t;

/** @brief Stats struct — all counters are uint32_t, lock-guarded. */
typedef struct {
    uint32_t select_ppse_count;    /**< PPSE SELECT received and served */
    uint32_t select_app_count;     /**< App AID SELECT received and served */
    uint32_t gpo_count;            /**< GPO commands served */
    uint32_t read_record_count;    /**< READ RECORD commands served */
    uint32_t apdu_unknown_count;   /**< APDUs returning 6D00 */
    uint32_t apdu_bad_state_count; /**< APDUs returning 6985 (wrong session state) */
    uint32_t error_count;          /**< Mandatory (CONVENTIONS §6) */
    int32_t  last_error_code;      /**< Mandatory (CONVENTIONS §6) */
} emv_service_stats_t;

/** @brief Minimal lifecycle state — Pattern A. */
typedef enum {
    EMV_SERVICE_STATE_UNINITIALIZED = 0,
    EMV_SERVICE_STATE_INITIALIZED,
} emv_service_state_t;
```

### 1.4 Session State (internal — `emv_service.c`)

```c
/** Per-session EMV state — resets on deselect/field-off. */
typedef enum {
    EMV_SESSION_IDLE = 0,         /**< No application selected */
    EMV_SESSION_PPSE_SELECTED,    /**< PPSE FCI sent; awaiting app SELECT */
    EMV_SESSION_APP_SELECTED,     /**< App FCI sent; awaiting GPO */
    EMV_SESSION_GPO_DONE,         /**< GPO served; awaiting READ RECORD(s) */
} emv_session_state_t;
```

### 1.5 TLV Writer (internal — `emv_service.c`)

```c
/**
 * Minimal bounds-checked TLV serializer. All writes are explicit byte-by-byte
 * with overflow tracked in the error flag — no sprintf, no unbounded copies.
 * All tags are 1 or 2 bytes; all lengths are 1 or 2 bytes (short / 0x81 form).
 * Writer is purely stack-resident; no allocation (MISRA Rule 11.5 / creed §9).
 */
typedef struct {
    uint8_t *buf;    /**< Output buffer */
    size_t   cap;    /**< Buffer capacity in bytes */
    size_t   pos;    /**< Current write position */
    bool     error;  /**< Set on first overflow; all subsequent writes are no-ops */
} tlv_writer_t;

/* Inline statics — emv_service.c internal only, never exposed in the header. */
static void    tlv_init(tlv_writer_t *w, uint8_t *buf, size_t cap);
static void    tlv_put_byte(tlv_writer_t *w, uint8_t b);
static void    tlv_put_bytes(tlv_writer_t *w, const uint8_t *data, size_t len);
/**
 * Write a primitive TLV element with a 1- or 2-byte tag.
 * @param tag  16-bit value; single-byte tags have high byte 0x00.
 *             Two-byte tags: e.g., 0xBF0C → writes 0xBF then 0x0C.
 */
static void    tlv_put_primitive(tlv_writer_t *w, uint16_t tag,
                                  const uint8_t *val, size_t vlen);
/**
 * Open a constructed TLV element. Returns the position of the length byte(s)
 * placeholder (to be patched by tlv_close_constructed). Caller writes body,
 * then calls tlv_close_constructed.
 * Single-byte length placeholder written as 0x00 if body ≤ 127 expected;
 * if body overflows, the writer error flag fires.
 */
static size_t  tlv_open_constructed(tlv_writer_t *w, uint16_t tag);
/**
 * Patch the length placeholder at @p len_pos with the actual body length
 * (pos - body_start). Uses short-form if body ≤ 127, 0x81 form otherwise.
 * If 0x81 form is needed but only 1 byte was reserved, sets writer error.
 */
static void    tlv_close_constructed(tlv_writer_t *w, size_t len_pos);
/** @retval true if any write has overflowed the buffer. */
static bool    tlv_error(const tlv_writer_t *w);
/** @return bytes written so far (valid only if !tlv_error). */
static size_t  tlv_len(const tlv_writer_t *w);
```

> **Implementation note:** `tlv_open_constructed` always reserves 2 bytes for the
> length field (tag byte + `0x81` form support). `tlv_close_constructed` patches
> in 1-byte form (≤127) or 2-byte `0x81 <len>` form. For all blobs in this service
> the body never exceeds 127 bytes (PPSE FCI ≈ 50 bytes, App FCI ≈ 45 bytes),
> so 1-byte lengths always suffice; the 2-byte reserve is a defensive allowance.

---

## 2. Public API

All declarations in `emv_service.h`.

```c
/* ── Mandatory Wave 4 surface (CONVENTIONS §2; wave4-stack.md §5.2) ────────── */

/**
 * @brief Initialize the EMV service.
 *
 * Loads the default card image (DECISION-8), pre-builds TLV caches
 * (DECISION-4), and registers the vtable. Sets state to INITIALIZED.
 * STATS_RESET is the first statement after the -EALREADY guard (CONVENTIONS §6).
 *
 * @param cfg  Configuration; NULL → defaults (emv_service_config_t has no
 *             runtime-tunable fields; this param satisfies the per-module
 *             contract from CONVENTIONS §2 / wave4 §5.2).
 * @retval  0         UNINITIALIZED → INITIALIZED.
 * @retval -EALREADY  Already INITIALIZED; stats not reset.
 * @retval -EIO       TLV cache build failed (buffer overflow in emv_rebuild_caches).
 * @caller_sync
 */
int emv_service_init(const emv_service_config_t *cfg);

/**
 * @brief Shut down the EMV service. Idempotent from any state.
 *
 * Clears session state and resets to UNINITIALIZED. Always returns 0.
 *
 * @retval 0
 * @caller_sync
 */
int emv_service_shutdown(void);

/**
 * @brief Return pointer to the file-static nfc_service_t vtable.
 *
 * Safe to call before init(). Returns a non-NULL compile-time constant.
 * Wave 4 calls this during profile registration.
 *
 * @retval Non-NULL always.
 * @isr_safe
 */
const nfc_service_t *emv_service_get(void);

/* ── Module contract getters (CONVENTIONS §2) ──────────────────────────────── */

/**
 * @retval Non-NULL always (file-static; safe before init).
 * @isr_safe
 */
const emv_service_config_t *emv_service_get_config(void);

/**
 * @brief Copy-out stats under spinlock.
 * @retval  0       Success; *out populated.
 * @retval -EINVAL  out is NULL.
 * @isr_safe
 */
int emv_service_get_stats(emv_service_stats_t *out);

/**
 * @retval Current lifecycle state; never fails.
 * @isr_safe
 */
emv_service_state_t emv_service_get_state(void);

/* ── Wave 4 amendment helpers — DECISION-1 ─────────────────────────────────── */

/**
 * @brief Return pointer to the static payment-app AID array.
 *
 * Wave 4 NFC_PROFILE_EMV case must call aid_router_register with this AID
 * and emv_service_get() to route app SELECTs to the service — see DECISION-1.
 *
 * @retval Pointer to k_emv_app_aid[EMV_SERVICE_APP_AID_LEN]; never NULL.
 * @isr_safe
 */
const uint8_t *emv_service_get_app_aid(void);

/**
 * @brief Return the byte length of the payment-app AID.
 * @retval EMV_SERVICE_APP_AID_LEN (compile-time constant).
 * @isr_safe
 */
size_t emv_service_get_app_aid_len(void);
```

---

## 3. Contracts

### `emv_service_init()`
- **Pre:** module in any state; `cfg == NULL` → defaults (struct has no runtime-tunable fields).
- **Post:** state == INITIALIZED; `s_card_image` populated from `k_emv_default_card`;
  `s_ppse_fci_buf`, `s_app_fci_buf`, `s_gpo_buf`, all record blobs pre-built.
- **Errors:** `-EALREADY` if already INITIALIZED (stats NOT reset); `-EIO` if
  `emv_rebuild_caches()` returns non-zero (buffer overflow on any pre-built blob).

### `emv_service_shutdown()`
- **Pre:** any state.
- **Post:** `s_session_state = EMV_SESSION_IDLE`; `s_state = UNINITIALIZED`.
- **Errors:** none (always 0; idempotent).

### `emv_service_get()`
- **Pre:** none (safe before init).
- **Post:** returns `&s_vtable` (compile-time constant).
- **Errors:** none.

### `emv_service_get_stats()`
- **Pre:** `out != NULL`.
- **Post:** `*out` is a coherent snapshot under spinlock.
- **Errors:** `-EINVAL` if `out == NULL`.

### vtable → `on_select(aid, aid_len, user_ctx)`
- **Thread:** `nfc_work_q` exclusively.
- **Pre:** none (defensive: if state == UNINITIALIZED, fall through to 6D00 path).
- **Post:** `nfc_transport_send_response()` called exactly once before returning.
- **Dispatch:**
  - `aid_len == EMV_PPSE_AID_LEN` AND `memcmp(aid, k_emv_ppse_aid, EMV_PPSE_AID_LEN) == 0`:
    → `s_session_state = EMV_SESSION_PPSE_SELECTED`; send `s_ppse_fci_buf + 9000`.
  - `aid_len == EMV_SERVICE_APP_AID_LEN` AND `memcmp(aid, k_emv_app_aid, EMV_SERVICE_APP_AID_LEN) == 0`:
    → `s_session_state = EMV_SESSION_APP_SELECTED`; send `s_app_fci_buf + 9000`.
  - Any other AID (should not occur if router is correctly configured):
    → `s_session_state = EMV_SESSION_IDLE`; send `NFC_SW_FILE_NOT_FOUND (6A82)`;
    `STATS_ERROR`. (Defensive path per DECISION-1 until Wave 4 amendment lands.)

### vtable → `on_apdu(apdu, user_ctx)`
- **Thread:** `nfc_work_q` exclusively.
- **Pre:** `apdu != NULL`; `apdu->data` valid for duration of call.
- **Post:** `nfc_transport_send_response()` called exactly once.
- **Dispatch:** See §5 APDU Decision Table.

### vtable → `on_deselect(user_ctx)` / `on_field_off(user_ctx)`
- **Thread:** `nfc_work_q` exclusively.
- **Post:** `s_session_state = EMV_SESSION_IDLE`. No response sent.

### vtable → `serialize(out, max, out_len, user_ctx)` — `@caller_sync`
- **Pre:** `out != NULL`, `out_len != NULL`.
- **Post:** `*out_len` bytes of serialized card image written to `out`.
- **Errors:** `-EINVAL` if NULL args; `-ENOSPC` if `max < EMV_SERVICE_MAX_SERIALIZED`.

### vtable → `deserialize(in, len, user_ctx)` — `@caller_sync`
- **Pre:** `in != NULL`, `len > 0`.
- **Post:** `s_card_image` updated; `emv_rebuild_caches()` called to refresh response blobs.
- **Errors:** `-EINVAL` if NULL/zero; `-EBADMSG` if version mismatch, length too short,
  or `in.app_aid` ≠ `k_emv_app_aid` (DECISION-1); `-EIO` if cache rebuild fails.

---

## 4. Internal State

All state is file-static in `emv_service.c`. Never dynamically allocated.

```c
/* ── Lifecycle ───────────────────────────────────────────────────────────────── */
static emv_service_state_t   s_state;         /* Pattern A; @caller_sync lifecycle */
static emv_service_config_t  s_config;        /* frozen after init; no tunables     */

/* ── Session ─────────────────────────────────────────────────────────────────── */
static emv_session_state_t   s_session_state; /* nfc_work_q only; no lock needed    */

/* ── Card model ──────────────────────────────────────────────────────────────── */
static emv_card_image_t      s_card_image;    /* nfc_work_q during session; @caller_sync via serialize/deserialize (stack quiesced per §6.5) */

/* ── Response caches (pre-built TLV blobs, without trailing SW bytes) ─────────── */
static uint8_t  s_ppse_fci_buf[128U]; /* PPSE FCI blob; ~50 bytes, 128 reserve     */
static size_t   s_ppse_fci_len;
static uint8_t  s_app_fci_buf[128U];  /* App FCI blob; ~45 bytes, 128 reserve       */
static size_t   s_app_fci_len;
static uint8_t  s_gpo_buf[16U];       /* GPO Format-1 blob: 80 06 [AIP:2][AFL:4]    */
static size_t   s_gpo_len;

/* ── Outbound response buffer ─────────────────────────────────────────────────── */
/* File-static (not a static local — MISRA Dir 4.9 / creed §9). Borrowed by HAL
 * until next callback. Must not be overwritten before next on_select/on_apdu. */
static uint8_t  s_resp_buf[EMV_RESP_BUF_SIZE];

/* ── Statistics ───────────────────────────────────────────────────────────────── */
static emv_service_stats_t  s_stats;          /* spinlock-guarded                   */
static struct k_spinlock     s_stats_lock;

/* ── Vtable (compile-time constant — function pointers fixed at link time) ────── */
static const nfc_service_t  s_vtable;         /* initialiser in §5 Internal State   */
```

### Static AID Constants

```c
/* k_emv_ppse_aid and k_emv_app_aid are file-static const arrays, declared at
 * file scope per MISRA Dir 4.9 / creed §9 (no static locals). */
static const uint8_t k_emv_ppse_aid[]    = EMV_PPSE_AID;
static const uint8_t k_emv_app_aid[]     = EMV_SERVICE_APP_AID;
static const uint8_t k_emv_app_aid_label[] = "VISA DEBIT      "; /* 16 bytes, space-padded */
```

> **Note:** FCI builders (`emv_build_ppse_fci`, `emv_build_app_fci`) write the
> **unpadded** label length into the TLV `50` tag — exactly as shown in the §6.2/§6.3
> wire traces ("VISA DEBIT" = 10 bytes; "TEST CARD" = 9 bytes). The trailing space
> padding in `k_emv_app_aid_label` is **not** emitted; the builder uses `strlen()` or
> an explicit unpadded count, not `sizeof(k_emv_app_aid_label)`.

### Default Card Image Constant

```c
/* k_emv_default_card is file-static const, used to initialise s_card_image at
 * init(). Values are clearly fake test data (DECISION-8). */
static const emv_card_image_t k_emv_default_card = {
    .pan         = { 0x99U,0x99U,0x99U,0x99U,0x99U,0x99U,0x99U,0x99U }, /* 9999999999999999 */
    .pan_len     = 16U,
    .expiry      = { 0x99U, 0x12U, 0x31U },  /* YYMMDD = 9912 31 (Dec 2099) */
    .name        = { 'T','E','S','T','/',' ','C','A','R','D',' ',' ',' ',' ',' ',' ',
                     ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' }, /* 26 bytes, space-padded */
    .track2      = { 0x99U,0x99U,0x99U,0x99U,0x99U,0x99U,0x99U,0x99U,
                     0xD9U,0x91U,0x21U,0x10U,0x10U,0x00U,0x00U,0x0FU }, /* 16 bytes */
    .track2_len  = 16U,
    .aip         = { 0x00U, 0x00U },  /* No SDA/DDA/CDA (DECISION-3) */
    .afl         = { 0x0CU, 0x01U, 0x01U, 0x00U }, /* SFI=1 (0x0C = 1<<3|4), rec 1..1, 0 offline */
    .app_aid     = { 0xA0U,0x00U,0x00U,0x00U,0x03U,0x10U,0x10U,
                     0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U },
    .app_aid_len = 7U,
    .record_count = 1U,
    /* record_data[] is populated by emv_rebuild_caches(), not baked into this const */
    .record_data  = { { 0U } },
    .record_len   = { 0U },
};
```

> **Note:** `record_data[]` in the default card constant is zero-initialised; it is
> filled by `emv_build_default_record()` inside `emv_rebuild_caches()` on first call
> from `init()`. This avoids encoding a large pre-assembled TLV blob as a `const`.

### Session State Machine

```
                  on_select(PPSE)         on_select(app AID)
IDLE ──────────────────────────► PPSE_SELECTED ──────────────────► APP_SELECTED
  ◄──── on_deselect / on_field_off ────────────────────────────────────┘
                                              on_apdu(GPO) ──► GPO_DONE
                                                     ◄─ on_deselect / on_field_off ─┘
                                                              │ on_apdu(READ RECORD) (loop)
                                                              └──────────────────────────┘
```

Transitions:
| Current State       | Event               | Next State          | Response          |
|---------------------|---------------------|---------------------|-------------------|
| ANY                 | on_select(PPSE AID) | PPSE_SELECTED       | PPSE FCI + 9000   |
| ANY                 | on_select(app AID)  | APP_SELECTED        | App FCI + 9000    |
| ANY                 | on_deselect         | IDLE                | (none)            |
| ANY                 | on_field_off        | IDLE                | (none)            |
| APP_SELECTED / GPO_DONE | on_apdu(GPO)   | GPO_DONE            | GPO resp + 9000   |
| PPSE_SELECTED / IDLE | on_apdu(GPO)      | (unchanged)         | 6985              |
| GPO_DONE            | on_apdu(READ REC)   | GPO_DONE            | Record + 9000 / 6A83 |
| other               | on_apdu(READ REC)   | (unchanged)         | 6985              |
| ANY                 | on_apdu(other)      | (unchanged)         | 6D00              |

---

## 5. Integration Points

### 5.1 Down — Functions Called by This Service

```c
/* Wave 1 (HAL) */
int nfc_transport_send_response(const uint8_t *buf, size_t len);
/*   @threadsafe; borrows buf until next callback; return cast to (void).         */
/*   MISRA Rule 17.7: return value explicitly cast to (void) — no recovery        */
/*   possible from a response failure at service level.                           */

/* Wave 2 (framing/apdu_types.h) — zero-copy APDU view, used as-is */
/* nfc_apdu_t fields: cla, ins, p1, p2, data (NULL if lc==0), lc, le, has_le */

/* SW constants from apdu_types.h: NFC_SW_OK, NFC_SW_INS_NOT_SUPPORTED,          */
/* NFC_SW_FILE_NOT_FOUND, NFC_SW_CONDITIONS_NOT_SATISFIED,                        */
/* NFC_SW_WRONG_LENGTH, NFC_SW1, NFC_SW2                                          */
/* NFC_SW_RECORD_NOT_FOUND, NFC_SW_WRONG_P1P2 (wave2-framing §1.1 vocabulary)    */
```

### 5.2 Up — Callers of This Service

```c
/* Wave 3 (router) — calls via nfc_service_t vtable */
svc->on_select(aid, aid_len, user_ctx);   /* on nfc_work_q; exact AID match */
svc->on_apdu(apdu, user_ctx);             /* on nfc_work_q; all non-SELECT APDUs */
svc->on_deselect(user_ctx);               /* on nfc_work_q; re-SELECT deselects */
svc->on_field_off(user_ctx);              /* on nfc_work_q; field removed */

/* Wave 4 (nfc_stack) — lifecycle */
emv_service_init(NULL);  /* called from nfc_stack_init(); NULL → defaults */
emv_service_shutdown();  /* called from nfc_stack_shutdown() */
emv_service_get();       /* called during profile registration (PROFILE_EMV / PROFILE_ALL) */
/* Wave 4 amendment (DECISION-1): */
emv_service_get_app_aid();      /* called in NFC_PROFILE_EMV case */
emv_service_get_app_aid_len();  /* called in NFC_PROFILE_EMV case */

/* Wave 6 (store) — persistence */
svc->serialize(out, max, out_len, user_ctx);   /* @caller_sync; stack quiesced */
svc->deserialize(in, len, user_ctx);           /* @caller_sync; stack quiesced */
/* persist_id = NFC_PERSIST_ID_EMV = 0x04U (wave3-router §1.1 table)            */
```

### 5.3 APDU Decision Table (complete)

| Command | CLA | INS | P1 | P2 | lc / data | Session state | SW Response | Body |
|---------|-----|-----|----|----|-----------|---------------|-------------|------|
| SELECT PPSE | `00` | `A4` | `04` | `00` | `0E` / PPSE AID | Any → `PPSE_SELECTED` | `9000` | PPSE FCI TLV |
| SELECT app AID | `00` | `A4` | `04` | `00` | app AID len / app AID | Any → `APP_SELECTED` | `9000` | App FCI TLV |
| SELECT other AID | `00` | `A4` | `04` | `00` | other | Any → `IDLE` | `6A82` | — |
| GET PROCESSING OPTIONS | `80` | `A8` | `00` | `00` | any | `APP_SELECTED`/`GPO_DONE` → `GPO_DONE` | `9000` | `80 06` + AIP + AFL |
| GET PROCESSING OPTIONS | `80` | `A8` | `00` | `00` | any | `PPSE_SELECTED`/`IDLE` | `6985` | — |
| READ RECORD — valid SFI, valid rec# | `00` | `B2` | rec# | `(SFI<<3)\|04` | absent | `GPO_DONE` | `9000` | record TLV |
| READ RECORD — valid SFI, rec# out of range | `00` | `B2` | bad rec# | `(SFI<<3)\|04` | absent | `GPO_DONE` | `6A83` | — |
| READ RECORD — wrong SFI | `00` | `B2` | any | wrong SFI | absent | `GPO_DONE` | `6A82` | — |
| READ RECORD — bad P2 format | `00` | `B2` | any | P2&7≠4 | absent | `GPO_DONE` | `6B00` | — |
| READ RECORD — wrong state | `00` | `B2` | any | any | any | not `GPO_DONE` | `6985` | — |
| Any other INS | any | any | any | any | any | Any | `6D00` | — |

> **Note on SELECT routing:** `on_select` is only called by the router when an
> incoming AID matched a registered entry. With the Wave 4 amendment (DECISION-1)
> in place, only PPSE AID and app AID are registered — so `on_select` will only
> ever receive those two AIDs. The "SELECT other AID" row is a defensive fallback.

### 5.4 Response Assembly Pattern

All responses use `s_resp_buf`. The pattern is:

```c
/* Example: READ RECORD success */
size_t pos = 0U;
(void)memcpy(&s_resp_buf[pos], s_card_image.record_data[idx],
             (size_t)s_card_image.record_len[idx]);
pos += (size_t)s_card_image.record_len[idx];
s_resp_buf[pos]     = NFC_SW1(NFC_SW_OK);
s_resp_buf[pos + 1U] = NFC_SW2(NFC_SW_OK);
pos += 2U;
(void)nfc_transport_send_response(s_resp_buf, pos);
```

Error-only responses (no body):
```c
s_resp_buf[0U] = NFC_SW1(sw);
s_resp_buf[1U] = NFC_SW2(sw);
(void)nfc_transport_send_response(s_resp_buf, 2U);
```

### 5.5 RESERVED — `emv_poller` (Reader Role)

> **Not implemented in this slice.** When a reader-capable backend becomes available
> (`CONFIG_NFC_ROLE_READER` + ST25R3916/RFAL or PN7160), the EMV protocol module gains
> an `emv_poller` component that performs a standard EMV card-reading sequence
> (SELECT PPSE → SELECT AID → GET PROCESSING OPTIONS → READ RECORD) and populates
> the same `emv_card_image_t` model that this listener serves. The result would
> serialize with completeness flag `NFC_STORE_ENTRY_FLAG_READER_CAPTURED` (wave6-store.md §1.7 / spec v3 §4.5).
> **Header/interface placeholder only** — no `emv_poller` API is declared or compiled
> in this slice. When `CONFIG_NFC_ROLE_READER` is added, the poller source compiles
> under that gate, orthogonal to `CONFIG_NFC_ROLE_CARD`.

---

## 6. TLV Response Builder Internals

These are pure static functions in `emv_service.c`, callable with no global state reads.
They are the primary targets for ztest coverage.

### 6.1 `emv_rebuild_caches(const emv_card_image_t *img)` → `int`

Called from `init()` and `deserialize()`. Builds:
1. `s_ppse_fci_buf` / `s_ppse_fci_len` via `emv_build_ppse_fci(img, ...)`
2. `s_app_fci_buf` / `s_app_fci_len` via `emv_build_app_fci(img, ...)`
3. `s_gpo_buf` / `s_gpo_len` via `emv_build_gpo_response(img, ...)`
4. `s_card_image.record_data[i]` / `record_len[i]` via `emv_build_record(img, i, ...)`

Returns 0 or `-EIO` (any builder overflowed its buffer).

### 6.2 PPSE FCI Structure

```
6F [len]           FCI Template
  84 0E [14B]      DF Name = "2PAY.SYS.DDF01"
  A5 [len]         FCI Proprietary Template
    BF 0C [len]    FCI Issuer Discretionary Data
      61 [len]     Directory Entry
        4F [alen] [app AID bytes]       Application Identifier
        50 0A "VISA DEBIT"              Application Label (10 bytes example)
        87 01 01                         Application Priority Indicator
```

### 6.3 App FCI Structure

```
6F [len]           FCI Template
  84 [alen] [AID]  DF Name = app AID
  A5 [len]         FCI Proprietary Template
    50 09 "TEST CARD"         Application Label
    87 01 01                   Application Priority Indicator
```

> No PDOL (tag 9F38) in App FCI — GPO will accept any data field (DECISION-6).

### 6.4 GPO Response (Format 1)

```
80 06 [AIP:2bytes] [AFL:4bytes]
```

AIP = `s_card_image.aip[0..1]`; AFL = `s_card_image.afl[0..3]`.
Total body = 6 bytes; with tag+length: `80 06 xx xx xx xx xx xx` = 8 bytes.

### 6.5 READ RECORD Response

Pre-built by `emv_build_record(img, rec_idx, buf, cap, &out_len)`:
```
70 [len]           Record Template
  57 [t2_len] [track2 bytes]         Track 2 Equivalent Data
  5A 08 [pan:8 bytes]                PAN (BCD, 8 bytes)
  5F 24 03 [expiry:3 bytes]          Expiry Date (YYMMDD)
  5F 20 [name_len] [name bytes]      Cardholder Name
  9F 36 02 00 01                      Application Transaction Counter (ATC = 1)
```

> ATC is static `0x0001` — no increment logic. Flagged: a reader performing
> velocity checking will always see the same ATC; acceptable for protocol-walk
> emulation.

---

## 7. Implementation Notes

### 7.1 MISRA C:2012 Compliance

| Decision point | MISRA rule | Note / deviation |
|----------------|------------|-----------------|
| All integer literals suffixed `U` for unsigned arithmetic | Rule 7.2 | `0x6FU`, `0x9000U`, etc. |
| `s_resp_buf`, `s_ppse_fci_buf`, etc. are file-static (BSS), **not** `static` locals | Dir 4.9 / creed §9 | Static locals in functions are forbidden |
| `switch` in `emv_service_on_apdu` has `default:` sending `6D00` | Rule 16.4 | No dead code |
| All variables initialized at declaration | Rule 9.1 | `size_t pos = 0U;`, `tlv_writer_t w = { 0 };` |
| `(void)nfc_transport_send_response(...)` | Rule 17.7 | No recovery from send failure at this layer; explicit cast required |
| `(void)memcpy(...)` | Rule 17.7 | Return value unused; explicit cast required |
| `memcmp` for AID comparison | Rule 21.14 | `memcmp` is MISRA-permitted; no `strcmp` on byte arrays |
| `K_SPINLOCK_DEFINE` not available in NCS v3.2.4; use `static struct k_spinlock` | — | CONVENTIONS §6 |
| `CONFIG_NFC_EMV_MAX_RECORDS` / `CONFIG_NFC_EMV_RECORD_SIZE` in array sizes | Rule 1.1 / VLA | These are compile-time Kconfig constants → fixed-size arrays, no VLA |
| TLV writer uses `bool error` field | Rule 4.5 | `stdbool.h` included; `bool` is permitted |
| Log macros | DEV-ZEP-008 | `LOG_MODULE_REGISTER(emv_service, LOG_LEVEL_INF)` |
| Shell `shell_print` variadic | DEV-ZEP-009 | Pre-approved deviation |
| `CONTAINER_OF` not needed (no `k_fifo` or `net_buf` in this service) | — | No DEV-ZEP-001 needed |
| `IS_ENABLED(CONFIG_NFC_SERVICE_EMV)` guard in CMakeLists | — | Source only compiled when enabled |

### 7.2 Buffer Model (CONVENTIONS §5)

- **Inbound**: `nfc_apdu_t` is a zero-copy view. `apdu->data` is valid only for the
  duration of `on_apdu()`. The service does NOT retain `apdu->data` after returning.
  The GPO data field (PDOL response) is ignored without copying (DECISION-6).
- **Outbound**: `s_resp_buf` is file-static, borrowed by HAL until next callback.
  Since the service is synchronous (no deferred work), the next use of `s_resp_buf`
  occurs at the NEXT `on_apdu`/`on_select` invocation — which only happens after
  the reader ACKs the previous response. Buffer lifetime is safe.
- No `net_buf` usage; this layer is response-only.

### 7.3 Threading (CONVENTIONS §7)

- `on_select`, `on_apdu`, `on_deselect`, `on_field_off`: exclusively on `nfc_work_q`.
  `s_session_state`, `s_card_image` reads, `s_resp_buf` writes: **no lock needed**
  (single writer/reader, single thread).
- `init()`, `shutdown()`, `get_config()`, `get_state()`, `serialize()`, `deserialize()`:
  `@caller_sync`. `serialize`/`deserialize` only run while stack is NOT STARTED
  (CONVENTIONS §3, spec §6.5 `-EBUSY` gating); no lock needed on `s_card_image`.
- `get_stats()`: acquires `s_stats_lock` (spinlock; `@isr_safe`).
- `emv_service_get()`, `emv_service_get_app_aid()`, `emv_service_get_app_aid_len()`:
  read-only static data; `@isr_safe`.

### 7.4 Serialization Format (Version 1)

```
Offset  Size   Field
     0     1   Format version (0x01)
     1     1   pan_len
     2     8   pan[8]
    10     3   expiry[3]
    13    26   name[26]
    39     1   track2_len
    40    19   track2[19]
    59     2   aip[2]
    61     4   afl[4]
    65     1   app_aid_len
    66    16   app_aid[16]
    82     1   record_count
    83     1   record_data[0] actual length (N0)
    84    CONFIG_NFC_EMV_RECORD_SIZE  record_data[0][0..RECORD_SIZE-1]
    84+RECORD_SIZE   1   record_data[1] actual length (N1)
    ...  (repeated for each record up to record_count)
```

`deserialize()` validates: version == 1, `len >=` minimum for declared `record_count`,
`app_aid_len == EMV_SERVICE_APP_AID_LEN`, `memcmp(app_aid, k_emv_app_aid)` (DECISION-1).

`BUILD_ASSERT(EMV_SERVICE_MAX_SERIALIZED <= STORE_BUFFER_MAX, ...)` in `emv_service.c`
(placeholder — actual store buffer size confirmed by Wave 6).

### 7.5 TLV Construction Safety

- Every write path goes through `tlv_put_byte` / `tlv_put_bytes` / `tlv_put_primitive`
  which check `w->pos < w->cap` before writing; on overflow, `w->error = true` and all
  subsequent writes are no-ops.
- `emv_rebuild_caches()` checks `tlv_error(&w)` after each builder call; returns `-EIO`
  on any overflow.
- No `sprintf`, `strlcpy`, or similar unbounded-write function is used anywhere in
  the TLV path.
- Two-byte tags (`5F24`, `5F20`, `9F36`, `BF0C`) are written as two explicit
  `tlv_put_byte` calls, not as a word-sized write, avoiding alignment concerns
  (MISRA Rule 11.3 / endian safety).

### 7.6 Vtable Initializer

```c
static const nfc_service_t s_vtable = {
    .on_select     = emv_service_on_select,
    .on_apdu       = emv_service_on_apdu,
    .on_deselect   = emv_service_on_deselect,
    .on_field_off  = emv_service_on_field_off,
    .serialize     = emv_service_serialize,
    .deserialize   = emv_service_deserialize,
    .persist_id    = NFC_PERSIST_ID_EMV,  /* 0x04 — wave3-router §1.1 */
    .user_ctx      = NULL,                /* singleton; all state is file-static */
};
```

---

## 8. Conventions Compliance Checklist (CONVENTIONS §12)

- [x] **Lifecycle §2** — minimal: `init` / `shutdown` only; no `start`/`stop`/`deinit`;
      state enum `{UNINITIALIZED, INITIALIZED}`.
- [x] **`config_t` / `stats_t` / `state_t` + three getters §2** — `emv_service_config_t`,
      `emv_service_stats_t`, `emv_service_state_t`; `get_config()`, `get_stats()`,
      `get_state()` all defined.
- [x] **Pattern A §2** — plain enum `s_state`; WQ-thread-only session state; no `atomic_t`
      needed (no ISR reads of `s_state`).
- [x] **Coupling §3** — service calls `nfc_transport_send_response()` directly (downward);
      receives callbacks via vtable (upward Pattern A). No cross-layer header includes
      except vtable and apdu_types.
- [x] **Callbacks §4** — vtable fields use `_fn` typedef suffix per `service.h`; `user_ctx`
      is NULL (singleton); invoker (router) NULL-checks per wave 3; dispatch thread
      documented on every public function.
- [x] **Buffer model §5** — no `net_buf` (response-only service); `s_resp_buf` is
      file-static (BSS), not a static local; HAL borrows it until next callback.
- [x] **Stats recipe §6** — `s_stats` + `s_stats_lock`; `STATS_RESET` after `-EALREADY`
      guard; `STATS_ERROR` on every non-fatal failure; named counters for each drop
      path; copy-out getter via `STATS_COPY_OUT`.
- [x] **Threading §7** — all vtable callbacks `@nfc_work_q`; lifecycle `@caller_sync`;
      getters `@isr_safe`; every public function annotated; no lock on session state
      (WQ-only); spinlock on stats.
- [x] **Error codes §9** — errno only: `-EINVAL` null ptr, `-EALREADY` double-init,
      `-ENODEV` not needed (vtable safe before init), `-EBADMSG` corrupt blob,
      `-ENOSPC` serialize overflow, `-EIO` cache build failure.
- [x] **Shell §10** — `emv_service_shell_cmds.c` registers `config` / `stats` / `state`
      subcommands under `emv_service`; `shell_print` variadic DEV-ZEP-009.
- [x] **MISRA §11** — full citation table in §7.1; all DEV-ZEP deviations noted.

---

## 9. Tasks

### Phase 0 — Scaffolding

- [ ] **T01. Create directory and CMakeLists.txt.**
  Create `src/nfc/services/emv/`. Write `CMakeLists.txt`:
  ```cmake
  if(CONFIG_NFC_SERVICE_EMV)
    target_sources(app PRIVATE
      emv_service.c
      emv_service_shell_cmds.c
    )
    target_include_directories(app PUBLIC .)
  endif()
  ```
  **Commit.**

- [ ] **T02. Write Kconfig.**
  ```kconfig
  config NFC_EMV_MAX_RECORDS
      int "Maximum number of EMV records in the static card image"
      default 2
      range 1 8
      depends on NFC_SERVICE_EMV
      help
        Compile-time maximum record count. Increasing this raises RAM usage
        by CONFIG_NFC_EMV_RECORD_SIZE bytes per additional record slot.

  config NFC_EMV_RECORD_SIZE
      int "Maximum size of a single pre-built record TLV blob in bytes"
      default 64
      range 32 256
      depends on NFC_SERVICE_EMV
      help
        Size of each record_data[i] slot in emv_card_image_t. Must be
        large enough for the tag-70 TLV encoding of track2 + PAN + expiry
        + name + ATC. Default 64 is sufficient for a 16-digit PAN record.
  ```
  **Note:** `NFC_SERVICE_EMV` enable symbol lives in `src/nfc/Kconfig` (wave4
  §1.6) — not re-declared here.
  **Commit.**

- [ ] **T03. Write `emv_service.h` skeleton.**
  Transcribe §1 (types/constants) and §2 (public API) verbatim. Verify that
  `EMV_SERVICE_MAX_SERIALIZED` macro compiles without warning (uses
  `CONFIG_NFC_EMV_MAX_RECORDS` and `CONFIG_NFC_EMV_RECORD_SIZE`). `#ifndef`
  guard `SRC_NFC_SERVICES_EMV_EMV_SERVICE_H`. Includes: `<stdint.h>`, `<stddef.h>`,
  `<stdbool.h>`. No Zephyr kernel headers in the public header.
  **Commit.**

### Phase 1 — TLV Writer (TDD — pure, ztest-able)

- [ ] **T04. Write `tests/unit/nfc_emv/test_tlv_writer.c` — RED.**
  Create `tests/unit/nfc_emv/CMakeLists.txt` (ztest; `qemu_cortex_m33` locally, `native_sim` on Linux CI). Write
  test cases:
  - `test_tlv_put_primitive_short` — write a 1-byte-tag, short-length primitive;
    verify byte-exact output.
  - `test_tlv_put_primitive_two_byte_tag` — write `5F24` tag; verify first byte
    `0x5F`, second byte `0x24`, then length, then value.
  - `test_tlv_open_close_constructed` — open a `6F` constructed, write content,
    close; verify length byte is patched correctly.
  - `test_tlv_overflow_sets_error` — write to a 4-byte buffer with 5-byte content;
    verify `tlv_error()` returns `true`, no out-of-bounds write.
  - `test_tlv_noop_after_error` — after overflow, subsequent writes are no-ops;
    final pos unchanged.
  All tests RED (functions not yet defined).
  **Commit.**

- [ ] **T05. Implement `tlv_writer_t` helpers in `emv_service.c` — GREEN.**
  Implement `tlv_init`, `tlv_put_byte`, `tlv_put_bytes`, `tlv_put_primitive`,
  `tlv_open_constructed`, `tlv_close_constructed`, `tlv_error`, `tlv_len` as
  `static inline` functions. Move shared internal test-stubs or use a separate
  `emv_tlv.h` private header if tests need to include them — but prefer making
  the functions `static` in `emv_service.c` and have the test file include the
  `.c` directly via a well-known technique:
  ```c
  /* test_tlv_writer.c */
  #define STATIC_FOR_TEST   /* empty — makes statics visible when compiled into test */
  #include "emv_service.c"  /* include whole translation unit; pure-C, no platform deps */
  ```
  All T04 tests GREEN. **Commit.**

### Phase 2 — Response Builders (TDD — pure, ztest-able)

- [ ] **T06. Write `tests/unit/nfc_emv/test_response_builders.c` — RED.**
  Test cases (all using a known card image struct, no global state):
  - `test_build_ppse_fci_structure` — call `emv_build_ppse_fci(&img, buf, cap, &len)`;
    verify: starts with `6F`; tag `84` present with 14-byte PPSE AID; tag `BF0C`
    present; tag `4F` with correct app AID; `tlv_error == false`; `len ≤ cap`.
  - `test_build_ppse_fci_overflow` — call with cap=10; verify returns `-ENOSPC`
    (or sets error flag and builder returns error). **DECISION:** builders return
    `int` (0 / `-ENOSPC`); they do NOT call `send_response`.
  - `test_build_app_fci_structure` — verify `6F` > `84` (app AID) > `A5` > `50`
    (label) present; length fields correct.
  - `test_build_gpo_response` — verify `80 06 [AIP] [AFL]`; exactly 8 bytes.
  - `test_build_record_structure` — verify starts with `70`; tag `57` present
    (track2); tag `5A` present (PAN); `5F24` (expiry); `5F20` (name); `9F36` (ATC).
  - `test_build_record_index_out_of_range` — record_count=1, request index=1;
    verify returns `-EINVAL`.
  All RED.
  **Commit.**

- [ ] **T07. Implement response builder functions — GREEN.**
  Implement `emv_build_ppse_fci`, `emv_build_app_fci`, `emv_build_gpo_response`,
  `emv_build_record` in `emv_service.c`. Each takes `(const emv_card_image_t *img,
  uint8_t *buf, size_t cap, size_t *out_len)`. All use `tlv_writer_t` exclusively.
  All bounds-checked (DECISION-2). All T06 tests GREEN. **Commit.**

- [ ] **T08. Implement `emv_rebuild_caches()` — GREEN.**
  Calls each builder; stores results in module-static cache vars. Checks
  `tlv_error()` and returns `-EIO` on any overflow. Test: `test_rebuild_caches_ok`
  (default card image → returns 0, cache lengths > 0). **Commit.**

### Phase 3 — Serialization (TDD — pure, ztest-able)

- [ ] **T09. Write `tests/unit/nfc_emv/test_serialize.c` — RED.**
  Test cases:
  - `test_serialize_round_trip` — serialize a known card image to a buffer; call
    deserialize into a fresh struct; verify all fields match byte-for-byte.
  - `test_serialize_too_small` — cap = 10; verify returns `-ENOSPC`.
  - `test_deserialize_wrong_version` — blob version byte = 0xFF; verify `-EBADMSG`.
  - `test_deserialize_wrong_app_aid` — blob with a different `app_aid`; verify `-EBADMSG`.
  - `test_deserialize_truncated` — len < minimum; verify `-EBADMSG`.
  All RED.
  **Commit.**

- [ ] **T10. Implement `emv_service_serialize` / `emv_service_deserialize` — GREEN.**
  Implement per §7.4 format. `deserialize` validates version, length, app_aid match,
  then calls `emv_rebuild_caches()` on the newly loaded image; returns `-EIO` on
  cache rebuild failure. All T09 tests GREEN. **Commit.**

### Phase 4 — Module Skeleton

- [ ] **T11. Implement lifecycle — `init()` and `shutdown()`.**
  ```c
  int emv_service_init(const emv_service_config_t *cfg) {
      if (s_state == EMV_SERVICE_STATE_INITIALIZED) { return -EALREADY; }
      STATS_RESET(s_stats);             /* after -EALREADY guard per CONVENTIONS §6 */
      s_card_image = k_emv_default_card;
      int rc = emv_rebuild_caches(&s_card_image);
      if (rc != 0) { return -EIO; }
      s_session_state = EMV_SESSION_IDLE;
      s_state = EMV_SERVICE_STATE_INITIALIZED;
      return 0;
  }
  int emv_service_shutdown(void) {
      s_session_state = EMV_SESSION_IDLE;
      s_state = EMV_SERVICE_STATE_UNINITIALIZED;
      return 0;
  }
  ```
  Confirm with a smoke test (init → check state, second init → -EALREADY, shutdown →
  UNINITIALIZED). **Commit.**

- [ ] **T12. Implement getters and `emv_service_get_app_aid()` family.**
  Implement `get_config`, `get_stats`, `get_state`, `emv_service_get`,
  `emv_service_get_app_aid`, `emv_service_get_app_aid_len`. All trivial except
  `get_stats` which uses `STATS_COPY_OUT`. **Commit.**

### Phase 5 — Dispatch Handlers

- [ ] **T13. Implement `emv_service_on_select()`.**
  Exact AID match via `memcmp`. Two branches: PPSE → copy `s_ppse_fci_buf` to
  `s_resp_buf`, append SW `9000`, call `send_response`; app AID → same with
  `s_app_fci_buf`; else → send `6A82`, `STATS_ERROR`. Update `s_session_state`.
  Increment `select_ppse_count` or `select_app_count` via `STATS_INC`.
  **Commit.**

- [ ] **T14. Implement `emv_service_on_apdu()` — GPO path.**
  Guard: `apdu->cla == 0x80U && apdu->ins == 0xA8U` (GPO).
  State check: `APP_SELECTED` or `GPO_DONE` → serve; else send `6985` +
  `STATS_INC(apdu_bad_state_count)`. On serve: copy `s_gpo_buf`, append `9000`,
  send; `s_session_state = EMV_SESSION_GPO_DONE`; `STATS_INC(gpo_count)`.
  **Commit.**

- [ ] **T15. Implement `emv_service_on_apdu()` — READ RECORD path.**
  Guard: `apdu->cla == NFC_CLA_ISO7816 && apdu->ins == 0xB2U` (READ RECORD).
  State check: `GPO_DONE` only; else `6985`. P2 format check: `(apdu->p2 & 0x07U) != 4U`
  → `6B00`. SFI extraction: `sfi = (apdu->p2 >> 3U) & 0x1FU`; compare to AFL SFI:
  `afl_sfi = (s_card_image.afl[0] >> 3U) & 0x1FU`; mismatch → `6A82`. Record
  number: `rec_no = apdu->p1`; range check 1 ≤ rec_no ≤ record_count; else `6A83`.
  On serve: memcpy `record_data[rec_no - 1U]`, append `9000`, send;
  `STATS_INC(read_record_count)`. **Commit.**

- [ ] **T16. Implement `emv_service_on_apdu()` — default / other INS path.**
  `default:` branch in the INS `switch`: send `6D00` (INS not supported);
  `STATS_INC(apdu_unknown_count)`. Ensure `switch` has `default:` (MISRA Rule 16.4).
  **Commit.**

- [ ] **T17. Implement `on_deselect()` and `on_field_off()`.**
  Both: `s_session_state = EMV_SESSION_IDLE;`. No stats increment needed (not errors).
  **Commit.**

### Phase 6 — Shell

- [ ] **T18. Implement `emv_service_shell_cmds.c`.**
  ```c
  SHELL_STATIC_SUBCMD_SET_CREATE(emv_service_cmds,
      SHELL_CMD(config, NULL, "Print EMV service config", cmd_emv_service_config),
      SHELL_CMD(stats,  NULL, "Print EMV service stats",  cmd_emv_service_stats),
      SHELL_CMD(state,  NULL, "Print EMV service state",  cmd_emv_service_state),
      SHELL_SUBCMD_SET_END);
  SHELL_CMD_REGISTER(emv_service, &emv_service_cmds, "EMV service control", NULL);
  ```
  `cmd_emv_service_config`: calls `emv_service_get_config()`; prints `_reserved` (trivial).
  `cmd_emv_service_stats`: calls `emv_service_get_stats()`; prints all 8 fields.
  `cmd_emv_service_state`: calls `emv_service_get_state()`; prints enum name string.
  **Commit.**

### Phase 7 — Integration

- [ ] **T19. Wire into `src/nfc/Kconfig` (Wave 4 sourcing extension).**
  Add `rsource "services/emv/Kconfig"` to `src/nfc/Kconfig` (alongside the
  existing service Kconfig rsources — the Wave 4 plan has these as commented
  placeholders). **Commit.**

- [ ] **T20. Flag Wave 4 follow-up in code (DECISION-1 marker).**
  Add a `/* TODO(Wave4-amendment, DECISION-1): ... */` comment inside the
  `NFC_PROFILE_EMV` case in `nfc_stack.c` referencing the second
  `aid_router_register` call for the payment app AID. Add the
  `emv_service_get_app_aid()` declaration to `emv_service.h` (already done
  in T03). **Commit.**

- [ ] **T21. Verify `NFC_SW_RECORD_NOT_FOUND` and `NFC_SW_WRONG_P1P2` in `apdu_types.h`.**
  Both are part of the wave2-framing §1.1 vocabulary (DECISION-7). Confirm they
  exist there and that this service defines no local SW constants. **Commit.**

- [ ] **T22. Run all ztest suites; confirm no regressions.**
  ```sh
  west twister -T tests/unit/nfc_emv --platform qemu_cortex_m33 --no-sysbuild
  ```
  Verify all T04, T06, T09 test cases pass. Fix any compile-time lints (MISRA). **Commit.**

- [ ] **T23. Run ReadLints on all produced files; resolve any linter errors.**
  Files to check: `emv_service.c`, `emv_service.h`, `emv_service_shell_cmds.c`.
  Fix all errors. **Commit.**

---

## Appendix A: Wire-Level Traces

### Full PPSE → App SELECT → GPO → READ RECORD session

```
Reader → Card:  00 A4 04 00 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 00
Card → Reader:  6F 29
                  84 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31
                  A5 17
                    BF 0C 14
                      61 12
                        4F 07 A0 00 00 00 03 10 10
                        50 0A 56 49 53 41 20 44 45 42 49 54   (VISA DEBIT)
                        87 01 01
                90 00

Reader → Card:  00 A4 04 00 07 A0 00 00 00 03 10 10 00
Card → Reader:  6F 1B
                  84 07 A0 00 00 00 03 10 10
                  A5 10
                    50 09 54 45 53 54 20 43 41 52 44          (TEST CARD)
                    87 01 01
                90 00

Reader → Card:  80 A8 00 00 02 83 00 00
Card → Reader:  80 06 00 00 0C 01 01 00
                90 00

Reader → Card:  00 B2 01 0C 00
Card → Reader:  70 [len]
                  57 10 99 99 99 99 99 99 99 99 D9 91 21 10 10 00 00 0F
                  5A 08 99 99 99 99 99 99 99 99
                  5F 24 03 99 12 31
                  5F 20 0A 54 45 53 54 2F 43 41 52 44
                  9F 36 02 00 01
                90 00
```

---

## Appendix B: Memory Budget

| Symbol | Size (bytes) | Location |
|--------|-------------|----------|
| `s_card_image` (default: 2 records × 64) | ~220 | BSS |
| `s_ppse_fci_buf` | 128 | BSS |
| `s_app_fci_buf` | 128 | BSS |
| `s_gpo_buf` | 16 | BSS |
| `s_resp_buf` | 256 | BSS |
| `s_stats` | ~32 | BSS |
| `s_stats_lock` | ≤4 | BSS |
| `s_vtable` | ~40 | RODATA |
| `k_emv_default_card` | ~220 | RODATA |
| `k_emv_ppse_aid`, `k_emv_app_aid` | 21 | RODATA |
| **Total RAM (BSS)** | **~790** | |

Dominant costs: `s_card_image` (scales with `CONFIG_NFC_EMV_MAX_RECORDS × RECORD_SIZE`)
and the two FCI cache buffers. At Kconfig defaults, total RAM < 1 KB.
