# Wave 6: Store (nfc_store) Implementation Plan

> **Architecture reframe — spec v3 (2026-06-12).**
> This plan is part of the capability-driven, dual-role architecture defined in
> [`docs/superpowers/specs/2026-06-12-nfc-stack-architecture.md`](../specs/2026-06-12-nfc-stack-architecture.md)
> §4.5 + §9 (Wave 6 row).
>
> **The Wave 6 store blob IS the unified, cross-device-portable `.card` interchange
> format** — not merely nRF-local persistence. The same versioned container the
> card role serializes today is the file a future reader role will produce after
> capturing a real card, and the file a synthetic instantiation loads from.
> Cross-device portability guarantees (all already satisfied by the existing format):
> - **Keyed by `persist_id`** (not array order) — a card captured under one profile
>   composition restores correctly under another; unknown `persist_id` entries are
>   silently skipped.
> - **Versioned** — forward-compatible evolution without breaking stored blobs; v0x01
>   blobs load cleanly in a v0x02 decoder (flags treated as 0).
> - **CRC-protected** — detects corruption, truncation, and bit-flip before any
>   service deserializer is invoked.
>
> **The store and file format are role-independent.** Used by the card role today;
> the reader role will produce identical blobs after capturing a physical card.
> Existing Kconfig symbol names (`NFC_STORE`, `NFC_STORE_BLOB_SIZE`, etc.) are
> final and are not scoped to card-only use.

**Layer:** `store/nfc_store` — persistence stub seam for NFC card data

**Files produced:**
- `src/nfc/store/nfc_store.h`
- `src/nfc/store/nfc_store.c`
- `src/nfc/store/nfc_store_shell_cmds.c`
- `src/nfc/store/nfc_store_default_cards.h`
- `src/nfc/store/CMakeLists.txt`
- `src/nfc/store/Kconfig`
- `tests/unit/nfc_store/` (own suite — blob codec pure-C tests + save/load orchestration; Twister tag `sigmation.nfc.store`; `native_sim` is Linux-CI-only — use `qemu_cortex_m33` or DK locally)

**Depends on:**
- `docs/superpowers/plans/wave1-hal.md` (LOCKED — 2026-06-12) — threading/WQ/context model
- `docs/superpowers/plans/wave2-framing.md` (LOCKED — 2026-06-12) — `apdu_types.h` vocabulary
- `docs/superpowers/plans/wave3-router.md` (LOCKED — 2026-06-12) — **binding**: `nfc_service_t` vtable incl. `serialize`/`deserialize`/`persist_id`, persist_id table, `NFC_PERSIST_ID_*` macros
- `docs/superpowers/plans/wave4-stack.md` (LOCKED — 2026-06-12) — **binding**: wave4 §5.3 expected API surface (exact signatures confirmed here), `-EBUSY` quiescence rule, `nfc_stack_save/load` callers
- `docs/superpowers/plans/wave5-ndef.md` (LOCKED) — `NDEF_SERVICE_MAX_SERIALIZED`
- `docs/superpowers/plans/wave5-desfire.md` (LOCKED) — `DESFIRE_SERVICE_MAX_SERIALIZED`
- `docs/superpowers/plans/wave5-ultralight.md` (LOCKED) — `ULTRALIGHT_SERVICE_MAX_SERIALIZED`
- `docs/superpowers/plans/wave5-emv.md` (LOCKED) — `EMV_SERVICE_MAX_SERIALIZED`
- `docs/superpowers/plans/wave5-aliro.md` (LOCKED) — `ALIRO_SERVICE_MAX_SERIALIZED`

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 minimal lifecycle/Pattern A, §3 coupling map + store row, §4 callback pattern, §6 stats recipe, §7 threading, §9 errno, §10 shell, §11 MISRA, §12 checklist)
2. `docs/superpowers/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) — §6.5 (store: stub seam, load primary/save debug, TLV format, save/load stubs, shell, `-EBUSY` gating), §9 (Kconfig symbol map), §12 (out of scope: real backend)
3. `docs/superpowers/plans/wave3-router.md` §1.1 — `nfc_service_t`, `serialize`/`deserialize` signatures, `persist_id` table
4. `docs/superpowers/plans/wave4-stack.md` §5.3 + §3 (`nfc_stack_save/load` sequence, `-EBUSY` rule, quiescence contract)
5. Wave 5 plans — `*_MAX_SERIALIZED` constants (§1 each)
6. `docs/API_DESIGN_CREED.md` — module lifecycle, Pattern A state, coupling, shell, creed §4 (errno), creed §9 (no static-local), creed §11 (shell), creed §13 (no dynamic allocation)
7. `docs/CALLBACK_REGISTRATION_GUIDE.md` — `_fn`/`_cb`, `user_ctx`, guard/NULL-clear, default-stub pattern
8. `docs/STATS_API_DESIGN.md` — `s_stats`/`s_stats_lock`, copy-out, `STATS_*` macros
9. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_RESET`, `STATS_COPY_OUT`
10. `prj.conf` — no NVS/settings/LittleFS enabled; confirms stub-seam-only scope for Wave 6
11. `/Users/majidfaroud/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-008/009/013/015/017
12. `/Users/majidfaroud/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012

> **NOT consulted / excluded:** `src/nfc_emulation.c/.h` (replaced prototype). Zephyr NVS/Settings APIs not read — real backends are post-Wave-6 callback swaps (spec v2 §12).

---

## DECISION Summary (all flags)

| ID | Decision | Where |
|----|----------|-------|
| **DECISION-ST-1** | Backend = stub seam only (load stub: compiled-in `.h`; save stub: shell hex dump). No NVS/Settings. `prj.conf` has neither; spec v2 §12 calls them out-of-scope. | §1, §6 |
| **DECISION-ST-2** | `nfc_store_config_t` has one field: `max_tag_len` (uint8_t; 0 → `CONFIG_NFC_STORE_MAX_TAG_LEN`). All other tunables are Kconfig. NULL cfg → defaults. | §1.1 |
| **DECISION-ST-3** | Blob format: 6-byte versioned header + TLV entries (1B persist_id + 1B flags [v0x02+] + 2B LE length + data) + 2-byte CRC16-CCITT trailer. Current write version: v0x02. v0x01 blobs (3-byte entry overhead) load cleanly with flags=0. Unknown persist_id entries are skipped without error. Truncated/corrupt entry → -EBADMSG immediately. **v0x01 framing (no flags byte) described in initial design; superseded by v0x02 per DECISION-ST-11.** | §1.3, §1.7, §6 |
| **DECISION-ST-4** | Partial-load semantics: **continue-on-deserialize-error**. If a service's `deserialize()` returns an error, `partial_load_count++` + `STATS_ERROR()` + continue to the next TLV entry. Services that successfully deserialize keep their state. After all entries, return the last deserialize error (or 0 if all succeeded). **Justification**: primary path is provisioning from compiled-in defaults; stopping at the first service failure leaves all subsequent services unprovisioned. The per-service boundary is recoverable; TLV-structure corruption (entry_len overrun) is not, so that stops parsing immediately with -EBADMSG. | §3, §6 |
| **DECISION-ST-5** | Tag-to-slot mapping: for the load stub, `tag` is a null-terminated C string looked up by `strcmp` in the compiled-in table in `nfc_store_default_cards.h`. First match wins; no hash, no enum. Max tag length: `CONFIG_NFC_STORE_MAX_TAG_LEN` (default 15 chars + NUL = 16). The save stub does not perform any lookup — it emits the blob to the shell unconditionally. A future NVS backend would derive a slot ID from the tag string externally to this module. | §1.3, §6 |
| **DECISION-ST-6** | CRC algorithm: CRC16-CCITT (polynomial 0x1021, initial 0xFFFF, no final XOR). Available from Zephyr `<zephyr/sys/crc.h>` as `crc16_ccitt()`. Chosen for smallest code footprint and Zephyr native availability; no external dep. | §1.3, §6 |
| **DECISION-ST-7** | Staging buffer `s_staging_buf[CONFIG_NFC_STORE_BLOB_SIZE]` is file-static BSS (no stack-local, no dynamic allocation — MISRA Dir 4.12, creed §13). `CONFIG_NFC_STORE_BLOB_SIZE` default = 12288. A `BUILD_ASSERT` guards that the configured size ≥ `NFC_STORE_MIN_BUF_NEEDED` (IS_ENABLED-conditional arithmetic; see §1.4). When DeSFire is disabled, the effective worst-case drops to ~1135 bytes — the ifdef arithmetic reflects this automatically. | §1.4, §6 |
| **DECISION-ST-8** | Shell `store save <tag>` and `store load <tag>` route through `nfc_stack_save(tag)` / `nfc_stack_load(tag)` — NOT through `nfc_store_save/load` directly. This preserves the `-EBUSY` guard that lives in `nfc_stack`. Shell commands include config/stats/state per CONVENTIONS §10. | §2, §5, §6 |
| **DECISION-ST-9** | `nfc_store_save()` returns `-ENOMEM` if the staging buffer would overflow even after BUILD_ASSERT (defensive check at runtime in case of CONFIG mismatch at link time across object units). DECISION: return `-ENOMEM` from `nfc_store_save` on buffer overflow rather than asserting, as MISRA forbids `__ASSERT` on non-debug paths. | §3 |
| **DECISION-ST-10** | `nfc_store_register_save_cb(NULL, …)` / `nfc_store_register_load_cb(NULL, …)` restores the default stub (save → shell, load → compiled-in header). CONVENTIONS §4: NULL accepted to clear. | §2, §3 |
| **DECISION-ST-11** | Blob format v0x02 adds a `flags:u8` byte between `persist_id` and `entry_len` in each TLV entry, making `NFC_STORE_ENTRY_OVERHEAD` = 4. Decoder is version-aware: v0x01 blobs use 3-byte entry overhead (flags = 0); v0x02+ blobs use 4-byte overhead. Encoder always writes v0x02. Flag bits: `NFC_STORE_ENTRY_FLAG_READER_CAPTURED` (bit 0), `_HAND_AUTHORED` (bit 1), `_EMULATION_COMPLETE` (bit 2), `_READ_ONLY_PARTIAL` (bit 3); bits 4–7 reserved = 0. Size impact: +5 bytes worst-case (11380 vs 11375). | §1.4, §1.7 |
| **DECISION-ST-12** | **[DECISION — flag for cross-review]** Live-commit concurrency: `nfc_store_on_dirty()` runs `@caller_wq` (from `nfc_work_q`). `s_staging_buf` is shared with `nfc_store_save/load()`, but those require NOT STARTED (enforced by `nfc_stack` `-EBUSY`), making the two paths mutually exclusive via the existing stack state machine. Rapid back-to-back mutations serialise naturally on the single WQ thread. Flag for cross-review if any future path calls `nfc_store_on_dirty` from ISR or a second thread. | §2.6, §5.2 |
| **DECISION-ST-13** | Flags producer = static per-persist_id table `k_persist_flags[]` in `nfc_store.c`; services do not report flags; vtable unchanged. Hand-provisioned services (NDEF/Ultralight/EMV/DeSFire data) carry `HAND_AUTHORED \| EMULATION_COMPLETE`. Reader-captured NDEF carries `READER_CAPTURED \| EMULATION_COMPLETE` (full clone — spec §1.1). DeSFire/Aliro reader captures carry `READER_CAPTURED \| READ_ONLY_PARTIAL`. | §3, §2.4, Task 16 |
| **DECISION-ST-14** | **[LOCKED 2026-06-13]** NDEF live persist is **required** for `NFC_PROFILE_NDEF`: `nfc_stack.c` calls `nfc_store_on_dirty(ndef_service_get(), active_tag)` after each successful NDEF-file UPDATE BINARY (stack tracks `active_tag` from last `nfc_stack_load()`). Replaces the prior "post-Wave-5 amendment / app-discretionary" wording. Ultralight profile is out of scope (`DECISION-UL-3`). | §2.6, §5.2 |

---

## 1. Types and Constants

### 1.1 `nfc_store_config_t` — Frozen After `init()`

```c
/**
 * @brief nfc_store configuration. Frozen after nfc_store_init().
 *
 * NULL → NFC_STORE_CONFIG_DEFAULT (all Kconfig defaults).
 */
typedef struct {
    /**
     * Maximum tag string length including the NUL terminator.
     * 0 → CONFIG_NFC_STORE_MAX_TAG_LEN (default 16).
     * Tags longer than this are rejected with -EINVAL.
     */
    uint8_t max_tag_len;
} nfc_store_config_t;

/** Apply when cfg == NULL or max_tag_len == 0. */
#define NFC_STORE_CONFIG_DEFAULT \
    { .max_tag_len = 0U }
```

> **DECISION-ST-2**: The config carries only `max_tag_len`. All buffer and blob sizing is Kconfig. The 0-means-default pattern matches every other layer in this stack.

### 1.2 `nfc_store_stats_t`

```c
typedef struct {
    /* Required by CONVENTIONS §2 / STATS_API_DESIGN */
    uint32_t error_count;           /**< Incremented by STATS_ERROR() on every non-fatal failure. */
    int32_t  last_error_code;       /**< Most recent negative errno stored by STATS_ERROR(). */

    /* Operation counters */
    uint32_t save_count;            /**< Successful nfc_store_save() completions. */
    uint32_t load_count;            /**< Successful nfc_store_load() completions (all entries OK). */

    /* Skip / skip-due-to-null-hook */
    uint32_t serialize_skip_count;  /**< Services skipped: NULL serialize hook, persist_id==0,
                                     *   or serialize() returned -ENOTSUP. */
    uint32_t deserialize_skip_count;/**< TLV entries whose persist_id matched no service in svcs[].
                                     *   These are silently skipped per the skip-unknown rule. */

    /* Integrity failures */
    uint32_t corrupt_blob_count;    /**< Blobs rejected: bad magic, wrong version, CRC mismatch,
                                     *   or a TLV entry_len exceeds the remaining payload. */
    uint32_t partial_load_count;    /**< Loads where ≥1 service's deserialize() returned an error
                                     *   but parsing continued (per DECISION-ST-4). */

    /* Live write-through */
    uint32_t live_commit_count;     /**< Successful nfc_store_on_dirty() commits (Task 17).
                                     *   Does not increment save_count. */
} nfc_store_stats_t;
```

### 1.3 `nfc_store_state_t`

```c
typedef enum {
    NFC_STORE_STATE_UNINITIALIZED = 0, /**< Before nfc_store_init(). Safe to call getters. */
    NFC_STORE_STATE_INITIALIZED   = 1, /**< After nfc_store_init(); ready for save/load. */
} nfc_store_state_t;
```

State storage: **Pattern A** — plain `static nfc_store_state_t` (caller thread only, never accessed from ISR or WQ).

### 1.4 Blob Format Constants

```c
/** Blob header: [magic0:u8][magic1:u8][version:u8][reserved:u8][payload_len:u16_LE] */
#define NFC_STORE_BLOB_MAGIC_0       0x4EU   /**< 'N' */
#define NFC_STORE_BLOB_MAGIC_1       0x46U   /**< 'F' */
#define NFC_STORE_BLOB_VERSION_V1   0x01U   /**< Initial format: 3-byte TLV entry overhead (no completeness flags). */
#define NFC_STORE_BLOB_VERSION      0x02U   /**< Current write version: adds flags:u8 in entry header (DECISION-ST-11). */
#define NFC_STORE_BLOB_HDR_SIZE      6U      /**< bytes 0..5 */
#define NFC_STORE_BLOB_CRC_SIZE      2U      /**< CRC16 trailer */
#define NFC_STORE_ENVELOPE_OVERHEAD  (NFC_STORE_BLOB_HDR_SIZE + NFC_STORE_BLOB_CRC_SIZE)  /* 8U */

/** Per-TLV entry overhead v0x01 (decode-only): [persist_id:u8][entry_len:u16_LE] */
#define NFC_STORE_ENTRY_OVERHEAD_V1  3U
/** Per-TLV entry overhead v0x02+ (encode + decode): [persist_id:u8][flags:u8][entry_len:u16_LE] */
#define NFC_STORE_ENTRY_OVERHEAD     4U

/**
 * Minimum staging buffer needed for compiled-in services.
 *
 * IS_ENABLED() expands to 1 (true) or 0 (false) at compile time — legal in
 * constant arithmetic (DEV-ZEP-017: use of Zephyr IS_ENABLED macro).
 * Each term adds MAX_SERIALIZED + entry envelope only when that service is
 * compiled in. When DeSFire is disabled the worst-case drops from ~11.4 KB to
 * ~1.1 KB, automatically reflected here.
 */
#define NFC_STORE_MIN_BUF_NEEDED                                                           \
    ((size_t)NFC_STORE_ENVELOPE_OVERHEAD                                                   \
    + (IS_ENABLED(CONFIG_NFC_SERVICE_NDEF)      ? (NDEF_SERVICE_MAX_SERIALIZED      + (size_t)NFC_STORE_ENTRY_OVERHEAD) : 0U) \
    + (IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)   ? (DESFIRE_SERVICE_MAX_SERIALIZED   + (size_t)NFC_STORE_ENTRY_OVERHEAD) : 0U) \
    + (IS_ENABLED(CONFIG_NFC_SERVICE_ULTRALIGHT)? (ULTRALIGHT_SERVICE_MAX_SERIALIZED+ (size_t)NFC_STORE_ENTRY_OVERHEAD) : 0U) \
    + (IS_ENABLED(CONFIG_NFC_SERVICE_EMV)       ? (EMV_SERVICE_MAX_SERIALIZED       + (size_t)NFC_STORE_ENTRY_OVERHEAD) : 0U) \
    + (IS_ENABLED(CONFIG_NFC_SERVICE_ALIRO)     ? (ALIRO_SERVICE_MAX_SERIALIZED     + (size_t)NFC_STORE_ENTRY_OVERHEAD) : 0U))
```

> **Sizing verification (all-services default):**
> The numbers below assume **`NFC_NDEF_MAX_SIZE=512`** (NDEF_SERVICE_MAX_SERIALIZED=514).
> At wave4 defaults (`NFC_NDEF_MAX_SIZE=256`), `NDEF_SERVICE_MAX_SERIALIZED=258` and the
> total drops to ≈ 11124 bytes — still well within the 12288 default. BUILD_ASSERT is the
> real guard; the values below are illustrative for the 512-byte NDEF case.
>
> `8 + (514+4) + (10240+4) + (256+4) + (214+4) + (128+4) = 8 + 518 + 10244 + 260 + 218 + 132 = 11380 bytes`  _(v0x02: 4-byte entry overhead; +5 bytes vs v0x01)_
> Default `CONFIG_NFC_STORE_BLOB_SIZE = 12288` provides 913 bytes of headroom (512-NDEF case). BUILD_ASSERT enforces this relationship at compile time.

```c
BUILD_ASSERT(
    (size_t)CONFIG_NFC_STORE_BLOB_SIZE >= NFC_STORE_MIN_BUF_NEEDED,
    "CONFIG_NFC_STORE_BLOB_SIZE too small for compiled-in services' MAX_SERIALIZED sum"
);
```

### 1.5 Tag Constant

```c
/** Effective max tag length (including NUL). Resolved in nfc_store_init(). */
#define NFC_STORE_MAX_TAG_LEN_DEFAULT  16U   /* CONFIG_NFC_STORE_MAX_TAG_LEN default */
```

### 1.6 Persist-ID Table (from wave3-router §1.1 — do NOT renumber)

```c
/* Imported from router/service.h — listed here for visibility only. */
/* NFC_PERSIST_ID_NDEF        0x01U */
/* NFC_PERSIST_ID_DESFIRE     0x02U */
/* NFC_PERSIST_ID_ULTRALIGHT  0x03U */
/* NFC_PERSIST_ID_EMV         0x04U */
/* NFC_PERSIST_ID_ALIRO       0x05U */
```

The store must NOT redefine these — it includes `router/service.h` and uses the locked macros. persist_id 0x00 means "not persistable" (service with 0 persist_id is silently skipped).


### 1.7 Capture-Completeness Flags

Each TLV entry in a v0x02+ blob carries a `flags:u8` byte (byte 1 of the 4-byte entry header,
between `persist_id` and `entry_len`) that records how the card data was obtained and what
the player can faithfully replay from it. Flags are OR-combined; 0x00 = unspecified.

```c
/* Completeness flags — stored in the TLV entry flags byte (v0x02+).
 * Entry header layout: [persist_id:u8][flags:u8][entry_len:u16_LE][data]
 */
#define NFC_STORE_ENTRY_FLAG_READER_CAPTURED    BIT(0U)  /**< Data obtained by reading a physical card. */
#define NFC_STORE_ENTRY_FLAG_HAND_AUTHORED      BIT(1U)  /**< Data created manually / synthetically. */
#define NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE BIT(2U)  /**< Card is fully emulatable (all secrets available). */
#define NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL  BIT(3U)  /**< Card read-but-not-key-cloneable (e.g. DeSFire/Aliro). */
                                                          /**< Bits 4–7: reserved, must be 0 on write. */
```

**Backward compatibility:** The decoder checks the blob version byte before parsing entries.
If `version == NFC_STORE_BLOB_VERSION_V1`, the entry header is 3 bytes (no flags byte);
all loaded entries have `flags = 0x00`. If `version == NFC_STORE_BLOB_VERSION`, the entry
header is 4 bytes. Version bytes other than 0x01 and 0x02 → `-EBADMSG` + `corrupt_blob_count++`.

**Usage guidance:** Protocol listeners that support writable emulation should set
`NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE` when serializing. **NDEF clones** (reader
role capture) carry `READER_CAPTURED | EMULATION_COMPLETE` — full writable replay
(spec §1.1). Authenticated cards (DeSFire, Aliro) captured via the reader role
carry `READER_CAPTURED | READ_ONLY_PARTIAL` — the player knows it can respond to
reads but cannot replicate the auth secrets.
---

## 2. Public API

All declarations in `src/nfc/store/nfc_store.h`. Header guard: `SRC_NFC_STORE_NFC_STORE_H`. Includes: `<stdint.h>`, `<stddef.h>`, `"router/service.h"`.

### 2.1 Callback Typedefs (CONVENTIONS §4: suffix `_fn`)

```c
/**
 * @brief Save-sink callback: receives the serialized blob and persists it.
 *
 * Default stub: emits hex lines to shell under the sentinel
 * "@@NFCDUMP@@ <tag>". Returning non-zero propagates as -EIO from
 * nfc_store_save().
 *
 * @param tag      Null-terminated slot identifier (max NFC_STORE_MAX_TAG_LEN_DEFAULT bytes).
 * @param blob     Packed TLV blob (versioned header + entries + CRC16 trailer).
 * @param len      Byte length of blob.
 * @param user_ctx Registered context pointer.
 * @retval  0      Success.
 * @retval <0      Error; propagated to nfc_store_save() caller.
 * @caller_sync
 */
typedef int (*nfc_store_save_fn)(const char     *tag,
                                  const uint8_t  *blob,
                                  size_t          len,
                                  void           *user_ctx);

/**
 * @brief Load-source callback: fetches the serialized blob for a tag.
 *
 * Default stub: looks up tag in the compiled-in table in
 * nfc_store_default_cards.h and copies the bytes into out.
 *
 * @param tag      Null-terminated slot identifier.
 * @param out      Destination buffer; size guaranteed = CONFIG_NFC_STORE_BLOB_SIZE.
 * @param max      Capacity of out in bytes (= CONFIG_NFC_STORE_BLOB_SIZE).
 * @param out_len  On return: actual blob length in bytes. Must not be NULL.
 * @param user_ctx Registered context pointer.
 * @retval  0      Success; *out_len set.
 * @retval -ENOENT No blob found for tag.
 * @retval <0      Other error; propagated to nfc_store_load() caller.
 * @caller_sync
 */
typedef int (*nfc_store_load_fn)(const char *tag,
                                  uint8_t    *out,
                                  size_t      max,
                                  size_t     *out_len,
                                  void       *user_ctx);
```

### 2.2 Lifecycle

```c
/**
 * @brief Initialize the store module.
 *
 * Resets stats, resolves config defaults, installs default stubs.
 * Idempotent protection: returns -EALREADY if already INITIALIZED.
 *
 * @param cfg  Module configuration. NULL → NFC_STORE_CONFIG_DEFAULT.
 * @retval  0        Success.
 * @retval -EALREADY Already initialized.
 * @caller_sync
 */
int nfc_store_init(const nfc_store_config_t *cfg);

/**
 * @brief Shut down the store module.
 *
 * Resets state to UNINITIALIZED and clears callbacks back to default stubs.
 * Always returns 0 (no resource to release — static buffer only).
 *
 * @retval 0  Always.
 * @caller_sync
 */
int nfc_store_shutdown(void);
```

### 2.3 Callback Registration (CONVENTIONS §4)

```c
/**
 * @brief Register the save-sink callback.
 *
 * NULL fn → restore default save stub (shell hex dump).
 * May be called after init() and at any time before a save is in progress
 * (the store has no in-progress save concept — @caller_sync).
 *
 * @param fn       Save callback; NULL restores default stub.
 * @param user_ctx Passed to fn on each call.
 * @retval  0        Success.
 * @retval -ENODEV   Called before nfc_store_init().
 * @caller_sync
 */
int nfc_store_register_save_cb(nfc_store_save_fn fn, void *user_ctx);

/**
 * @brief Register the load-source callback.
 *
 * NULL fn → restore default load stub (compiled-in header lookup).
 *
 * @param fn       Load callback; NULL restores default stub.
 * @param user_ctx Passed to fn on each call.
 * @retval  0        Success.
 * @retval -ENODEV   Called before nfc_store_init().
 * @caller_sync
 */
int nfc_store_register_load_cb(nfc_store_load_fn fn, void *user_ctx);
```

### 2.4 Core Operations

```c
/**
 * @brief Serialize services and send to the save callback.
 *
 * For each svc in svcs[0..n): calls svc->serialize(), frames the result
 * as a TLV entry keyed by svc->persist_id, and packs into s_staging_buf.
 * Services with NULL serialize hook or persist_id==0 are silently skipped
 * (serialize_skip_count++). serialize() returning -ENOTSUP is also skipped.
 * The full blob (versioned header + TLV entries + CRC16 trailer) is passed
 * to the registered save callback.
 *
 * Called from nfc_stack_save(). The stack's -EBUSY guard (STARTED) ensures
 * this is only called while quiesced — no concurrency with nfc_work_q.
 *
 * @param tag   Null-terminated slot identifier; must be <= s_config.max_tag_len.
 * @param svcs  Array of n service pointers. No element may be NULL.
 * @param n     Number of entries in svcs.
 * @retval  0        Success.
 * @retval -ENODEV   Not initialized.
 * @retval -EINVAL   tag is NULL, or tag length > s_config.max_tag_len, or
 *                   n > 0 and svcs is NULL.
 * @retval -ENOMEM   Staging buffer overflow (BUILD_ASSERT should prevent this).
 * @retval -EIO      serialize() returned a non-zero, non-ENOTSUP error, or the
 *                   save callback returned an error.
 * @caller_sync
 */
int nfc_store_save(const char *tag, const nfc_service_t *const *svcs, size_t n);

/**
 * @brief Load blob via the load callback and scatter to service deserializers.
 *
 * Fetches blob via the registered load callback, validates the versioned
 * header and CRC16 trailer, then iterates TLV entries. Each entry is matched
 * to a service by persist_id. Unmatched entries are skipped
 * (deserialize_skip_count++). Per-service deserialize errors are counted but
 * parsing continues to the next entry (DECISION-ST-4). A TLV entry whose
 * declared length exceeds the remaining payload causes -EBADMSG immediately.
 *
 * @param tag   Null-terminated slot identifier.
 * @param svcs  Array of n service pointers. No element may be NULL.
 * @param n     Number of entries in svcs.
 * @retval  0        Success; all entries loaded without error.
 * @retval -ENODEV   Not initialized.
 * @retval -EINVAL   tag is NULL, or tag length > s_config.max_tag_len, or
 *                   n > 0 and svcs is NULL.
 * @retval -EIO      Load callback returned an error.
 * @retval -ENOENT   Load callback returned -ENOENT (tag not found).
 * @retval -EBADMSG  Blob corrupt: bad magic, wrong version, CRC mismatch, or
 *                   TLV entry_len overflows remaining payload. Also returned
 *                   when any deserialize() returned an error (DECISION-ST-4:
 *                   partial load has occurred; partial_load_count incremented).
 * @caller_sync
 */
int nfc_store_load(const char *tag, const nfc_service_t *const *svcs, size_t n);
```

### 2.5 Module Contract Getters (CONVENTIONS §2)

```c
/**
 * @retval Non-NULL always. File-static backing; safe before init().
 * @caller_sync
 */
const nfc_store_config_t *nfc_store_get_config(void);

/**
 * @param out  Destination; must not be NULL.
 * @retval  0       Success (copy-out under spinlock).
 * @retval -EINVAL  out is NULL.
 * @caller_sync
 */
int nfc_store_get_stats(nfc_store_stats_t *out);

/**
 * @retval nfc_store_state_t — never fails.
 * @caller_sync
 */
nfc_store_state_t nfc_store_get_state(void);
```


### 2.6 Live-Persist Hook

> **LOCKED — 2026-06-13 (DECISION-ST-14).** The hook API
> (`nfc_store_on_dirty` / `nfc_store_register_commit_cb`) is implemented in Wave 6.
> For **`NFC_PROFILE_NDEF`**, `nfc_stack.c` **must** call `nfc_store_on_dirty()`
> after each successful NDEF-file `UPDATE BINARY` while emulating under an active
> load tag (`active_tag` set by last successful `nfc_stack_load(tag)`). The ndef
> service signals the stack via a callback registered in `nfc_stack.c` — no
> service→store upward include (CONVENTIONS §3). Ultralight profile is unchanged
> (`DECISION-UL-3` / spec §1.1). Default commit stub remains no-op until a real
> backend is registered (DECISION-ST-1).

A fine-grained, per-mutation commit path distinct from the full save/load.
Called by the card role after any writable-card mutation (e.g. NDEF UPDATE_BINARY).
The `-EBUSY` quiescence rule does **not** apply here — this fires **while** the stack is STARTED.

```c
/**
 * @brief Live write-through commit callback.
 *
 * Invoked by nfc_store_on_dirty() after a writable-card mutation during emulation.
 * The -EBUSY guard (nfc_stack NOT STARTED) does NOT apply to this path.
 *
 * @param svc      Service whose data model mutated.
 * @param tag      Active tag name the card was loaded under.
 * @param user_ctx Registered context pointer.
 * @retval  0      Success.
 * @retval <0      Error; logged via STATS_ERROR; not fatal to the caller.
 * @caller_wq      Called from nfc_work_q context (same as the mutation handler).
 */
typedef int (*nfc_store_commit_fn)(const nfc_service_t *svc,
                                    const char          *tag,
                                    void                *user_ctx);

/**
 * @brief Register the live write-through commit callback.
 *
 * NULL fn → restore default no-op stub (consistent with DECISION-ST-1: stub seam only).
 *
 * @param fn       Commit callback; NULL restores no-op stub.
 * @param user_ctx Passed to fn on each call.
 * @retval  0        Success.
 * @retval -ENODEV   Called before nfc_store_init().
 * @caller_sync
 */
int nfc_store_register_commit_cb(nfc_store_commit_fn fn, void *user_ctx);

/**
 * @brief Signal that a writable card service has mutated; trigger live commit.
 *
 * Called by the card role (e.g. NDEF service after UPDATE_BINARY). If a commit
 * callback is registered, serializes the single mutated service and invokes it.
 * Default stub is a no-op (DECISION-ST-1: stub seam; real NVS write is a callback swap).
 *
 * Does NOT use the full staging buffer path — it operates on a service-sized
 * slice of s_staging_buf. See DECISION-ST-12 for concurrency analysis.
 *
 * @param svc  Service that mutated. persist_id must not be 0.
 * @param tag  Active tag name. Must not be NULL.
 * @retval  0        Success or no callback registered (no-op stub).
 * @retval -ENODEV   Not initialized.
 * @retval -EINVAL   svc is NULL, svc->persist_id == 0, or tag is NULL.
 * @retval <0        Error from commit callback (logged; returned to caller).
 * @caller_wq        Must only be called from nfc_work_q context (DECISION-ST-12).
 */
int nfc_store_on_dirty(const nfc_service_t *svc, const char *tag);
```
---

## 3. Contracts

### `nfc_store_init(cfg)` — Sequence and Guards

**Pre:** `@caller_sync`.

```
1. if (s_state == NFC_STORE_STATE_INITIALIZED) → return -EALREADY
   [before STATS_RESET — a rejected double-init must not wipe live stats]
2. STATS_RESET(s_stats)
   [first statement after the -EALREADY guard, per CONVENTIONS §6]
3. s_config = (cfg != NULL) ? *cfg : NFC_STORE_CONFIG_DEFAULT
4. if (s_config.max_tag_len == 0U):
       s_config.max_tag_len = (uint8_t)CONFIG_NFC_STORE_MAX_TAG_LEN
5. Install default callbacks:
       s_save_fn  = nfc_store_save_stub;   s_save_ctx = NULL;
       s_load_fn  = nfc_store_load_stub;   s_load_ctx = NULL;
6. s_state = NFC_STORE_STATE_INITIALIZED
7. return 0
```

| Input condition | Action |
|-----------------|--------|
| `s_state == INITIALIZED` | Return `-EALREADY` (no STATS_RESET) |
| `cfg == NULL` | Use `NFC_STORE_CONFIG_DEFAULT` |
| `cfg->max_tag_len == 0` | Resolve to `CONFIG_NFC_STORE_MAX_TAG_LEN` |

### `nfc_store_shutdown()` — Sequence

```
1. s_save_fn = nfc_store_save_stub; s_save_ctx = NULL;
   s_load_fn = nfc_store_load_stub; s_load_ctx = NULL;
2. s_state = NFC_STORE_STATE_UNINITIALIZED
3. return 0
```

Always returns 0. Legal from UNINITIALIZED (idempotent). Stats are NOT reset — the shutdown path must preserve diagnostic history.

### `nfc_store_register_save_cb(fn, user_ctx)` / `nfc_store_register_load_cb(fn, user_ctx)`

| Condition | Action |
|-----------|--------|
| `s_state == UNINITIALIZED` | Return `-ENODEV` |
| `fn == NULL` | Restore default stub; `ctx` ignored |
| `fn != NULL` | Install `fn` and `user_ctx` |
| `s_state == INITIALIZED` | Always accept (no STARTED state in store) |

Returns 0 on success.

### `nfc_store_save(tag, svcs, n)` — Full Decision Table

**Pre:** `@caller_sync`. Called from `nfc_stack_save()` which has already enforced the `-EBUSY` guard. The store may assume quiescence — `nfc_work_q` is not dispatching APDUs to services.

```
Guard checks:
  if (s_state == UNINITIALIZED) → return -ENODEV
  if (tag == NULL) → return -EINVAL
  if (strnlen(tag, s_config.max_tag_len) == s_config.max_tag_len) → return -EINVAL
    [tag not null-terminated within max_tag_len; conservative check]
  if (n > 0U && svcs == NULL) → return -EINVAL

Blob construction in s_staging_buf:
  write_pos = 0
  write header stub (6 bytes: magic, version, reserved, payload_len=0):
      s_staging_buf[0] = NFC_STORE_BLOB_MAGIC_0  (0x4E)
      s_staging_buf[1] = NFC_STORE_BLOB_MAGIC_1  (0x46)
      s_staging_buf[2] = NFC_STORE_BLOB_VERSION  (0x02)
      s_staging_buf[3] = 0x00U                   (reserved)
      s_staging_buf[4..5] = 0x00U                (payload_len — filled below)
  write_pos = NFC_STORE_BLOB_HDR_SIZE  (6)
  payload_start = write_pos

  for i in [0..n):
      svc = svcs[i]
      if (svc->serialize == NULL || svc->persist_id == 0U):
          STATS_INC(&s_stats_lock, s_stats, serialize_skip_count)
          continue
      remaining = CONFIG_NFC_STORE_BLOB_SIZE - write_pos - NFC_STORE_BLOB_CRC_SIZE
      if (remaining < NFC_STORE_ENTRY_OVERHEAD):
          STATS_ERROR(&s_stats_lock, s_stats, -ENOMEM)
          return -ENOMEM    [DECISION-ST-9]
      entry_max = remaining - NFC_STORE_ENTRY_OVERHEAD
      entry_ptr = &s_staging_buf[write_pos + NFC_STORE_ENTRY_OVERHEAD]
      rc = svc->serialize(entry_ptr, entry_max, &entry_len, svc->user_ctx)
      if (rc == -ENOTSUP):
          STATS_INC(&s_stats_lock, s_stats, serialize_skip_count)
          continue
      if (rc != 0):
          STATS_ERROR(&s_stats_lock, s_stats, rc)
          return -EIO
      s_staging_buf[write_pos]     = svc->persist_id
      s_staging_buf[write_pos + 1] = k_persist_flags[svc->persist_id]          [flags from static table — DECISION-ST-13]
      s_staging_buf[write_pos + 2] = (uint8_t)(entry_len & 0xFFU)               [LE low]
      s_staging_buf[write_pos + 3] = (uint8_t)((entry_len >> 8U) & 0xFFU)       [LE high]
      write_pos += NFC_STORE_ENTRY_OVERHEAD + entry_len

  payload_len = (uint16_t)(write_pos - payload_start)
  s_staging_buf[4] = (uint8_t)(payload_len & 0xFFU)        [LE low byte]
  s_staging_buf[5] = (uint8_t)((payload_len >> 8U) & 0xFFU)[LE high byte]

  crc = crc16_ccitt(0xFFFFU, s_staging_buf, write_pos)
  s_staging_buf[write_pos]     = (uint8_t)(crc & 0xFFU)
  s_staging_buf[write_pos + 1] = (uint8_t)((crc >> 8U) & 0xFFU)
  total_len = write_pos + NFC_STORE_BLOB_CRC_SIZE

Call save callback:
  rc = s_save_fn(tag, s_staging_buf, total_len, s_save_ctx)
  if (rc != 0):
      STATS_ERROR(&s_stats_lock, s_stats, rc)
      return -EIO

STATS_INC(&s_stats_lock, s_stats, save_count)
return 0
```

| Error | Condition |
|-------|-----------|
| `-ENODEV` | UNINITIALIZED |
| `-EINVAL` | NULL tag; tag overlong; n>0 and NULL svcs |
| `-ENOMEM` | Staging buffer overflow mid-construction (defensive; BUILD_ASSERT prevents in normal builds) |
| `-EIO` | `serialize()` returned non-zero/non-ENOTSUP, or save callback returned non-zero |

### `nfc_store_load(tag, svcs, n)` — Full Decision Table

**Pre:** `@caller_sync`. Quiescence guaranteed by `nfc_stack_load()`'s `-EBUSY` guard.

```
Guard checks: (same as nfc_store_save — ENODEV, EINVAL)

Fetch blob:
  blob_len = 0
  rc = s_load_fn(tag, s_staging_buf, sizeof(s_staging_buf), &blob_len, s_load_ctx)
  if (rc == -ENOENT): return -ENOENT   [propagate tag-not-found directly]
  if (rc != 0): STATS_ERROR(..., rc); return -EIO
  if (blob_len < NFC_STORE_ENVELOPE_OVERHEAD):
      STATS_INC(..., corrupt_blob_count); return -EBADMSG

Validate header:
  if (s_staging_buf[0] != NFC_STORE_BLOB_MAGIC_0 ||
      s_staging_buf[1] != NFC_STORE_BLOB_MAGIC_1):
      STATS_INC(..., corrupt_blob_count); return -EBADMSG
  if (s_staging_buf[2] != NFC_STORE_BLOB_VERSION &&
      s_staging_buf[2] != NFC_STORE_BLOB_VERSION_V1):
      STATS_INC(..., corrupt_blob_count); return -EBADMSG   [unknown version]
  blob_version = s_staging_buf[2]   /* 0x01 or 0x02 */
  payload_len = (uint16_t)s_staging_buf[4] | ((uint16_t)s_staging_buf[5] << 8U)
  expected_total = (size_t)NFC_STORE_BLOB_HDR_SIZE + (size_t)payload_len
                   + (size_t)NFC_STORE_BLOB_CRC_SIZE
  if (expected_total != blob_len):
      STATS_INC(..., corrupt_blob_count); return -EBADMSG

Verify CRC16:
  stored_crc = (uint16_t)s_staging_buf[blob_len - 2U]
             | ((uint16_t)s_staging_buf[blob_len - 1U] << 8U)
  computed_crc = crc16_ccitt(0xFFFFU, s_staging_buf, blob_len - NFC_STORE_BLOB_CRC_SIZE)
  if (stored_crc != computed_crc):
      STATS_INC(..., corrupt_blob_count); return -EBADMSG

Parse TLV entries:
  parse_pos = NFC_STORE_BLOB_HDR_SIZE
  end_pos   = NFC_STORE_BLOB_HDR_SIZE + payload_len
  last_deser_rc = 0
  entry_overhead = (blob_version == NFC_STORE_BLOB_VERSION_V1)
                   ? NFC_STORE_ENTRY_OVERHEAD_V1   /* 3 bytes: pid + len_lo + len_hi */
                   : NFC_STORE_ENTRY_OVERHEAD       /* 4 bytes: pid + flags + len_lo + len_hi (v0x02+) */

  while (parse_pos < end_pos):
      if (end_pos - parse_pos < entry_overhead):
          STATS_INC(..., corrupt_blob_count); return -EBADMSG  [truncated entry header]
      pid      = s_staging_buf[parse_pos]
      if (blob_version == NFC_STORE_BLOB_VERSION_V1):
          entry_flags = 0x00U                       /* v0x01: no flags byte */
          elen_lo  = s_staging_buf[parse_pos + 1U]
          elen_hi  = s_staging_buf[parse_pos + 2U]
      else:
          entry_flags = s_staging_buf[parse_pos + 1U]
          elen_lo  = s_staging_buf[parse_pos + 2U]
          elen_hi  = s_staging_buf[parse_pos + 3U]
      entry_len = (uint16_t)elen_lo | ((uint16_t)elen_hi << 8U)
      parse_pos += entry_overhead

      if (entry_len > (end_pos - parse_pos)):
          STATS_INC(..., corrupt_blob_count); return -EBADMSG  [entry_len overflow]

      entry_ptr = &s_staging_buf[parse_pos]

      /* Match pid to a service */
      matched = false
      for j in [0..n):
          if (svcs[j]->persist_id == pid && svcs[j]->deserialize != NULL):
              rc = svcs[j]->deserialize(entry_ptr, entry_len, svcs[j]->user_ctx)
              if (rc != 0):
                  STATS_INC(..., partial_load_count)
                  STATS_ERROR(..., rc)
                  last_deser_rc = rc  [save; parsing continues — DECISION-ST-4]
              matched = true
              break
      if (!matched):
          STATS_INC(..., deserialize_skip_count)

      parse_pos += entry_len  [advance regardless of deserialize result]

  if (last_deser_rc != 0):
      return -EBADMSG   [partial load; DECISION-ST-4]

STATS_INC(..., load_count)
return 0
```

| Error | Condition |
|-------|-----------|
| `-ENODEV` | UNINITIALIZED |
| `-EINVAL` | NULL tag; overlong tag; n>0 and NULL svcs |
| `-EIO` | Load callback returned non-zero/non-ENOENT |
| `-ENOENT` | Load callback returned -ENOENT (tag not found in backing store) |
| `-EBADMSG` | Bad magic, wrong version, CRC mismatch, TLV entry_len overrun, or any `deserialize()` returned error (partial_load_count++) |

---

## 4. Internal State

### 4.1 File-Static Variables (`nfc_store.c`)

```c
/* Lifecycle — Pattern A (plain enum; all access @caller_sync) */
static nfc_store_state_t  s_state  = NFC_STORE_STATE_UNINITIALIZED;

/* Module contract statics */
static nfc_store_config_t s_config = NFC_STORE_CONFIG_DEFAULT;
static nfc_store_stats_t  s_stats;
static struct k_spinlock  s_stats_lock;  /* NOT K_SPINLOCK_DEFINE — unavailable NCS v3.2.4 */

/* Registered callbacks — default stubs installed at init() */
static nfc_store_save_fn  s_save_fn  = nfc_store_save_stub;
static void              *s_save_ctx = NULL;
static nfc_store_load_fn  s_load_fn  = nfc_store_load_stub;
static void              *s_load_ctx = NULL;

/* Static staging buffer — no dynamic allocation (MISRA Dir 4.12, creed §13) */
static uint8_t s_staging_buf[CONFIG_NFC_STORE_BLOB_SIZE];

BUILD_ASSERT(
    (size_t)CONFIG_NFC_STORE_BLOB_SIZE >= NFC_STORE_MIN_BUF_NEEDED,
    "CONFIG_NFC_STORE_BLOB_SIZE too small for compiled-in services' MAX_SERIALIZED sum"
);
```

**RAM cost table:**

| Build configuration | `s_staging_buf` | Remaining statics | Total approx |
|--------------------|-----------------|-------------------|-------------|
| All services enabled (default 12288) | 12288 B | ~100 B | ~12.4 KB |
| DeSFire disabled (worst-case ~1135 B) | up to 12288 B | ~100 B | ~12.4 KB |
| DeSFire disabled + `CONFIG_NFC_STORE_BLOB_SIZE=2048` | 2048 B | ~100 B | ~2.1 KB |

> When DeSFire is not enabled, the user should reduce `CONFIG_NFC_STORE_BLOB_SIZE` to recover RAM. The `NFC_STORE_MIN_BUF_NEEDED` macro computes the true minimum at build time; the BUILD_ASSERT lets the user confirm the tighter setting is valid.

### 4.2 State Machine

```
      nfc_store_init()
UNINITIALIZED ──────────────────► INITIALIZED
      ◄──────────────────────────
      nfc_store_shutdown()
```

Two states only. No STARTED/STOPPED — the store has no running phase; `@caller_sync` throughout. The `-EBUSY` quiescence contract is enforced by `nfc_stack`, not by the store.

### 4.3 Default Stubs (file-static functions; not exported)

**`nfc_store_save_stub`** — emits the blob as a hex dump to the logging system:

```c
static int nfc_store_save_stub(const char *tag, const uint8_t *blob,
                                size_t len, void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    /* Emit sentinel line followed by pairs of hex lines.
     * DEV-ZEP-008: LOG_INF variadic — pre-approved. */
    LOG_INF("@@NFCDUMP@@ %s len=%zu", tag, len);
    for (size_t i = 0U; i < len; i += 16U) {
        /* LOG_HEXDUMP_INF used per Zephyr convention — no custom hex loop needed */
        LOG_HEXDUMP_INF(&blob[i], MIN(16U, len - i), "");
    }
    return 0;
}
```

> Note: `LOG_HEXDUMP_INF` is the idiomatic Zephyr hex-dump log call (DEV-ZEP-008). The `@@NFCDUMP@@` sentinel allows scripted extraction from a terminal session and pasting directly into `nfc_store_default_cards.h`.

**`nfc_store_load_stub`** — looks up `tag` in the compiled-in table:

```c
/* Forward declaration only; implementation detail of nfc_store_default_cards.h */
static int nfc_store_load_stub(const char *tag, uint8_t *out,
                                size_t max, size_t *out_len, void *user_ctx)
{
    ARG_UNUSED(user_ctx);
    /* nfc_store_default_cards_lookup(tag, out, max, out_len) defined in
     * nfc_store_default_cards.h. Returns 0 or -ENOENT. */
    return nfc_store_default_cards_lookup(tag, out, max, out_len);
}
```

### 4.4 `nfc_store_default_cards.h` Format

This file defines the compiled-in card table. The lookup function is `static inline` to avoid link-time duplication:

```c
/* src/nfc/store/nfc_store_default_cards.h
 *
 * Compiled-in card blob table. Add cards here; the store load stub will find
 * them by tag name. Each blob MUST be a valid nfc_store blob (versioned header
 * + TLV entries + CRC16 trailer). Current encoder writes NFC_STORE_BLOB_VERSION
 * 0x02 (4-byte entry overhead; DECISION-ST-11). v0x01 blobs (3-byte overhead,
 * no flags byte) are also accepted by the decoder; flags treated as 0.
 *
 * Usage:
 *   - Run `nfc store save default` while the stack is stopped.
 *   - Copy the @@NFCDUMP@@ shell output (hex bytes) into the table below.
 *   - Rebuild; the blob is provisioned on next nfc_stack_load("default").
 */

#ifndef SRC_NFC_STORE_NFC_STORE_DEFAULT_CARDS_H
#define SRC_NFC_STORE_NFC_STORE_DEFAULT_CARDS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    const char    *tag;
    const uint8_t *blob;
    size_t         len;
} nfc_store_card_entry_t;

/* ── Add compiled-in card blobs below ──────────────────────────────────────── */
/* Example (replace with real blob from @@NFCDUMP@@ output):
 * static const uint8_t s_blob_default[] = { 0x4EU, 0x46U, 0x02U, ... };
 * static const nfc_store_card_entry_t s_cards[] = {
 *     { "default", s_blob_default, sizeof(s_blob_default) },
 * };
 */
static const nfc_store_card_entry_t s_cards[] = {
    /* empty — add blobs here */
};

static inline int nfc_store_default_cards_lookup(const char *tag,
                                                   uint8_t    *out,
                                                   size_t      max,
                                                   size_t     *out_len)
{
    size_t i;
    for (i = 0U; i < ARRAY_SIZE(s_cards); i++) {
        if (strcmp(s_cards[i].tag, tag) == 0) {
            if (s_cards[i].len > max) {
                return -ENOSPC;  /* caller's buffer too small — config error */
            }
            memcpy(out, s_cards[i].blob, s_cards[i].len);
            *out_len = s_cards[i].len;
            return 0;
        }
    }
    return -ENOENT;
}

#endif /* SRC_NFC_STORE_NFC_STORE_DEFAULT_CARDS_H */
```

---

## 5. Integration Points

### 5.1 Down — What the Store Calls

| Call | File | When |
|------|------|------|
| `svc->serialize(out, max, &len, ctx)` | `router/service.h` | During `nfc_store_save()` |
| `svc->deserialize(in, len, ctx)` | `router/service.h` | During `nfc_store_load()` |
| `s_save_fn(tag, blob, len, ctx)` | registered callback | End of `nfc_store_save()` |
| `s_load_fn(tag, out, max, &len, ctx)` | registered callback | Start of `nfc_store_load()` |
| `crc16_ccitt(init, buf, len)` | `<zephyr/sys/crc.h>` | CRC computation |
| `STATS_INC/STATS_ERROR/STATS_COPY_OUT/STATS_RESET` | `src/stats.h` | Stats management |
| `LOG_INF/LOG_HEXDUMP_INF/LOG_ERR` | `<zephyr/logging/log.h>` | Save stub + errors |
| `strnlen/strcmp/memcpy/memset` | `<string.h>` | Tag validation, blob copy |

### 5.2 Up — Who Calls the Store

| Caller | Calls | When |
|--------|-------|------|
| `nfc_stack.c` → `nfc_stack_init()` | `nfc_store_init(NULL)` | Stack initialization (step 10) |
| `nfc_stack.c` → `nfc_stack_shutdown()` | `nfc_store_shutdown()` | Stack teardown |
| `nfc_stack.c` → `nfc_stack_save(tag)` | `nfc_store_save(tag, svcs, n)` | After STARTED guard |
| `nfc_stack.c` → `nfc_stack_load(tag)` | `nfc_store_load(tag, svcs, n)` | After STARTED guard |
| `nfc_store_shell_cmds.c` | `nfc_store_get_config/stats/state()` | Shell `config/stats/state` |
| `nfc_store_shell_cmds.c` | `nfc_stack_save/load(tag)` | Shell `store save/load <tag>` |
| Card role — `nfc_stack.c` (NDEF profile) | `nfc_store_on_dirty(ndef_service_get(), active_tag)` | After successful NDEF-file UPDATE BINARY while STARTED (DECISION-ST-14) |
| `nfc_store_shell_cmds.c` | `nfc_stack_save/load(tag)` (via export/import path) | Shell `store export/import <tag>` |

**Quiescence contract (inherited from nfc_stack):** `nfc_store_save()` and `nfc_store_load()` are only ever called from `nfc_stack_save/load()`, which enforces `s_state == STARTED → return -EBUSY`. The store may therefore assume that when `nfc_store_save/load()` execute, `nfc_work_q` is not concurrently dispatching APDUs to services — no locking between `s_staging_buf` and service data models is required.

### 5.3 Wiring in `nfc_stack.c` (CONVENTIONS §3)

```
nfc_stack_init():
    nfc_store_init(NULL)                 [step 10 — see wave4 §3]
    (no callback registration — default stubs are sufficient;
     a real backend is registered by the application before start)

nfc_stack_shutdown():
    nfc_store_shutdown()                 [step 4 per wave4 §3]
```

No `nfc_store_register_save_cb` / `nfc_store_register_load_cb` calls from `nfc_stack.c` — wiring a custom backend is an application-layer concern (call after `nfc_stack_init`, before `nfc_stack_start`).


### 5.4 Role-Independence of the Store and File Format

The `nfc_store` module and the `.card` file format are **role-independent**.
The card role (NFCT, Wave 1–6) uses them today. The reader role (PN7160/ST25R3916,
future) will produce blobs with the same format after capturing a physical card,
setting `READER_CAPTURED` and, where applicable, `READ_ONLY_PARTIAL` flags.
Existing Kconfig symbols (`NFC_STORE`, `NFC_STORE_BLOB_SIZE`, etc.) are not
scoped to card-only use and remain unchanged when the reader role is introduced.
---

## 6. Implementation Notes

### 6.1 No Real Persistence Backend

> **DECISION-ST-1**: `prj.conf` enables no NVS, ZMS, or Settings subsystem. The spec v2 §12 lists real NVS/LittleFS as out of scope. Wave 6 ships the stub seam: load from compiled-in `.h`, save to shell. A future NVS backend is a one-file addition that registers a different `nfc_store_load_fn`/`nfc_store_save_fn` and calls `nfc_store_register_*_cb()` — nothing else in the stack changes.

### 6.2 Blob Format Rationale

The 6-byte header + TLV + CRC16 design satisfies all constraints:
- **Versioned**: `version` field at byte 2 allows future format evolution; the load path rejects unknown versions with `corrupt_blob_count++` and `-EBADMSG`.
- **Keyed by persist_id**: load matches entries to services by `persist_id`, not array position. Profile composition changes (e.g., adding EMV to a blob that only had NDEF) don't corrupt restores — unknown IDs are skipped.
- **Length-prefixed**: each entry is self-describing; entries can be seeked past without decoding.
- **CRC16**: `crc16_ccitt()` is zero-dependency (Zephyr sys header). Detects corruption, bit-flip, and truncation.

### 6.3 MISRA Compliance Notes

| Issue | Rule | Resolution |
|-------|------|------------|
| `IS_ENABLED()` in `NFC_STORE_MIN_BUF_NEEDED` | Rule 20.1 (macro side-effects) | Pre-approved DEV-ZEP-017: `IS_ENABLED` is a pure arithmetic expansion in constant context |
| `LOG_INF`, `LOG_HEXDUMP_INF` variadic | Rule 16.1 (variadic functions) | Pre-approved DEV-ZEP-008 |
| `shell_print` in shell cmds file | Rule 16.1 | Pre-approved DEV-ZEP-009 |
| `ARRAY_SIZE()` in `nfc_store_default_cards.h` | Rule 20.7 (macro argument expansion) | Pre-approved DEV-ZEP-017 |
| `BUILD_ASSERT()` | Rule 20.1 | Pre-approved DEV-ZEP-015 |
| `crc16_ccitt()` | — | Standard Zephyr API; no MISRA issue |
| `static uint8_t s_staging_buf[...]` | Dir 4.12 (dynamic allocation) | **Compliant**: file-static BSS — no `malloc`/`k_heap_alloc` |
| `static inline` function in `.h` | Rule 5.5 (identifier scope) | `static inline` in a header included once per translation unit; no conflict |
| All variables initialized at declaration | Rule 9.1 | All file-statics have zero-init (BSS) or explicit initializers |
| `switch` with `default` | Rule 16.4 | The store has no `switch` on enums with varying arms; state machine is `if/else` |
| `uint8_t`, `uint16_t`, `uint32_t`, `size_t` | Rule 6.1 | Used throughout; no plain `int` in data fields |
| Bit-shift on `uint8_t`/`uint16_t` | Rule 12.2 (shift count) | All shifts on explicitly-typed unsigned operands; counts < type width |

### 6.4 Threading Annotations Summary

| Function | Annotation | Rationale |
|----------|-----------|-----------|
| `nfc_store_init/shutdown` | `@caller_sync` | Pattern A; no atomic; no ISR path |
| `nfc_store_register_*_cb` | `@caller_sync` | Modifies static pointers; not ISR-safe |
| `nfc_store_save/load` | `@caller_sync` | Long operation; no ISR or WQ context |
| `nfc_store_get_config` | `@caller_sync` | Returns pointer to static; Pattern A |
| `nfc_store_get_stats` | `@caller_sync` | `STATS_COPY_OUT` holds spinlock briefly |
| `nfc_store_get_state` | `@caller_sync` | Plain enum read |
| `nfc_store_save_stub` | `@caller_sync` | Calls LOG_INF (may sleep if buffered) |
| `nfc_store_load_stub` | `@caller_sync` | Reads BSS table (no blocking) |

### 6.5 `crc16_ccitt()` Usage

```c
#include <zephyr/sys/crc.h>
/* Polynomial 0x1021, init value 0xFFFF, final XOR = none.
 * Covers all bytes from start of header to end of TLV payload (excludes the
 * 2-byte CRC trailer itself). */
uint16_t crc = crc16_ccitt(0xFFFFU, s_staging_buf, byte_count);
```

### 6.6 Shell Command Routing (DECISION-ST-8)

Shell commands `store save <tag>` and `store load <tag>` call `nfc_stack_save(tag)` and `nfc_stack_load(tag)` — not `nfc_store_save/load()` directly. This is mandatory to preserve the `-EBUSY` guard in `nfc_stack`. The shell file includes `nfc_stack.h`, not `nfc_store.h`, for the save/load operations. For `config/stats/state` it includes `nfc_store.h`.

### 6.7 `nfc_store_default_cards.h` Workflow

The expected provisioning workflow:
1. Program the device, run `nfc stack init` (or power up).
2. Call `nfc store load default` → returns `-ENOENT` (empty table is fine on first boot; services use their built-in defaults).
3. Optionally, configure card content via service-specific shell commands.
4. Stop the stack (`nfc stack stop`).
5. Run `nfc store save default` → triggers `nfc_stack_save("default")` → stub emits `@@NFCDUMP@@ default` hex lines to the log.
6. Copy hex bytes into `nfc_store_default_cards.h` as `s_blob_default[]`.
7. Rebuild and flash; `nfc store load default` now provisions all services from compiled-in data.

---

## 7. Conventions Compliance

- [x] **Lifecycle matches §2** — Minimal lifecycle: `init` / `shutdown` only. No `start` / `stop` / `deinit`. Shutdown not deinit.
- [x] **`config_t` / `stats_t` / `state_t` + three getters defined** — `nfc_store_config_t`, `nfc_store_stats_t`, `nfc_store_state_t`; `get_config` / `get_stats` / `get_state` all defined.
- [x] **State storage Pattern A** — `static nfc_store_state_t s_state`; plain enum; `@caller_sync` access only.
- [x] **Coupling matches §3** — Store row: `nfc_stack → store (direct call)`, `store → service (vtable serialize/deserialize)`, `store → backend (registered callback)`. No layer `#include`s another solely for callbacks.
- [x] **Callbacks follow §4** — Typedefs end in `_fn`; registration functions guard on UNINITIALIZED (`-ENODEV`); NULL accepted to restore default stub; no ISR delivery; context pointer named `user_ctx`.
- [x] **Buffer model per §5** — No `net_buf` (store is not on the ISR/WQ APDU path). Static outbound staging buffer. No dynamic allocation. Blob passed to save callback as a const pointer (borrowed for callback duration only).
- [x] **Stats recipe per §6** — `static nfc_store_stats_t s_stats` + `static struct k_spinlock s_stats_lock`. `STATS_RESET` as first statement after `-EALREADY` guard. Copy-out getter via `STATS_COPY_OUT`. Named counter for every silent-drop path: `serialize_skip_count`, `deserialize_skip_count`, `corrupt_blob_count`, `partial_load_count`.
- [x] **Threading + annotations per §7** — All functions `@caller_sync`; no ISR or WQ access path; documented in header.
- [x] **Error codes per §9** — `-ENODEV`, `-EALREADY`, `-EINVAL`, `-ENOMEM`, `-EIO`, `-ENOENT`, `-EBADMSG`. No invented codes.
- [x] **Shell per §10** — `nfc_store_shell_cmds.c` separate from core. Exposes `config`/`stats`/`state` + `save <tag>` / `load <tag>`. Save/load route through `nfc_stack_save/load` (not direct store calls — DECISION-ST-8).
- [x] **MISRA notes + DEV-ZEP citations per §11** — See §6.3 table.

---

## 8. Tasks

### Task 1 — Kconfig (`src/nfc/store/Kconfig`) [~3 min]

Create `src/nfc/store/Kconfig`. Note: `NFC_STORE` enable symbol already lives in `src/nfc/Kconfig` (wave4 §1.6) — do NOT redefine it here. This file adds tunables only:

```kconfig
# src/nfc/store/Kconfig — tunables for the nfc_store layer.
# The NFC_STORE enable symbol is defined in src/nfc/Kconfig.

config NFC_STORE_BLOB_SIZE
    int "Store staging buffer size in bytes"
    default 12288
    range 256 65536
    depends on NFC_STORE
    help
      Size of the static staging buffer used by nfc_store_save() and
      nfc_store_load(). Must be large enough to hold the serialized blob
      for the largest enabled profile (NDEF+DeSFire+Ultralight+EMV+Aliro
      worst case ~11.4 KB). A BUILD_ASSERT in nfc_store.c enforces that
      this value >= NFC_STORE_MIN_BUF_NEEDED. When DeSFire is disabled,
      this can safely be reduced to ~2 KB.

config NFC_STORE_MAX_TAG_LEN
    int "Maximum tag name length including NUL terminator"
    default 16
    range 4 64
    depends on NFC_STORE
    help
      Tags longer than this (including NUL) are rejected with -EINVAL.
      Default 16 accommodates typical short names like "default" or
      "aliro_card". The compiled-in stub uses strcmp(); NVS backends may
      use this as a key-string length cap.

config NFC_STORE_LOG_LEVEL
    int "nfc_store log level"
    default 3
    range 0 4
    depends on NFC_STORE
    help
      Logging verbosity for the store module (0=off, 1=error, 2=warn,
      3=info, 4=debug). The save stub emits hex dumps at LOG_LEVEL_INF.
```

Also update `src/nfc/Kconfig`: uncomment the rsource line added as a comment by wave4:

```kconfig
rsource "store/Kconfig"
```

**Commit:** `nfc: store: add Kconfig tunables`

---

### Task 2 — CMakeLists.txt (`src/nfc/store/CMakeLists.txt`) [~2 min]

```cmake
# src/nfc/store/CMakeLists.txt

if(CONFIG_NFC_STORE)
    target_sources(app PRIVATE
        nfc_store.c
        nfc_store_shell_cmds.c
    )
    target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
```

Also add `add_subdirectory(store)` to `src/nfc/CMakeLists.txt` (alongside the existing layer entries from wave4 §5.5).

**Commit:** `nfc: store: add CMakeLists.txt`

---

### Task 3 — Public header (`src/nfc/store/nfc_store.h`) [~5 min]

Create the header with exact content from §1 + §2:
- Header guard `SRC_NFC_STORE_NFC_STORE_H`
- Includes: `<stdint.h>`, `<stddef.h>`, `"router/service.h"`
- `nfc_store_config_t`, `NFC_STORE_CONFIG_DEFAULT`
- `nfc_store_stats_t`, `nfc_store_state_t`
- All §1.4 constants (`NFC_STORE_BLOB_MAGIC_0`, `_1`, `_VERSION`, `_HDR_SIZE`, `_CRC_SIZE`, `_ENVELOPE_OVERHEAD`, `_ENTRY_OVERHEAD`, `NFC_STORE_MAX_TAG_LEN_DEFAULT`)
- `NFC_STORE_MIN_BUF_NEEDED` macro (IS_ENABLED-conditional arithmetic; includes `ndef_service.h`, `desfire_service.h`, etc. headers for `*_MAX_SERIALIZED` — each guarded by `IS_ENABLED`)
- All public function declarations with `@caller_sync` annotations

**Commit:** `nfc: store: add nfc_store.h public header`

---

### Task 4 — Default cards header (`src/nfc/store/nfc_store_default_cards.h`) [~3 min]

Create the file with the format from §4.4: empty `s_cards[]` table and `nfc_store_default_cards_lookup()` `static inline`. Include a comment block explaining the `@@NFCDUMP@@` workflow (§6.7). This file ships empty (no compiled-in blobs); the implementer adds blobs after first provisioning.

**Commit:** `nfc: store: add nfc_store_default_cards.h (empty table)`

---

### Task 5 — Core implementation (`src/nfc/store/nfc_store.c`) — scaffolding [~5 min]

```c
#include "nfc_store.h"
#include "nfc_store_default_cards.h"
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "src/stats.h"

LOG_MODULE_REGISTER(nfc_store, CONFIG_NFC_STORE_LOG_LEVEL);

/* File-static state per §4.1 */
static nfc_store_state_t  s_state      = NFC_STORE_STATE_UNINITIALIZED;
static nfc_store_config_t s_config     = NFC_STORE_CONFIG_DEFAULT;
static nfc_store_stats_t  s_stats;
static struct k_spinlock  s_stats_lock;
static nfc_store_save_fn  s_save_fn    = nfc_store_save_stub;
static void              *s_save_ctx   = NULL;
static nfc_store_load_fn  s_load_fn    = nfc_store_load_stub;
static void              *s_load_ctx   = NULL;
static uint8_t            s_staging_buf[CONFIG_NFC_STORE_BLOB_SIZE];

BUILD_ASSERT(
    (size_t)CONFIG_NFC_STORE_BLOB_SIZE >= NFC_STORE_MIN_BUF_NEEDED,
    "CONFIG_NFC_STORE_BLOB_SIZE too small for compiled-in services"
);

/* Forward declarations for default stubs */
static int nfc_store_save_stub(const char *, const uint8_t *, size_t, void *);
static int nfc_store_load_stub(const char *, uint8_t *, size_t, size_t *, void *);
```

**Commit:** `nfc: store: add nfc_store.c skeleton with static state`

---

### Task 6 — `nfc_store_init()` and `nfc_store_shutdown()` [~4 min]

Implement per §3. Key points:
- `-EALREADY` check before `STATS_RESET` (CONVENTIONS §6)
- Resolve `max_tag_len == 0` → `CONFIG_NFC_STORE_MAX_TAG_LEN`
- Install default stubs
- Shutdown: restore stubs, set UNINITIALIZED, return 0

**TDD:** Write `tests/unit/nfc_store/test_lifecycle.c` first:
```c
/* test_init_ok */       /* nfc_store_init(NULL) → 0; state == INITIALIZED */
/* test_init_ealready */ /* second init → -EALREADY; stats.error_count unchanged */
/* test_shutdown_from_uninit */ /* shutdown before init → 0 (idempotent) */
/* test_getters_before_init */  /* get_config/stats/state safe before init */
```

**Commit:** `nfc: store: implement init/shutdown; add lifecycle tests`

---

### Task 7 — `nfc_store_register_*_cb()` and default stubs [~4 min]

Implement registration per §3. Implement `nfc_store_save_stub` (LOG_HEXDUMP_INF) and `nfc_store_load_stub` (delegates to `nfc_store_default_cards_lookup`).

**TDD:** Add to test file:
```c
/* test_register_before_init */   /* register_save_cb → -ENODEV */
/* test_register_null_restores */ /* register with NULL → restores default stub */
/* test_register_custom_cb */     /* custom cb gets called from nfc_store_save() */
```

**Commit:** `nfc: store: implement callback registration + default stubs`

---

### Task 8 — Blob encoder (pure function) [~5 min]

Extract the blob-building loop from `nfc_store_save()` into a pure helper:

```c
static int blob_encode(const nfc_service_t *const *svcs, size_t n,
                        uint8_t *buf, size_t buf_size, size_t *out_len);
```

This function takes no global state — all inputs and outputs are parameters. It is pure and fully testable without the module being initialized.

**TDD (ztest — pure logic; run on `qemu_cortex_m33` or DK; `native_sim` is Linux-CI-only):**
```c
/* test_encode_single_service */
    /* one service with persist_id=1, serialize writes 4 bytes → blob */
    /* verify header magic, version, payload_len, TLV structure, CRC16 */

/* test_encode_null_serialize_skipped */
    /* service with serialize=NULL → entry absent from blob; skip counter */

/* test_encode_notsup_skipped */
    /* serialize() returns -ENOTSUP → entry absent; serialize_skip_count++ */

/* test_encode_all_services */
    /* 5 services all serializing → correct TLV ordering, CRC valid */

/* test_encode_buffer_overflow */
    /* buf_size barely too small for even 1 entry overhead → -ENOMEM */
```

**Commit:** `nfc: store: implement blob_encode helper + encode tests`

---

### Task 9 — Blob decoder (pure function) [~5 min]

Extract the TLV-parsing loop from `nfc_store_load()` into a pure helper:

```c
static int blob_decode(const uint8_t *blob, size_t blob_len,
                        const nfc_service_t *const *svcs, size_t n,
                        nfc_store_stats_t *stats, struct k_spinlock *lock);
```

The function validates header + CRC, then iterates TLV entries per §3.

**TDD (extensive — this is the critical path):**
```c
/* test_decode_roundtrip */
    /* encode then decode with same svcs[] → all deserializers called with correct data */

/* test_decode_unknown_persist_id */
    /* blob contains entry with pid=0xFF; no service matches → skip; deserialize_skip_count++ */

/* test_decode_bad_magic */
    /* first byte != 0x4E → -EBADMSG; corrupt_blob_count++ */

/* test_decode_wrong_version */
    /* version byte = 0x03 → -EBADMSG */

/* test_decode_crc_mismatch */
    /* flip one byte in TLV payload → CRC fails → -EBADMSG */

/* test_decode_truncated_at_header */
    /* blob_len = 3 (less than header) → -EBADMSG */

/* test_decode_truncated_at_entry_header */
    /* payload_len says 10; blob has only 2 bytes of payload (no room for entry header) → -EBADMSG */

/* test_decode_entry_len_overflow */
    /* entry_len field says 9000; remaining payload is 4 → -EBADMSG */

/* test_decode_partial_load_continues */
    /* entry 1: deserialize succeeds; entry 2: deserialize returns -EBADMSG;
     * → partial_load_count++; return -EBADMSG; entry 1 state IS updated */

/* test_decode_version_mismatch_from_service */
    /* deserialize returns -EBADMSG (service's own version check) →
     * partial_load_count++; continue; return -EBADMSG */

/* test_decode_empty_payload */
    /* payload_len == 0 → no entries; return 0; load_count++ (no error) */
```

**Commit:** `nfc: store: implement blob_decode helper + decode tests (comprehensive)`

---

### Task 10 — `nfc_store_save()` and `nfc_store_load()` integration [~4 min]

Implement the public functions per §3, using `blob_encode` / `blob_decode` internally. Guard checks exactly as in §3 decision tables. Wire to `s_save_fn` / `s_load_fn`. `STATS_INC(save_count)` / `STATS_INC(load_count)` on success.

**TDD (integration, uses mock callbacks):**
```c
/* test_save_uninit */       /* save before init → -ENODEV */
/* test_save_null_tag */     /* NULL tag → -EINVAL */
/* test_save_overlong_tag */ /* tag strlen >= max_tag_len → -EINVAL */
/* test_save_calls_cb */     /* custom save_cb gets the blob; verify tag and len */
/* test_load_enoent */       /* load_cb returns -ENOENT → nfc_store_load returns -ENOENT */
/* test_load_success */      /* end-to-end: save writes blob, load_cb returns it, services deserialized */
```

**Commit:** `nfc: store: implement nfc_store_save/load; add integration tests`

---

### Task 11 — Getter implementations [~2 min]

```c
const nfc_store_config_t *nfc_store_get_config(void) { return &s_config; }

int nfc_store_get_stats(nfc_store_stats_t *out)
{
    return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

nfc_store_state_t nfc_store_get_state(void) { return s_state; }
```

**Commit:** `nfc: store: implement getters`

---

### Task 12 — Shell commands (`src/nfc/store/nfc_store_shell_cmds.c`) [~4 min]

```c
#include <zephyr/shell/shell.h>
#include "nfc_store.h"
#include "nfc_stack.h"   /* for nfc_stack_save / nfc_stack_load */

/* DECISION-ST-8: store save/load route through nfc_stack to preserve -EBUSY guard */

static int cmd_store_save(const struct shell *sh, size_t argc, char **argv)
{
    /* argv[1] = tag */
    int rc = nfc_stack_save(argv[1]);
    shell_print(sh, "store save '%s': %d", argv[1], rc);
    return rc;
}

static int cmd_store_load(const struct shell *sh, size_t argc, char **argv)
{
    int rc = nfc_stack_load(argv[1]);
    shell_print(sh, "store load '%s': %d", argv[1], rc);
    return rc;
}

static int cmd_store_config(const struct shell *sh, size_t argc, char **argv)
{
    const nfc_store_config_t *cfg = nfc_store_get_config();
    shell_print(sh, "max_tag_len: %u", cfg->max_tag_len);
    return 0;
}

static int cmd_store_stats(const struct shell *sh, size_t argc, char **argv)
{
    nfc_store_stats_t st = { 0 };
    (void)nfc_store_get_stats(&st);
    shell_print(sh, "error_count:            %u", st.error_count);
    shell_print(sh, "last_error_code:        %d", st.last_error_code);
    shell_print(sh, "save_count:             %u", st.save_count);
    shell_print(sh, "load_count:             %u", st.load_count);
    shell_print(sh, "serialize_skip_count:   %u", st.serialize_skip_count);
    shell_print(sh, "deserialize_skip_count: %u", st.deserialize_skip_count);
    shell_print(sh, "corrupt_blob_count:     %u", st.corrupt_blob_count);
    shell_print(sh, "partial_load_count:     %u", st.partial_load_count);
    shell_print(sh, "live_commit_count:      %u", st.live_commit_count);
    return 0;
}

static int cmd_store_state(const struct shell *sh, size_t argc, char **argv)
{
    nfc_store_state_t st = nfc_store_get_state();
    shell_print(sh, "state: %s",
                st == NFC_STORE_STATE_INITIALIZED ? "INITIALIZED" : "UNINITIALIZED");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_store_cmds,
    SHELL_CMD_ARG(save,   NULL, "Save card: <tag>",  cmd_store_save,  2, 0),
    SHELL_CMD_ARG(load,   NULL, "Load card: <tag>",  cmd_store_load,  2, 0),
    SHELL_CMD(config, NULL, "Print config",          cmd_store_config),
    SHELL_CMD(stats,  NULL, "Print stats",           cmd_store_stats),
    SHELL_CMD(state,  NULL, "Print state",           cmd_store_state),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(store, &nfc_store_cmds, "NFC store control", NULL);
```

**Commit:** `nfc: store: add shell commands (config/stats/state/save/load)`

#### Task 12 extension — Export/Import shell commands [~3 min]

Add two commands to `nfc_store_shell_cmds.c` for portable card file movement:

**`store export <tag>`** — serialize the active card to a hex string on the shell.
Registers a one-shot export save callback (captures `shell*`), calls
`nfc_stack_save(tag)` (preserving the `-EBUSY` guard), prints the blob as a
contiguous lowercase hex string, then restores the previous save callback.

**`store import <tag> <hex>`** — load a card file from a hex string argument.
Parses `<hex>` via `hex2bin()` (`<zephyr/sys/util.h>`) into
`s_import_buf[CONFIG_NFC_STORE_BLOB_SIZE]` (file-static in shell cmds file;
MISRA Dir 4.12 compliant). Registers a one-shot import load callback that copies
`s_import_buf` into the store output buffer, calls `nfc_stack_load(tag)`,
then restores the previous load callback. If the hex string is malformed
(odd length, non-hex chars) returns `-EINVAL` without calling load.

Update `SHELL_STATIC_SUBCMD_SET_CREATE`:
```c
    SHELL_CMD_ARG(export, NULL, "Export card as hex: <tag>", cmd_store_export, 2, 0),
    SHELL_CMD_ARG(import, NULL, "Import card from hex: <tag> <hex>", cmd_store_import, 3, 0),
```

The round-trip invariant: `store export <tag>` followed by `store import <tag2> <hex>`
and `nfc_stack_load(<tag2>)` must restore an identical service state.

**Commit:** `nfc: store: add export/import shell commands`

---

### Task 13 — Test suite CMakeLists (`tests/unit/nfc_store/`) [~2 min]

```cmake
# tests/unit/nfc_store/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_nfc_store)

target_sources(app PRIVATE
    src/test_lifecycle.c
    src/test_blob_encode.c
    src/test_blob_decode.c
    src/test_integration.c
)
target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/../../../src/nfc/store
    ${CMAKE_SOURCE_DIR}/../../../src/nfc/router
    ${CMAKE_SOURCE_DIR}/../../../src/nfc/framing
    ${CMAKE_SOURCE_DIR}/../../../src
)
```

```ini
# tests/unit/nfc_store/prj.conf
CONFIG_ZTEST=y
CONFIG_NFC_STORE=y
CONFIG_NFC_STORE_BLOB_SIZE=16384
CONFIG_NFC_STORE_LOG_LEVEL=3
CONFIG_LOG=y
```

**Commit:** `nfc: store: add test suite scaffold (CMakeLists + prj.conf)`

---

### Task 14 — Linter + BUILD_ASSERT verification [~3 min]

1. Build for `qemu_cortex_m33` (or `native_sim` on Linux CI) to confirm all `#include` paths resolve.
2. Verify `BUILD_ASSERT` fires correctly when `NFC_STORE_BLOB_SIZE` is set too small (set `CONFIG_NFC_STORE_BLOB_SIZE=64` temporarily → build should fail with the assert message).
3. Verify `BUILD_ASSERT` passes at default 12288 with all services enabled.
4. Run `cppcheck --enable=all` (or the project's lint target) on `nfc_store.c` and `nfc_store_shell_cmds.c`; fix any flagged issues.

**Commit:** `nfc: store: BUILD_ASSERT verified; lint clean`

---

### Task 15 — Integration smoke test with `nfc_stack` [~3 min]

With `nfc_stack_init(NULL)`:
1. Confirm `nfc_store_get_state()` == `INITIALIZED` after `nfc_stack_init`.
2. Call `nfc_stack_load("default")` → expect `-EIO` wrapping `-ENOENT` (empty table); verify `nfc_store_get_stats().error_count == 1`.
3. Call `nfc_stack_save("default")` → expect 0; verify shell log contains `@@NFCDUMP@@ default`.
4. Call `nfc_stack_start(&uid)` → expect 0.
5. Call `nfc_stack_save("x")` while STARTED → expect `-EBUSY` from `nfc_stack` (not reaching store).

Add these as a ztest integration scenario in `tests/unit/nfc_store/src/test_integration.c` using a mock `nfc_stack` shim or the real stack on `qemu_cortex_m33`/DK.

**Commit:** `nfc: store: integration smoke test against nfc_stack`

---

### Task 16 — Completeness flags: encode + decode (TDD) [~4 min]

Extend `blob_encode` and `blob_decode` helpers (Tasks 8 & 9) for v0x02:

- `blob_encode`: Source flags from the file-static per-persist_id table
  `k_persist_flags[]` in `nfc_store.c` (DECISION-ST-13). Do **not** add a
  `flags` array parameter to `blob_encode`; the vtable is unchanged.
  Populate the table as follows (hand-provisioned content):
  ```c
  /* nfc_store.c */
  static const uint8_t k_persist_flags[] = {
      [NFC_PERSIST_ID_NDEF]       = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
      [NFC_PERSIST_ID_ULTRALIGHT] = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
      [NFC_PERSIST_ID_EMV]        = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
      [NFC_PERSIST_ID_DESFIRE]    = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
      /* Reader-captured flags are added when the reader role lands (post-Wave-5 amendment). */
  };
  ```
  Write 4-byte entry headers (`persist_id`, `k_persist_flags[persist_id]`, `entry_len` LE u16).
  Always write `NFC_STORE_BLOB_VERSION` (0x02) in the header.
- `blob_decode`: Branch on `version` byte: v0x01 → 3-byte overhead, flags = 0;
  v0x02 → 4-byte overhead, read flags byte. Pass flags to a new optional
  `uint8_t *flags_out` parameter (NULL = caller does not need flags).
- Expose flags via `nfc_store_get_entry_flags(persist_id)` after load (optional;
  may be deferred to when the reader role needs it).

**TDD (add to test_blob_encode.c / test_blob_decode.c):**
```c
/* test_encode_sets_flags */
    /* flags[0]=EMULATION_COMPLETE → byte 1 of entry header = 0x04 */
/* test_decode_v2_flags_preserved */
    /* encode with flags → decode → flags_out matches */
/* test_decode_v1_blob_flags_zero */
    /* v0x01 blob (3-byte overhead) → flags_out = 0 for all entries */
/* test_decode_unknown_version_rejected */
    /* version = 0x03 → -EBADMSG; corrupt_blob_count++ */
/* test_roundtrip_v2_read_only_partial */
    /* READER_CAPTURED | READ_ONLY_PARTIAL flags survive encode→decode */
```

**Commit:** `nfc: store: implement v0x02 completeness flags in blob codec`

---

### Task 17 — Live-persist hook implementation [~4 min]

Implement the live write-through path from §2.6:

1. Add `static nfc_store_commit_fn s_commit_fn` + `s_commit_ctx` to `nfc_store.c` file-statics.
   Default = `nfc_store_commit_stub` (no-op; returns 0).
2. Implement `nfc_store_register_commit_cb` per §2.6 (same guard pattern as `register_save_cb`).
3. Implement `nfc_store_on_dirty(svc, tag)`:
   - Guards: ENODEV, EINVAL (NULL svc/tag, persist_id==0).
   - Serializes only `svc` into a service-sized sub-region of `s_staging_buf`
     (no full-blob construction; no CRC; raw service bytes only).
   - Calls `s_commit_fn(svc, tag, s_commit_ctx)`. STATS_ERROR on failure.
   - Does not increment `save_count`; add a `live_commit_count` stat counter
     (add to `nfc_store_stats_t`).
4. The default no-op stub ensures this is safe before any real NVS backend is wired.

**DECISION-ST-12 reminder:** `nfc_store_on_dirty` is `@caller_wq`. No extra locking
needed around `s_staging_buf` vs save/load (mutually exclusive via NOT STARTED guard).
Add `live_commit_count` to the `nfc_store_stats_t` struct and the `store stats` shell output.

**TDD:**
```c
/* test_on_dirty_uninit */           /* before init → -ENODEV */
/* test_on_dirty_null_svc */         /* NULL svc → -EINVAL */
/* test_on_dirty_zero_persist_id */  /* svc->persist_id == 0 → -EINVAL */
/* test_on_dirty_noop_stub */        /* no callback registered → returns 0 */
/* test_on_dirty_custom_cb_called */ /* custom cb receives correct svc + tag */
/* test_live_commit_count_incremented */ /* live_commit_count++ on success */
```

**Commit:** `nfc: store: implement nfc_store_on_dirty live write-through hook`

---

### Task 18 — Export/import implementation + round-trip test [~4 min]

Implement `cmd_store_export` and `cmd_store_import` from the Task 12 extension.

Key implementation notes:
- `s_import_buf[CONFIG_NFC_STORE_BLOB_SIZE]` is a file-static BSS array in
  `nfc_store_shell_cmds.c` (MISRA Dir 4.12 compliant; no dynamic allocation).
- `hex2bin(hex_str, hex_len, s_import_buf, sizeof(s_import_buf), &bin_len)` from
  `<zephyr/sys/util.h>`. Returns negative on malformed input → `-EINVAL` early exit.
- The one-shot callback swap (register → call nfc_stack → restore) must restore
  the original callback even if nfc_stack returns an error (use cleanup path).
- Export output format: single line `@@NFCEXPORT@@ <tag> <hex>` to the shell,
  where `<hex>` is lowercase, no spaces — suitable for direct copy-paste to
  `store import <tag> <hex>`.

**TDD:**
```c
/* test_export_import_roundtrip */
    /* export a saved card → hex string → import into fresh service state
     * → all service deserializers receive identical data. Pure ztest. */
/* test_import_bad_hex */
    /* odd-length hex string → -EINVAL; load callback not called */
/* test_export_requires_not_started */
    /* stack STARTED → nfc_stack_save returns -EBUSY → export returns -EBUSY */
```

**Commit:** `nfc: store: implement export/import shell commands + round-trip test`

## Decision Register

| Flag | Decision | Section |
|------|----------|---------|
| **DECISION-ST-1** | Backend = stub seam only. No NVS/Settings/ZMS. Real backends arrive as callback registrations post-Wave-6. `prj.conf` confirms; spec v2 §12 confirms. | §1, §6.1 |
| **DECISION-ST-2** | `nfc_store_config_t` = one field: `max_tag_len` (uint8_t). All other tunables via Kconfig. NULL cfg → defaults. | §1.1 |
| **DECISION-ST-3** | Blob format: 6-byte versioned header (`NF` magic, version=0x01, reserved, payload_len LE u16) + TLV entries (persist_id u8, entry_len u16 LE, data) + 2-byte CRC16-CCITT trailer. No external persistence; CRC from `<zephyr/sys/crc.h>`. **Note:** this entry describes the v0x01 initial framing; DECISION-ST-11 (below) supersedes the TLV entry format with the v0x02 extension (flags:u8 added). Current encoder writes v0x02. | §1.3–1.4 |
| **DECISION-ST-4** | Partial-load: continue-on-deserialize-error. `partial_load_count++` + `STATS_ERROR()` + continue to next entry. TLV-structure corruption stops immediately. Return `-EBADMSG` if any deserialize failed. | §3, §6 |
| **DECISION-ST-5** | Tag → slot: `strcmp` in compiled-in table for the load stub. First match wins. Max tag len = `CONFIG_NFC_STORE_MAX_TAG_LEN` (default 16 incl. NUL). Future NVS backend owns its own mapping. | §1.3, §4.4 |
| **DECISION-ST-6** | CRC = CRC16-CCITT (poly 0x1021, init 0xFFFF). `crc16_ccitt()` from Zephyr sys. No dependency. | §1.3, §6.5 |
| **DECISION-ST-7** | `s_staging_buf[CONFIG_NFC_STORE_BLOB_SIZE]` file-static BSS. Default 12288. BUILD_ASSERT on `NFC_STORE_MIN_BUF_NEEDED`. Trim by reducing `CONFIG_NFC_STORE_BLOB_SIZE` when DeSFire is disabled. | §1.4, §4.1 |
| **DECISION-ST-8** | Shell `store save/load` → `nfc_stack_save/load()`. NOT direct to store. Preserves `-EBUSY` guard in `nfc_stack`. | §2, §5, §6.6 |
| **DECISION-ST-9** | `nfc_store_save()` returns `-ENOMEM` on runtime buffer overflow (BUILD_ASSERT is compile-time only; defensive check in code). | §3 |
| **DECISION-ST-10** | `nfc_store_register_*_cb(NULL, …)` restores default stub (save → shell, load → compiled-in header). | §2.3, §3 |
| **DECISION-ST-11** | Blob v0x02 extends TLV entry header with `flags:u8` (bit 0=READER_CAPTURED, 1=HAND_AUTHORED, 2=EMULATION_COMPLETE, 3=READ_ONLY_PARTIAL). Encoder writes v0x02; decoder is version-aware (v0x01 → flags=0). +5 bytes worst-case. | §1.4, §1.7, Task 16 |
| **DECISION-ST-12** | **[DECISION — cross-review flag]** `nfc_store_on_dirty()` is `@caller_wq`. `s_staging_buf` shared with save/load but mutually exclusive via NOT STARTED guard. Single WQ thread serialises rapid mutations. Flag for cross-review if ISR or second-thread path added. | §2.6, §5.2, Task 17 |
| **DECISION-ST-13** | Flags producer = static per-persist_id table `k_persist_flags[]` in `nfc_store.c`; services don't report flags; vtable unchanged. Hand-provisioned services (NDEF/Ultralight/EMV/DeSFire) carry `HAND_AUTHORED \| EMULATION_COMPLETE`. Reader-captured NDEF carries `READER_CAPTURED \| EMULATION_COMPLETE` (full clone — spec §1.1). DeSFire/Aliro reader captures carry `READER_CAPTURED \| READ_ONLY_PARTIAL`. | §3, §2.4, Task 16 |
| **DECISION-ST-14** | **[LOCKED 2026-06-13]** NDEF live persist is **required** for `NFC_PROFILE_NDEF`: `nfc_stack.c` calls `nfc_store_on_dirty(ndef_service_get(), active_tag)` after each successful NDEF-file UPDATE BINARY (stack tracks `active_tag` from last `nfc_stack_load()`). Replaces the prior "post-Wave-5 amendment / app-discretionary" wording. Ultralight profile is out of scope (`DECISION-UL-3`). | §2.6, §5.2 |
