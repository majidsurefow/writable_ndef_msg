# Wave 5b: DeSFire Service Implementation Plan

**Status:** DRAFT — 2026-06-12

**Layer:** `services/desfire`

**Files produced:**
- `src/nfc/services/desfire/desfire_service.h`
- `src/nfc/services/desfire/desfire_service.c`
- `src/nfc/services/desfire/desfire_data.h`
- `src/nfc/services/desfire/desfire_service_shell_cmds.c`
- `src/nfc/services/desfire/CMakeLists.txt`
- `src/nfc/services/desfire/Kconfig`
- `tests/unit/nfc_desfire/` (ztest — frame builders, auth math vectors, file lookup, serialize round-trip; Twister tag `sigmation.nfc.desfire`)

**Depends on (LOCKED):**
- `docs/superpowers/plans/wave1-hal.md` (2026-06-12) — `nfc_transport_send_response`, `NFC_TRANSPORT_MAX_RESPONSE_LEN`, `nfc_work_q`
- `docs/superpowers/plans/wave2-framing.md` (2026-06-12) — `nfc_apdu_t`, `apdu_types.h` SW vocabulary, `NFC_CLA_DESFIRE 0x90`
- `docs/superpowers/plans/wave3-router.md` (2026-06-12) — `nfc_service_t` vtable, `service.h`, `NFC_PERSIST_ID_DESFIRE 0x02`
- `docs/superpowers/plans/wave4-stack.md` (2026-06-12) — `desfire_service_init/shutdown/get` surface, AID `D2 76 00 00 85 01 00`, `CONFIG_NFC_SERVICE_DESFIRE`

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full (§2 lifecycle/pattern A + SMF, §3 coupling, §4 vtable, §5 buffer model, §6 stats, §7 threading, §9 errno, §10 shell, §11 MISRA, §12 checklist)
2. `docs/superpowers/plans/wave3-router.md` §1 — `service.h` vtable verbatim + persist-ID table
3. `docs/superpowers/plans/wave4-stack.md` §5.2 — expected surface; AID constant
4. `docs/nfc/archive/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) §6.4.2, §6.5, §13
5. `docs/superpowers/plans/wave1-hal.md` §2 — `send_response` contract, `NFC_TRANSPORT_MAX_RESPONSE_LEN`
6. `docs/superpowers/plans/wave2-framing.md` §1 — `nfc_apdu_t`, SW vocabulary
7. `flipperzero/lib/nfc/protocols/mf_desfire/mf_desfire.h` — command codes, data model field layout
8. `flipperzero/lib/nfc/protocols/mf_desfire/mf_desfire_i.h` — status byte constants
9. `flipperzero/lib/nfc/protocols/mf_desfire/mf_desfire_poller.h` — read/auth operation signatures (reference only)
10. `docs/API_DESIGN_CREED.md`, `docs/STATS_API_DESIGN.md` — module contract patterns
11. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_RESET`, `STATS_COPY_OUT`
12. `~/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001 through DEV-ZEP-018
13. `~/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h` (replaced prototype).
> **Flipper source used for:** command/status byte constants and data model field names ONLY. All logic is re-expressed independently (Flipper = GPL; this stack = project license). No Flipper API structure, call conventions, or `SimpleArray` patterns carried forward.

---

> **Architecture Framing — spec v3 (2026-06-12):** This service is the
> **NFCT/card-role first-slice** of the **DeSFire** protocol module as defined by the
> [NFC Stack Architecture v3](../specs/2026-06-12-nfc-stack-architecture.md) (§4.3).
> A protocol module owns: data model (`desfire_data_t`) · (de)serialize ·
> **listener** (this file, built under `CONFIG_NFC_ROLE_CARD`) · **poller**
> (reader role — RESERVED, not implemented in this slice).
> **Lane:** `NFC_LANE_ISO_DEP` — Type-4, dispatched via APDU framing (Wave 2) + AID
> router (Wave 3). **Kconfig:** `NFC_SERVICE_DESFIRE` enables this protocol module; the
> listener compiles under `CONFIG_NFC_ROLE_CARD` (orthogonal, Wave 1). Existing symbol
> names are unchanged. **Serialize completeness:** output classified
> `NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED`
> for structural fields; secret AES keys in the blob are as-provisioned — a future
> poller blob would carry `NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL` because secret
> keys cannot be extracted from a physical DeSFire card
> (wave6 canonical C symbols — wave6-store.md §1.7 / spec v3 §4.5).

---

## DECISION Summary

| ID | Subject | Decision |
|----|---------|----------|
| DECISION-A | Command wrapping form | CLA=0x90 native DeSFire over ISO-DEP only. CLA=0x00 for non-SELECT not supported. |
| DECISION-B | Auth scope | EV1 (0xAA) + EV2First (0x71) supported; EV2NonFirst (0x77) supported as continuation. |
| DECISION-C | Data model sizing | MAX_APPS=4, MAX_FILES_PER_APP=8, MAX_FILE_SIZE=256 B. ~10.5 KB RAM total. |
| DECISION-D | Write operations | CreateApplication, WriteData, WriteRecord, ChangeKey, CommitTransaction — out of scope; respond 0x91 1C. |
| DECISION-E | ADDITIONAL_FRAME chaining | Generic pending-response mechanism for GetVersion (3 sub-frames) and any large read. |
| DECISION-F | on_select response | Plain ISO SW 9000 (not DeSFire 0x9100). |
| DECISION-G | Crypto dispatch | PSA AES runs synchronously on nfc_work_q. No deferred work. FWI=8 budget ~5 s; auth ≤ 500 µs. |
| DECISION-H | Auth reset triggers | Auth FSM resets on field-off, on deselect, and on any SELECT_APPLICATION. |
| DECISION-I | File access rights | Simplified: comm_settings == PLAIN → no auth; MAC or Encrypted → require AUTHENTICATED. |
| DECISION-J | GetFreeMemory | Reports static `CONFIG_NFC_DESFIRE_FREE_MEMORY` (Kconfig, default 7520). |
| DECISION-K | GetApplicationIDs | Single-frame response (MAX_APPS×3 ≤ 12 bytes; no chaining needed). |
| DECISION-L | Key scope during auth | PICC context → master_key; App context → apps[selected_idx].keys[auth_key_no]. |

---

## 1. Types and Constants

### 1.1 DeSFire Command Codes — `desfire_service.h`

Re-expressed from protocol facts (mf_desfire.h, mf_desfire_i.h). All names are project-local.

```c
/* ── DeSFire EV1/EV2 command codes (INS byte, CLA=0x90) ───────────────────── */
#define DESFIRE_CMD_GET_VERSION          0x60U  /**< Hardware/software version, UID, batch      */
#define DESFIRE_CMD_GET_FREE_MEMORY      0x6EU  /**< Free NV memory in bytes                    */
#define DESFIRE_CMD_GET_KEY_SETTINGS     0x45U  /**< Key settings + max_keys for selected app   */
#define DESFIRE_CMD_GET_KEY_VERSION      0x64U  /**< Version byte for one key                   */
#define DESFIRE_CMD_GET_APPLICATION_IDS  0x6AU  /**< List all application IDs                   */
#define DESFIRE_CMD_SELECT_APPLICATION   0x5AU  /**< Select application by 3-byte AID           */
#define DESFIRE_CMD_GET_FILE_IDS         0x6FU  /**< List file IDs in selected application      */
#define DESFIRE_CMD_GET_FILE_SETTINGS    0xF5U  /**< File type, comm, access rights, size       */
#define DESFIRE_CMD_READ_DATA            0xBDU  /**< Read standard/backup file data             */
#define DESFIRE_CMD_GET_VALUE            0x6CU  /**< Read value file                            */
#define DESFIRE_CMD_READ_RECORDS         0xBBU  /**< Read record file                           */
#define DESFIRE_CMD_AUTHENTICATE_AES     0xAAU  /**< EV1 3-pass AES mutual authentication       */
#define DESFIRE_CMD_AUTH_EV2_FIRST       0x71U  /**< EV2 first-application authentication       */
#define DESFIRE_CMD_AUTH_EV2_NON_FIRST   0x77U  /**< EV2 non-first authentication               */
#define DESFIRE_CMD_ADDITIONAL_FRAME     0xAFU  /**< Continuation frame (reader→card or status) */
```

### 1.2 DeSFire Status Bytes — `desfire_service.h`

Used as SW2 in DeSFire responses (SW1 always 0x91).

> **Scope note:** the `0x91xx` family is the proprietary DeSFire EV1/EV2 trailing-status
> convention, NOT ISO 7816-4 status words. These constants are private to this service
> (`DESFIRE_*` prefix, defined here — never added to `apdu_types.h`) and must not be
> built with the `NFC_SW1`/`NFC_SW2` macros, which exist for the shared ISO vocabulary.

```c
/* ── DeSFire native status codes (SW2 when SW1=0x91) ──────────────────────── */
#define DESFIRE_STATUS_OK                0x00U  /**< Operation completed successfully            */
#define DESFIRE_STATUS_NO_CHANGES        0x0CU  /**< Backup file: no changes to commit          */
#define DESFIRE_STATUS_ILLEGAL_CMD       0x1CU  /**< Command code not supported                 */
#define DESFIRE_STATUS_NO_SUCH_KEY       0x40U  /**< Invalid key number                         */
#define DESFIRE_STATUS_LENGTH_ERROR      0x7EU  /**< Command length invalid                     */
#define DESFIRE_STATUS_PERMISSION_DENIED 0x9DU  /**< Config does not allow this command         */
#define DESFIRE_STATUS_PARAM_ERROR       0x9EU  /**< Parameter value invalid                    */
#define DESFIRE_STATUS_APP_NOT_FOUND     0xA0U  /**< Requested AID not present                  */
#define DESFIRE_STATUS_AUTH_ERROR        0xAEU  /**< Current auth status does not allow command */
#define DESFIRE_STATUS_ADDITIONAL_FRAME  0xAFU  /**< Additional data frame expected             */
#define DESFIRE_STATUS_BOUNDARY_ERROR    0xBEU  /**< Read/write beyond file/record limits       */
#define DESFIRE_STATUS_CMD_ABORTED       0xCAU  /**< Previous command not fully completed       */
#define DESFIRE_STATUS_FILE_NOT_FOUND    0xF0U  /**< File number does not exist                 */

/* SW1 byte for all native DeSFire responses over ISO-DEP. */
#define DESFIRE_SW1                      0x91U
```

### 1.3 File and Key Constants — `desfire_service.h`

```c
#define DESFIRE_APP_ID_SIZE      3U   /**< DeSFire application ID is always 3 bytes   */
#define DESFIRE_MAX_KEYS        14U   /**< Maximum keys per application (EV1 spec)    */
#define DESFIRE_AES_KEY_SIZE    16U   /**< AES-128 key length in bytes                */
#define DESFIRE_RND_SIZE        16U   /**< RndA / RndB random challenge length        */
#define DESFIRE_UID_SIZE         7U   /**< PICC UID (7-byte double-size NFCID1)       */
#define DESFIRE_BATCH_SIZE       5U   /**< Production batch number length             */
#define DESFIRE_VERSION_SIZE     7U   /**< hw_version / sw_version field length       */

/** AID: ISO SELECT routes to this service. Internal app selection uses 0x5A. */
static const uint8_t k_desfire_aid[] = {
    0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U
};
#define DESFIRE_AID_LEN  7U

/**
 * Maximum bytes for serialize() output. Wave 6 store sizes its TLV blob from
 * this. Derivation (worst-case, Kconfig defaults):
 *   Header: 1 (format_ver) + 7 (hw_ver) + 7 (sw_ver) + 7 (uid) + 5 (batch)
 *           + 2 (prod_week/year) + 4 (free_mem) + 16 (master_key) + 1 (key_settings)
 *           + 1 (app_count) = 51 bytes
 *   Per-app: 3 (app_id) + 1 (key_settings) + 1 (key_count) + 1 (file_count)
 *            + 14×16 (keys) + per-file:
 *   Per-file: 1 (file_id) + 1 (type) + 1 (comm) + 2 (access_rights) + 12 (settings_union)
 *             + 2 (actual_data_len) + 256 (MAX_FILE_SIZE) = 275 bytes
 *   Per-app total: 6 + 224 + 8×275 = 2430 bytes
 *   Grand total: 51 + 4×2430 = 9771 → round up to 10240.
 */
#define DESFIRE_SERVICE_MAX_SERIALIZED   10240U
```

### 1.4 File Type and Communication Settings — `desfire_data.h`

```c
typedef enum {
    DESFIRE_FILE_TYPE_STANDARD    = 0U,  /**< Standard data file             */
    DESFIRE_FILE_TYPE_BACKUP      = 1U,  /**< Backup data file               */
    DESFIRE_FILE_TYPE_VALUE       = 2U,  /**< Value file (signed 32-bit)     */
    DESFIRE_FILE_TYPE_LINEAR_REC  = 3U,  /**< Linear record file             */
    DESFIRE_FILE_TYPE_CYCLIC_REC  = 4U,  /**< Cyclic record file             */
} desfire_file_type_t;

typedef enum {
    DESFIRE_COMM_PLAIN      = 0U,  /**< Plain communication                  */
    DESFIRE_COMM_MAC        = 1U,  /**< MACed communication                  */
    DESFIRE_COMM_ENCRYPTED  = 3U,  /**< Fully encrypted communication        */
} desfire_comm_t;
```

### 1.5 Data Model — `desfire_data.h`

**Design rationale:** Flipper Zero's full DeSFire model uses dynamic arrays (`SimpleArray*`),
consuming heap proportional to card content (up to 28 apps × 32 files × unbounded data).
On nRF54L15 (primary target) / nRF52840 (secondary), with no heap allocator in the running phase (creed §9) and limited RAM shared
with BLE + Aliro stacks, a statically-bounded model is mandatory. Kconfig tunables let
integrators trade memory against capacity.

```c
/* src/nfc/services/desfire/desfire_data.h
 *
 * Static-allocation DeSFire EV1/EV2 data model.
 * All sizes governed by NFC_DESFIRE_MAX_* Kconfig symbols.
 * This header is pure C — no Zephyr kernel includes.
 */
#ifndef SRC_NFC_SERVICES_DESFIRE_DESFIRE_DATA_H
#define SRC_NFC_SERVICES_DESFIRE_DESFIRE_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "desfire_service.h"   /* command/status constants */

/* ── File settings ──────────────────────────────────────────────────────────── */

/** Settings union specific to each file type. */
typedef union {
    struct {
        uint32_t size;         /**< Standard/backup file size in bytes         */
    } data;
    struct {
        int32_t  lo_limit;
        int32_t  hi_limit;
        int32_t  limited_credit_value;
        bool     limited_credit_enabled;
    } value;
    struct {
        uint32_t record_size;
        uint32_t max_records;
        uint32_t cur_records;
    } record;
} desfire_file_settings_union_t;

typedef struct {
    desfire_file_type_t         type;          /**< File type (enum above)         */
    desfire_comm_t              comm;          /**< Communication setting           */
    uint16_t                    access_rights; /**< 2-byte access rights bitfield  */
    desfire_file_settings_union_t settings;   /**< Type-specific size/limits       */
} desfire_file_settings_t;

/* ── Per-application record ──────────────────────────────────────────────────── */

typedef struct {
    uint8_t data[DESFIRE_APP_ID_SIZE];  /**< 3-byte application identifier */
} desfire_app_id_t;

typedef struct {
    desfire_app_id_t        app_id;
    uint8_t                 key_settings;    /**< Key settings byte              */
    uint8_t                 key_count;       /**< Number of valid keys (1..14)   */
    uint8_t                 file_count;      /**< Number of valid files          */
    uint8_t                 keys[DESFIRE_MAX_KEYS][DESFIRE_AES_KEY_SIZE];
    uint8_t                 file_ids[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP];
    desfire_file_settings_t file_settings[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP];
    uint16_t                file_data_len[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP]; /**< Actual bytes used */
    uint8_t                 file_data[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP]
                                     [CONFIG_NFC_DESFIRE_MAX_FILE_SIZE];
} desfire_application_t;

/* ── PICC-level card record ──────────────────────────────────────────────────── */

typedef struct {
    uint8_t hw_version[DESFIRE_VERSION_SIZE]; /**< 7-byte HW version block         */
    uint8_t sw_version[DESFIRE_VERSION_SIZE]; /**< 7-byte SW version block         */
    uint8_t uid[DESFIRE_UID_SIZE];            /**< 7-byte PICC UID                 */
    uint8_t batch[DESFIRE_BATCH_SIZE];        /**< 5-byte production batch number  */
    uint8_t prod_week;                        /**< Production week (BCD)           */
    uint8_t prod_year;                        /**< Production year (BCD)           */
    uint32_t free_memory;                     /**< Reported free NV memory (bytes) */
    uint8_t master_key[DESFIRE_AES_KEY_SIZE]; /**< PICC master key (AES-128)       */
    uint8_t master_key_settings;             /**< PICC master key settings byte   */
    uint8_t app_count;                        /**< Valid entries in apps[]         */
    desfire_application_t apps[CONFIG_NFC_DESFIRE_MAX_APPS];
} desfire_data_t;

#endif /* SRC_NFC_SERVICES_DESFIRE_DESFIRE_DATA_H */
```

**RAM footprint (Kconfig defaults: MAX_APPS=4, MAX_FILES_PER_APP=8, MAX_FILE_SIZE=256):**

| Component | Bytes |
|-----------|-------|
| PICC header (hw/sw/uid/batch/prod/free/master_key) | 28 + 4 + 1 + 1 + 16 + 1 = 51 |
| app_count | 1 |
| desfire_application_t × 4 (app_id 3 + key_settings 1 + key_count 1 + file_count 1 + keys 224 + file_ids 8 + file_settings 8×24 + file_data_len 8×2 + file_data 8×256) | 4 × (3+1+1+1+224+8+192+16+2048) = 4 × 2494 = 9,976 |
| **desfire_data_t total** | **~10,028 bytes** |
| Session state (s_auth_state) | ~80 bytes |
| Pending-response buffer | 288 bytes |
| Static response buffer (s_resp_buf) | 512 bytes |
| SMF context + lifecycle state | ~48 bytes |
| **Grand total** | **~10,956 bytes ≈ 10.7 KB** |

nRF54L15 has ~188 KiB SRAM (primary target); nRF52840 has 256 KB RAM (secondary). Typical BLE + Aliro overhead ≤ 150 KB on either target. 10.7 KB is acceptable.

> **DECISION-C (data model sizing):** `NFC_DESFIRE_MAX_APPS=4`, `NFC_DESFIRE_MAX_FILES_PER_APP=8`, `NFC_DESFIRE_MAX_FILE_SIZE=256`. Justification: (1) Real-world transit/access cards typically carry 1–4 apps, each with 4–8 files of ≤ 256 bytes. (2) Total ≤ 11 KB in BSS — fits comfortably in nRF54L15 SRAM shared with BLE and Aliro (and on the secondary nRF52840). (3) Implementers may increase via Kconfig at the cost of proportionally more RAM.

### 1.6 Module Config, Stats, State — `desfire_service.h`

```c
/* ── desfire_service_config_t — frozen after init() ────────────────────────── */
typedef struct {
    uint8_t max_apps;           /**< == CONFIG_NFC_DESFIRE_MAX_APPS          */
    uint8_t max_files_per_app;  /**< == CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP */
    uint16_t max_file_size;     /**< == CONFIG_NFC_DESFIRE_MAX_FILE_SIZE     */
} desfire_service_config_t;

/* ── desfire_service_stats_t ─────────────────────────────────────────────────── */
typedef struct {
    uint32_t select_count;           /**< on_select calls (ISO SELECT matched AID)  */
    uint32_t apdu_count;             /**< on_apdu calls dispatched                  */
    uint32_t auth_started_count;     /**< AUTHENTICATE_AES / AUTH_EV2_FIRST calls   */
    uint32_t auth_completed_count;   /**< Auth FSM reached AUTHENTICATED state       */
    uint32_t auth_failed_count;      /**< Auth FSM rejected (bad RndB or RndA)      */
    uint32_t app_not_found_count;    /**< SELECT_APPLICATION AID not in table        */
    uint32_t file_not_found_count;   /**< GetFileIDs/Settings/Read with bad file_id  */
    uint32_t auth_error_count;       /**< Auth required but not authenticated        */
    uint32_t illegal_cmd_count;      /**< Unknown/write command → ILLEGAL_CMD        */
    uint32_t chained_resp_count;     /**< ADDITIONAL_FRAME continuation frames sent  */
    uint32_t serialize_count;        /**< serialize() calls                          */
    uint32_t deserialize_count;      /**< deserialize() calls                        */
    uint32_t error_count;            /**< Mandatory — STATS_ERROR increments this    */
    int32_t  last_error_code;        /**< Mandatory — last code passed to STATS_ERROR*/
} desfire_service_stats_t;

/* ── desfire_service_state_t — Pattern A ────────────────────────────────────── */
typedef enum {
    DESFIRE_SERVICE_STATE_UNINITIALIZED = 0,  /**< Before init()            */
    DESFIRE_SERVICE_STATE_INITIALIZED,        /**< After init(); ready      */
} desfire_service_state_t;

/* ── Auth FSM states (SMF) ──────────────────────────────────────────────────── */
typedef enum {
    DESFIRE_AUTH_STATE_IDLE = 0,    /**< No auth in progress or session expired    */
    DESFIRE_AUTH_STATE_STEP1,       /**< Card sent enc_RndB; waiting for reader    */
    DESFIRE_AUTH_STATE_STEP2,       /**< Reader's challenge received; card verifying */
    DESFIRE_AUTH_STATE_AUTHENTICATED, /**< Mutual auth complete; session key live   */
} desfire_auth_state_t;
```

---

## 2. Public API

All declarations live in `desfire_service.h`.

```c
/* ── Minimal lifecycle (CONVENTIONS §2) ───────────────────────────────────────── */

/**
 * @brief Initialize the DeSFire service.
 *
 * Clears the card data model to zero, resets all session state, and sets
 * lifecycle state to INITIALIZED. Idempotent guard: returns -EALREADY if
 * already INITIALIZED. Stats are cleared after the guard.
 *
 * @param cfg  NULL → Kconfig defaults (desfire_service_config_t §1.6 struct).
 * @retval  0         Success.
 * @retval -EALREADY  Already initialized.
 * @caller_sync
 */
int desfire_service_init(const desfire_service_config_t *cfg);

/**
 * @brief Shut down the DeSFire service.
 *
 * Clears any session crypto material (PSA key handles, RndA/RndB buffers),
 * resets lifecycle state to UNINITIALIZED. Idempotent.
 *
 * @retval  0  Always 0; idempotent (no-op if already UNINITIALIZED).
 * @caller_sync
 */
int desfire_service_shutdown(void);

/* ── vtable accessor ──────────────────────────────────────────────────────────── */

/**
 * @brief Return the service vtable for registration with the aid_router.
 *
 * Returns a pointer to the static nfc_service_t instance. The caller (nfc_stack)
 * passes this directly to aid_router_register(). The pointer is stable for the
 * lifetime of the process.
 *
 * @retval non-NULL always (even before init — the struct is statically allocated).
 * @threadsafe (read-only static pointer)
 */
const nfc_service_t *desfire_service_get(void);

/* ── Module observability getters ─────────────────────────────────────────────── */

/**
 * @retval never NULL; config is frozen after init().
 * @threadsafe
 */
const desfire_service_config_t *desfire_service_get_config(void);

/**
 * @brief Copy-out stats snapshot.
 * @retval  0         Success; *out filled.
 * @retval -EINVAL    out is NULL.
 * @threadsafe
 */
int desfire_service_get_stats(desfire_service_stats_t *out);

/**
 * @retval Current lifecycle state; never fails.
 * @threadsafe
 */
desfire_service_state_t desfire_service_get_state(void);
```

---

## 3. Contracts

### 3.1 Lifecycle Guards

| Function | Pre-condition | Post-condition | Error codes |
|----------|---------------|----------------|-------------|
| `desfire_service_init()` | State == UNINITIALIZED | State == INITIALIZED; card data zeroed; session cleared | `-EALREADY` if already INITIALIZED; `cfg == NULL` → Kconfig defaults |
| `desfire_service_shutdown()` | Any state | State == UNINITIALIZED; PSA crypto material cleared | always 0; idempotent (was UNINITIALIZED → no-op) |
| `desfire_service_get()` | none | Returns `&s_service` (non-NULL static pointer) | never fails |
| `desfire_service_get_config()` | none | Returns `&s_config` | never fails |
| `desfire_service_get_stats(out)` | out != NULL | *out = copy of s_stats under spinlock | `-EINVAL` if out == NULL |
| `desfire_service_get_state()` | none | Returns s_state | never fails |

### 3.2 vtable Callback Contracts

All vtable callbacks are invoked **exclusively on `nfc_work_q`** (CONVENTIONS §7). The service must call `nfc_transport_send_response()` before returning from `on_select` and `on_apdu`. No sleeping or blocking.

#### `on_select(aid, aid_len, user_ctx)`

- Called when the ISO SELECT APDU `CLA=0x00, INS=0xA4, P1=0x04` matches AID `D2 76 00 00 85 01 00`.
- Resets all session state: auth FSM → IDLE, selected app → none (PICC context).
- Sends response `{0x90, 0x00}` (ISO SW 9000 — plain success, no FCI data).
- Increments `s_stats.select_count`.

> **DECISION-F (on_select response):** The initial AID SELECT is an ISO 7816-4 operation, not a native DeSFire command. The response is ISO SW 9000 (`{0x90, 0x00}`), not the DeSFire `{0x91, 0x00}`. The reader has selected the PICC-level DeSFire "applet" via T4T AID routing; subsequent DeSFire commands then arrive with CLA=0x90. Using 9000 here is standard T4T practice and consistent with NDEF/EMV service behavior (wave3 DECISION-B: service controls SELECT response).

#### `on_apdu(apdu, user_ctx)`

- Precondition: service is INITIALIZED; `apdu` is valid for the duration of the call.
- Dispatches on `apdu->ins` (and `apdu->cla` for form validation).
- **Must** call `nfc_transport_send_response()` with a DeSFire-format response before returning, OR (for ADDITIONAL_FRAME chaining) advance the pending state and send the next chunk.
- Increments `s_stats.apdu_count`.
- If `apdu->cla != DESFIRE_CLA_NATIVE (0x90)` and `apdu->ins != NFC_INS_SELECT (0xA4)` (already handled by router): respond `{DESFIRE_SW1, DESFIRE_STATUS_ILLEGAL_CMD}`.
- See §3.3 for the full command decision table.
- Note: `apdu->data` is a zero-copy pointer into the inbound net_buf (framing layer guarantee). Any bytes needed after `on_apdu` returns MUST be copied into `s_work_buf` before using them.

#### `on_deselect(user_ctx)`

- Resets all session state: auth FSM → IDLE, selected app → none, pending_cmd → NONE.
- **Does NOT** call `nfc_transport_send_response()` (the new SELECT's on_select will).

#### `on_field_off(user_ctx)`

- Resets all session state identically to on_deselect.
- Destroys any live PSA key handle in s_auth_session.

#### `serialize(out, max, out_len, user_ctx)` (@caller_sync only)

- Serializes `s_card_data` (not session state) into `out[0..max)`.
- Format: `[format_version:u8=1][desfire_data fields, length-prefixed per §6.7]`.
- Sets `*out_len` to bytes written; returns 0 on success, `-ENOSPC` if `max < DESFIRE_SERVICE_MAX_SERIALIZED`.
- Never called while stack is STARTED (nfc_stack -EBUSY guard). No internal locking needed.

#### `deserialize(in, len, user_ctx)` (@caller_sync only)

- Restores `s_card_data` from `in[0..len)`.
- Validates format_version byte; returns `-EBADMSG` on version mismatch or truncated data.
- Clears any live session state (a deserialized new card must not inherit prior auth).

### 3.3 Command Decision Table

`AUTH` column: whether AUTHENTICATED state is required for this command. Rows show the DeSFire status code returned under each condition.

| INS (hex) | Command | CTX required | AUTH required | Success response | Not auth'd | App not found | File not found | No app selected | Params bad |
|-----------|---------|-------------|---------------|------------------|------------|---------------|----------------|-----------------|------------|
| 0x60 | GetVersion | any | no | 3×0x91AF + 0x9100 | n/a | n/a | n/a | n/a | n/a |
| 0x6E | GetFreeMemory | any | no | `[free_mem:4LE]` + 0x9100 | n/a | n/a | n/a | n/a | n/a |
| 0x45 | GetKeySettings | any | no | `[key_settings:1][max_keys:1]` + 0x9100 | n/a | n/a | n/a | n/a | n/a |
| 0x64 | GetKeyVersion | any | no | `[key_ver:1]` + 0x9100 | n/a | n/a | n/a | 0x91A0 if no key | 0x9140 bad key_no |
| 0x6A | GetApplicationIDs | any | no | `[app_id × N]` + 0x9100 | n/a | n/a | n/a | n/a | n/a |
| 0x5A | SelectApplication | any | no | 0x9100 | n/a | 0x91A0 | n/a | n/a | 0x917E |
| 0x6F | GetFileIDs | CTX_APP | no | `[file_id × N]` + 0x9100 | n/a | n/a | n/a | 0x91A0 | n/a |
| 0xF5 | GetFileSettings | CTX_APP | no | `[type:1][comm:1][access:2][settings:8]` + 0x9100 | n/a | n/a | 0x91F0 | 0x91A0 | 0x917E |
| 0xBD | ReadData | CTX_APP | if comm≠PLAIN | `[data…]` + 0x9100 | 0x91AE | n/a | 0x91F0 | 0x91A0 | 0x917E |
| 0x6C | GetValue | CTX_APP | if comm≠PLAIN | `[value:4LE]` + 0x9100 | 0x91AE | n/a | 0x91F0 | 0x91A0 | 0x917E |
| 0xBB | ReadRecords | CTX_APP | if comm≠PLAIN | `[records…]` + 0x9100 | 0x91AE | n/a | 0x91F0 | 0x91A0 | 0x917E |
| 0xAA | AuthenticateAES | any | no (initiates auth) | `[enc_rnd_b:16]` + 0x91AF | n/a | n/a | n/a | n/a | 0x9140 bad key |
| 0x71 | AuthEV2First | any | no | `[enc_challenge:16]` + 0x91AF | n/a | n/a | n/a | n/a | 0x9140 |
| 0x77 | AuthEV2NonFirst | CTX_APP | partial (prev auth session) | `[enc_challenge:16]` + 0x91AF | 0x91AE | n/a | n/a | 0x91A0 | 0x9140 |
| 0xAF | AdditionalFrame | FSM/pending | — | next chunk or auth step 2 | see auth | — | — | — | 0x91CA if no pending |
| any other | — | — | — | 0x91 1C (ILLEGAL_CMD) | | | | | |

> **DECISION-A (Command wrapping form):** Support CLA=0x90 only for all DeSFire post-SELECT commands. Rationale: (1) DeSFire cards communicate through ISO-DEP using the "ISO 7816-4 APDU-wrapped" form where CLA=0x90 carries the DeSFire command as INS — this is the universally used form for DeSFire over contactless. (2) The framing layer (Wave 2) defines `NFC_CLA_DESFIRE 0x90U` exactly for this. (3) CLA=0x00 for non-SELECT DeSFire commands does not exist in any standard DeSFire reader implementation this stack targets. (4) The initial ISO SELECT (CLA=0x00) is handled by the router and surfaced via `on_select` — the service never sees it as an APDU.

> **DECISION-D (Write operations out of scope):** `CreateApplication (0xCA)`, `DeleteApplication (0xDA)`, `CreateFile (0xCD/0xCE/0xCC/0xCF/0xCB)`, `WriteData (0x3D)`, `WriteRecord (0x3B)`, `ChangeKey (0xC4)`, `CommitTransaction (0xC7)`, `AbortTransaction (0xA7)`, `DeleteFile (0xDF)`, `FormatPICC (0xFC)` are all responded to with `{0x91, 0x1C}` (ILLEGAL_COMMAND_CODE). Rationale: this device has no reader mode — card content is provisioned off-device via `nfc_store_load()`. Write capability adds significant complexity (journaling, access rights enforcement, key change crypto) with no benefit to the emulation use case. A future wave may extend this. Flagged as explicit scope limitation.

---

## 4. Internal State

### 4.1 File-Static Module State — `desfire_service.c`

```c
/* ── Lifecycle ──────────────────────────────────────────────────────────────── */
static desfire_service_state_t  s_state  = DESFIRE_SERVICE_STATE_UNINITIALIZED;
static desfire_service_config_t s_config = { /* populated in init() */ };
static desfire_service_stats_t  s_stats;
static struct k_spinlock        s_stats_lock;   /* NOT K_SPINLOCK_DEFINE (NCS v3.2.4) */

/* ── Card data model (persistent, serialize/deserialize) ───────────────────── */
static desfire_data_t           s_card_data;

/* ── Session state — reset on field-off / deselect / SELECT_APPLICATION ─────── */
typedef struct {
    struct smf_ctx ctx;          /* MUST be first — Zephyr SMF requirement        */
    desfire_auth_state_t state;  /* current auth FSM state (observable)           */
    uint8_t auth_key_no;         /* key number being authenticated                */
    bool    is_picc_auth;        /* true = PICC master key; false = app key       */
    bool    is_ev2;              /* true = EV2 flow (0x71); false = EV1 (0xAA)   */
    uint8_t rnd_a[DESFIRE_RND_SIZE];      /* Card-generated random A (auth STEP2) */
    uint8_t rnd_b[DESFIRE_RND_SIZE];      /* Card-generated random B (auth STEP1) */
    uint8_t session_key[DESFIRE_AES_KEY_SIZE];    /* EV1 session key              */
    uint8_t enc_key[DESFIRE_AES_KEY_SIZE];        /* EV2 SesAuthENC               */
    uint8_t mac_key[DESFIRE_AES_KEY_SIZE];        /* EV2 SesAuthMAC               */
    psa_key_id_t psa_key_id;      /* Volatile PSA key handle (0 = not live)       */
} desfire_auth_session_t;

static desfire_auth_session_t   s_auth;
static int8_t                   s_selected_app_idx; /* -1 = PICC ctx; 0..N-1 = app */

/* ── ADDITIONAL_FRAME chaining ──────────────────────────────────────────────── */
typedef enum {
    PENDING_NONE = 0U,
    PENDING_GET_VERSION,      /* GetVersion frame 2 or 3 pending */
    PENDING_READ_DATA,        /* ReadData next chunk pending      */
    PENDING_READ_RECORDS,     /* ReadRecords next chunk pending   */
} desfire_pending_cmd_t;

static desfire_pending_cmd_t    s_pending_cmd = PENDING_NONE;
static uint8_t                  s_pending_buf[CONFIG_NFC_DESFIRE_MAX_FILE_SIZE + 32U];
static uint16_t                 s_pending_total;  /* total bytes in s_pending_buf */
static uint16_t                 s_pending_offset; /* bytes already sent           */
static uint8_t                  s_pending_sub_frame; /* sub-frame index (GetVersion) */

/* ── Static response buffer (outbound, service-owned per CONVENTIONS §5) ─────── */
static uint8_t s_resp_buf[512U];

/* ── vtable instance ────────────────────────────────────────────────────────── */
static const nfc_service_t s_service = {
    .on_select    = desfire_on_select,
    .on_apdu      = desfire_on_apdu,
    .on_deselect  = desfire_on_deselect,
    .on_field_off = desfire_on_field_off,
    .serialize    = desfire_serialize,
    .deserialize  = desfire_deserialize,
    .persist_id   = NFC_PERSIST_ID_DESFIRE,   /* 0x02 from wave3 service.h      */
    .user_ctx     = NULL,                     /* singleton; all state is static  */
};
```

### 4.2 Auth FSM — Zephyr SMF

The auth FSM is a **per-session** state machine, reset on every field-off, deselect, or
SELECT_APPLICATION. It runs entirely on `nfc_work_q` — no cross-thread access.

```
                ┌────────────────────────────────────────────────────────────────┐
                │                   RESET TRIGGERS                               │
                │   field_off / deselect / SELECT_APPLICATION / auth failure     │
                └──────────────────────────┬─────────────────────────────────────┘
                                           │
                                           ▼
         ┌─────────────────────────────────────────────────────────────────────────┐
         │                            IDLE                                         │
         │  • s_selected_app_idx = -1  •  psa_key_id = PSA_KEY_ID_NULL            │
         └──────────────────────────┬──────────────────────────────────────────────┘
                                    │ AUTHENTICATE_AES(key_no) or AUTH_EV2_FIRST(key_no)
                                    │ → generate RndB, import PSA key, encrypt RndB
                                    │ → send {enc_rndB[16], 0x91, 0xAF}
                                    ▼
         ┌─────────────────────────────────────────────────────────────────────────┐
         │                           STEP1                                         │
         │  • rnd_b populated  •  psa_key_id live  •  auth_key_no set             │
         └──────────────────────────┬──────────────────────────────────────────────┘
                                    │ ADDITIONAL_FRAME(enc_challenge[32])
                                    │ → decrypt to get {RndA || rot_left(RndB)}
                                    │ → verify rot_left(RndB) == rot_left(s_auth.rnd_b)
                                    │ → on verify fail: → IDLE + send 0x91AE
                                    │ → on verify pass: → STEP2 (transient)
                                    ▼
         ┌─────────────────────────────────────────────────────────────────────────┐
         │                           STEP2  (transient)                            │
         │  • rnd_a populated from decrypted challenge                             │
         └──────────────────────────┬──────────────────────────────────────────────┘
                                    │ (same on_apdu call, synchronous SMF transition)
                                    │ → encrypt rot_left(RndA) → send {enc_rndA_rot, 0x91, 0x00}
                                    │ → derive session key
                                    │ → destroy PSA key handle
                                    ▼
         ┌─────────────────────────────────────────────────────────────────────────┐
         │                        AUTHENTICATED                                    │
         │  • session_key live  •  auth_key_no set  •  psa_key_id = NULL           │
         │  • EV2: enc_key + mac_key live                                          │
         └─────────────────────────────────────────────────────────────────────────┘
```

**State handlers** (SMF `run` function, no `entry`/`exit` needed for this simple FSM):

| State | `run()` trigger | Action |
|-------|----------------|--------|
| `IDLE` | — | Default; no action on entry. All non-auth APDUs dispatched from the main dispatch switch, not via SMF. |
| `STEP1` | Called by `desfire_cmd_additional_frame()` when `s_pending_cmd == PENDING_NONE` | Decrypt reader's combined frame; verify; on success: transition to STEP2 (then immediately AUTHENTICATED). |
| `STEP2` | Entry (transient) | Build encrypted `rot_left(RndA)`, send final auth response, derive session key, destroy PSA key. |
| `AUTHENTICATED` | — | Auth complete; `desfire_cmd_read_data()` etc. check `s_auth.state == DESFIRE_AUTH_STATE_AUTHENTICATED`. |

> **DECISION-H (Auth reset triggers):** Auth FSM resets to IDLE on: (1) `on_field_off`, (2) `on_deselect`, (3) any `SELECT_APPLICATION` command — including re-selecting the same app. Rationale: real DeSFire cards reset auth on SELECT_APPLICATION per spec. This is conservative and ensures a reader that sends a new SELECT_APPLICATION cannot inherit a prior session. The only PSA cleanup needed is `psa_destroy_key(s_auth.psa_key_id)` if `psa_key_id != PSA_KEY_ID_NULL`.

### 4.3 Pending-Response State Machine

ADDITIONAL_FRAME chaining for multi-frame responses (GetVersion sub-frames; large ReadData/ReadRecords). The pending state is wholly separate from the auth FSM.

```
PENDING_NONE  ─── GetVersion received ──────────→  PENDING_GET_VERSION (sub_frame=1)
                                                     │ reader sends 0xAF
                                                     │ → send SW[7] + 0x91AF, sub_frame=2
                                                     │ reader sends 0xAF
                                                     │ → send UID/batch + 0x9100
                                                     └──────────────────→  PENDING_NONE

PENDING_NONE  ─── ReadData → data > single frame ─→  PENDING_READ_DATA
                                                     │ reader sends 0xAF per chunk
                                                     └──────────────────→  PENDING_NONE

PENDING_NONE  ─── ReadRecords (similar) ──────────→  PENDING_READ_RECORDS
```

When `s_pending_cmd != PENDING_NONE` and `on_apdu` receives `INS=0xAF` with no data (`lc==0`), the chaining path is taken. When `s_pending_cmd != PENDING_NONE` and any non-0xAF APDU arrives, reset pending state and service the new command (per DeSFire spec: a new command cancels a pending chain).

---

## 5. Integration Points

### 5.1 Down: Calls This Layer Makes

| Dependency | Function | When called |
|-----------|---------|-------------|
| HAL | `nfc_transport_send_response(s_resp_buf, len)` | End of every `on_select` and `on_apdu` |
| PSA Crypto | `psa_generate_random(rnd_b, 16)` | Auth STEP1 entry |
| PSA Crypto | `psa_import_key(attrs, key_bytes, 16, &key_id)` | Auth STEP1 — import card key as volatile AES key |
| PSA Crypto | `psa_cipher_*` (ECB, CBC) | Auth STEP1/STEP2 encrypt/decrypt |
| PSA Crypto | `psa_mac_*` (CMAC) | EV2 session key derivation |
| PSA Crypto | `psa_destroy_key(key_id)` | Auth STEP2 (success or failure), field-off |
| Zephyr SMF | `smf_set_state(SMF_CTX(&s_auth), state)` | Auth FSM transitions |
| Zephyr SMF | `smf_run_state(SMF_CTX(&s_auth))` | From `desfire_cmd_additional_frame()` |

All PSA calls are made on `nfc_work_q`. PSA Crypto on nRF54L15 is accelerated by **CRACEN** (the on-chip crypto engine; PSA driver `CONFIG_PSA_CRYPTO_DRIVER_CRACEN`). Single AES-128 block operation timing: exact numbers **to be measured on nRF54L15 target**; expected to be ≤ 100 µs (comparable to CC310 on nRF52840). Full 3-pass EV1 auth (4 AES ops): expected ≤ 500 µs. FWI=8 (~5 s budget): no timing risk.

> **DECISION-G (Crypto timing):** PSA AES-128 runs synchronously on `nfc_work_q` inside `on_apdu`. No deferred work submission is needed. Expected budget: ≤ 500 µs for the full 3-pass auth (4 AES-128-CBC/ECB operations via PSA + **hardware AES via CRACEN on nRF54L15**; exact timing to-be-measured on target). FWI=8 allows ~5 s. This is consistent with how Aliro handles crypto on the same work queue. Unlike Aliro (which defers a multi-step P-256 exchange taking ~200 ms), DeSFire AES is fast enough for synchronous dispatch.

### 5.2 Up: How This Layer Is Consumed

| Consumer | Mechanism | Notes |
|---------|---------|-------|
| `nfc_stack.c` | `desfire_service_init()` during stack init | Called before `nfc_stack_start()` if `IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)` |
| `nfc_stack.c` | `desfire_service_get()` → `aid_router_register(k_desfire_aid, 7, svc)` | Registers AID `D2 76 00 00 85 01 00` with the router |
| `nfc_stack.c` | `desfire_service_shutdown()` during stack shutdown | Wave 4 §3 rollback path |
| `nfc_store` (Wave 6) | `svc->serialize()` / `svc->deserialize()` | Only on quiesced stack (`-EBUSY` when STARTED) |
| `aid_router` | `on_select / on_apdu / on_deselect / on_field_off` | Invoked on `nfc_work_q` |
| Shell | `desfire_service_get_stats()` / `get_config()` / `get_state()` | Via `desfire_service_shell_cmds.c` |

### 5.3 RESERVED — `desfire_poller` (Reader Role)

> **Not implemented in this slice.** When a reader-capable backend becomes available
> (`CONFIG_NFC_ROLE_READER` + ST25R3916/RFAL or PN7160), the DeSFire protocol module
> gains a `desfire_poller` component that reads a physical DeSFire card into the same
> `desfire_data_t` model that this listener serves.
> **Key-cloneability caveat:** a poller can capture card structure (application IDs,
> file metadata, plain-access file data) but **never the secret AES keys** —
> authenticated DeSFire cards are not key-cloneable. The serialized blob would carry
> a `NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL` completeness flag (spec v3 §4.5 / wave6-store.md §1.7).
> **Header/interface placeholder only** — no `desfire_poller` API is declared or
> compiled in this slice. When `CONFIG_NFC_ROLE_READER` is added, the poller source
> compiles under that gate, orthogonal to `CONFIG_NFC_ROLE_CARD`.

---

## 6. Implementation Notes

### 6.1 Command Dispatch Architecture

The `desfire_on_apdu()` function is a single switch on `apdu->ins`. It calls a dedicated
`desfire_cmd_<name>()` static function for each command, each of which builds the response
into `s_resp_buf` and calls `nfc_transport_send_response(s_resp_buf, len)`.

```c
static void desfire_on_apdu(const nfc_apdu_t *apdu, void *user_ctx)
{
    (void)user_ctx;
    STATS_INC(&s_stats_lock, s_stats, apdu_count);

    /* All post-SELECT DeSFire commands arrive with CLA=0x90. */
    if (apdu->cla != NFC_CLA_DESFIRE) {
        send_desfire_status(DESFIRE_STATUS_ILLEGAL_CMD);
        STATS_INC(&s_stats_lock, s_stats, illegal_cmd_count);
        return;
    }

    switch (apdu->ins) {
    case DESFIRE_CMD_GET_VERSION:         desfire_cmd_get_version(apdu);        break;
    case DESFIRE_CMD_GET_FREE_MEMORY:     desfire_cmd_get_free_memory(apdu);    break;
    case DESFIRE_CMD_GET_KEY_SETTINGS:    desfire_cmd_get_key_settings(apdu);   break;
    case DESFIRE_CMD_GET_KEY_VERSION:     desfire_cmd_get_key_version(apdu);    break;
    case DESFIRE_CMD_GET_APPLICATION_IDS: desfire_cmd_get_app_ids(apdu);        break;
    case DESFIRE_CMD_SELECT_APPLICATION:  desfire_cmd_select_app(apdu);         break;
    case DESFIRE_CMD_GET_FILE_IDS:        desfire_cmd_get_file_ids(apdu);       break;
    case DESFIRE_CMD_GET_FILE_SETTINGS:   desfire_cmd_get_file_settings(apdu);  break;
    case DESFIRE_CMD_READ_DATA:           desfire_cmd_read_data(apdu);          break;
    case DESFIRE_CMD_GET_VALUE:           desfire_cmd_get_value(apdu);          break;
    case DESFIRE_CMD_READ_RECORDS:        desfire_cmd_read_records(apdu);       break;
    case DESFIRE_CMD_AUTHENTICATE_AES:    desfire_cmd_auth_aes(apdu);           break;
    case DESFIRE_CMD_AUTH_EV2_FIRST:      desfire_cmd_auth_ev2_first(apdu);     break;
    case DESFIRE_CMD_AUTH_EV2_NON_FIRST:  desfire_cmd_auth_ev2_non_first(apdu); break;
    case DESFIRE_CMD_ADDITIONAL_FRAME:    desfire_cmd_additional_frame(apdu);   break;
    default:
        send_desfire_status(DESFIRE_STATUS_ILLEGAL_CMD);
        STATS_INC(&s_stats_lock, s_stats, illegal_cmd_count);
        break;
    }
}
```

**Response builder helpers** (all file-static, all pure — ztest-able; prefer `qemu_cortex_m33` or DK hardware for local runs; `native_sim` is Linux-CI-only in this repo):

```c
/* Write {data[len], 0x91, status} into s_resp_buf; call nfc_transport_send_response(). */
static void send_desfire_response(const uint8_t *data, size_t data_len, uint8_t status);

/* Write {0x91, status} (no data payload). */
static void send_desfire_status(uint8_t status);
```

### 6.2 EV1 AES Authentication Flow (3-Pass)

**Auth STEP1 — triggered by `AUTHENTICATE_AES (0xAA)` INS:**

1. Extract `key_no` from `apdu->data[0]`. Validate range.
2. Look up the key: if `s_selected_app_idx < 0` → master key from `s_card_data.master_key`; else → `s_card_data.apps[s_selected_app_idx].keys[key_no]`. If key_no out of range → `0x91 40`.
3. Call `psa_generate_random(s_auth.rnd_b, DESFIRE_RND_SIZE)`.
4. Import card key as volatile PSA key: `psa_import_key(attrs_ecb_encrypt_decrypt, key_bytes, 16, &s_auth.psa_key_id)`.
5. Encrypt `rnd_b` using AES-128-ECB (IV=0): `psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, rnd_b, 16, enc_rnd_b, ...)`.
6. Build response: `{enc_rnd_b[16], 0x91, 0xAF}` → `send_desfire_response(enc_rnd_b, 16, 0xAF)`.
7. Transition auth FSM: `smf_set_state(SMF_CTX(&s_auth), &k_auth_states[STEP1])`.
8. Set `s_auth.auth_key_no = key_no`, `s_auth.is_picc_auth`, `s_auth.is_ev2 = false`.

**Auth STEP2 — triggered by `ADDITIONAL_FRAME (0xAF)` with 32 bytes data while in STEP1:**

1. Check `apdu->lc == 32`. If not: send `0x91 7E` (LENGTH_ERROR), reset FSM to IDLE.
2. Decrypt reader's combined frame using AES-128-CBC (IV = enc_rnd_b from step 1):
   - `psa_cipher_decrypt(key_id, PSA_ALG_CBC_NO_PADDING, enc_rnd_b_as_iv + reader_data_32, ...)` → `plain[32]` = `{RndA[16] || rot_left(RndB)[16]}`.
   - Verify `plain[16..31] == rot_left(s_auth.rnd_b)`. If not: send `0x91 AE`, reset FSM to IDLE, `psa_destroy_key`.
3. Store `s_auth.rnd_a[0..15] = plain[0..15]`.
4. Transition to STEP2: `smf_set_state(SMF_CTX(&s_auth), &k_auth_states[STEP2])`.
5. SMF STEP2 run (still same on_apdu call):
   - Compute `rot_left_rnd_a[16]` = `{rnd_a[1..15], rnd_a[0]}`.
   - Encrypt using AES-128-CBC (IV = last ciphertext block from step 2 decrypt):
     `psa_cipher_encrypt(key_id, PSA_ALG_CBC_NO_PADDING, iv_last_block+rot_left_rnd_a, ...)` → `enc_rnd_a_rot[16]`.
   - Derive EV1 session key: `{rnd_a[0..3], rnd_b[0..3], rnd_a[12..15], rnd_b[12..15]}` → 16 bytes → store in `s_auth.session_key`.
   - `psa_destroy_key(s_auth.psa_key_id)`. Set `s_auth.psa_key_id = PSA_KEY_ID_NULL`.
   - Send `{enc_rnd_a_rot[16], 0x91, 0x00}`.
   - Transition to AUTHENTICATED: `smf_set_state(SMF_CTX(&s_auth), &k_auth_states[AUTHENTICATED])`.
   - `STATS_INC(auth_completed_count)`.

**Note on AES-CBC IV handling:** The PSA CBC API uses an explicit IV prepended to the ciphertext. For step 2 encryption, the IV is the last 16-byte ciphertext block from the decryption in step 2 (standard DeSFire EV1 CBC chaining). The implementation must track `iv_for_next_step` between step 1 and step 2 in `s_auth`.

### 6.3 EV2 Authentication Flow (AuthenticateEV2First 0x71)

EV2 uses CMAC-AES instead of ECB/CBC and derives two session keys (`SesAuthENC`, `SesAuthMAC`). The FSM structure is identical: STEP1 after sending card challenge, STEP2 on receipt of reader response, AUTHENTICATED on success.

Key differences from EV1:
- STEP1: Card generates RndB, sends `AES-ECB(key, RndB)` + `0x91AF` (same as EV1 first step).
- STEP2: Reader sends `AES-CBC_enc({RndA || rot(RndB)}, IV=0)`. Card decrypts, verifies rot(RndB), encrypts `rot(RndA)`, sends `{enc_rot_RndA, 0x91, 0x00}`.
- Session key derivation (EV2): Uses CMAC-AES on a KDF input. `SesAuthENC = CMAC(key, 0xA55A || RndA[0..7] || RndB[0..7] || ...)`, `SesAuthMAC = CMAC(key, 0x5AA5 || ...)` — exact KDF per NXP AN12343.
- PSA CMAC: `psa_mac_compute(key_id, PSA_ALG_CMAC, kdf_input, kdf_len, out, 16, &out_len)`.
- Auth STEP1/2 shared state: `s_auth.is_ev2 = true`; STEP2 uses CMAC path.

> **DECISION-B (EV2 scope):** AuthenticateEV2First (0x71) and AuthenticateEV2NonFirst (0x77) are supported. EV2NonFirst re-authenticates with a different app key within an existing EV2 session; it requires an active AUTHENTICATED state (else 0x91AE). Both share the same FSM topology. The EV2 session key KDF uses PSA CMAC. The implementation notes the NXP AN12343 KDF constants as literal arrays in the source with a comment citing the application note.

### 6.4 GetVersion ADDITIONAL_FRAME Chaining (DECISION-E)

`GetVersion` returns hardware version, software version, and UID/batch/production info in three consecutive APDU exchanges:

```
Reader → Card : CLA=0x90 INS=0x60 P1=0x00 P2=0x00  (GetVersion)
Card   → Reader: [hw_version[7]] [0x91][0xAF]          (part 1 — more to follow)
Reader → Card : CLA=0x90 INS=0xAF                    (AdditionalFrame)
Card   → Reader: [sw_version[7]] [0x91][0xAF]          (part 2)
Reader → Card : CLA=0x90 INS=0xAF                    (AdditionalFrame)
Card   → Reader: [uid[7]][batch[5]][prod_week][prod_year] [0x91][0x00]  (part 3 — done)
```

Implementation:
- `desfire_cmd_get_version()`: sends part 1, sets `s_pending_cmd = PENDING_GET_VERSION`, `s_pending_sub_frame = 1`.
- `desfire_cmd_additional_frame()`: checks `s_pending_cmd`:
  - `PENDING_GET_VERSION`: sends part `s_pending_sub_frame + 1`; increments; sets PENDING_NONE on sub_frame==2.
  - `PENDING_READ_DATA` / `PENDING_READ_RECORDS`: sends next `min(remain, CONFIG_NFC_APDU_BUF_SIZE - 4)` bytes; updates `s_pending_offset`; sends 0x9100 (not 0x91AF) on last chunk.
  - `PENDING_NONE` and no auth in progress: `0x91CA` (COMMAND_ABORTED).
  - `PENDING_NONE` and auth STEP1 in progress: route to SMF STEP2 processing.
- Precedence rule: auth frames (0xAF with data) take priority over data continuation (0xAF with no data); `apdu->lc > 0` distinguishes the two cases.

> **DECISION-K (GetApplicationIDs single-frame):** With `MAX_APPS=4`, the AID list is at most 4 × 3 = 12 bytes, fitting in one APDU response frame. No chaining needed. If `MAX_APPS` is increased to ≥ 170 via Kconfig, chaining becomes required — flag with `BUILD_ASSERT(CONFIG_NFC_DESFIRE_MAX_APPS * 3U + 2U <= 256U, "GetApplicationIDs would overflow single frame")` or implement chaining conditionally. Default is well within limit.

> **DECISION-J (GetFreeMemory — static value):** `desfire_data_t.free_memory` is populated during `desfire_service_init()` from `CONFIG_NFC_DESFIRE_FREE_MEMORY` (Kconfig int, default 7520, representing ~7.5 KB for a typical EV1 2K card). It is also serialized/deserialized so provisioning can override it. The service does not track dynamic allocation — it is an emulator, not a real EEPROM.

### 6.5 Response Encoding

DeSFire native response format over ISO-DEP:

```
[data_byte_0] ... [data_byte_N-1] [SW1=0x91] [SW2=status_code]
```

Where `SW2` is a DeSFire status byte (§1.2). A zero-data response (e.g. SelectApplication success) is `{0x91, 0x00}` (2 bytes). This differs from ISO 7816-4 `9000` but both use the same 2-byte trailer position in the APDU response.

The `send_desfire_response()` helper:
```c
static void send_desfire_response(const uint8_t *data, size_t data_len, uint8_t status)
{
    /* s_resp_buf is static, 512 bytes, guaranteed > any single DeSFire frame. */
    __ASSERT_NO_MSG(data_len + 2U <= sizeof(s_resp_buf));
    if (data_len > 0U) {
        (void)memcpy(s_resp_buf, data, data_len);
    }
    s_resp_buf[data_len]      = DESFIRE_SW1;   /* 0x91 */
    s_resp_buf[data_len + 1U] = status;
    (void)nfc_transport_send_response(s_resp_buf, data_len + 2U);
}
```

The response buffer (`s_resp_buf[512]`) is a file-static BSS array, not a stack local. It persists between `on_apdu` calls; the HAL borrows it until the next callback (CONVENTIONS §5). The service must not overwrite it until `on_apdu` is called again (guaranteed: single WQ thread, sequential callbacks).

### 6.6 Serialization Format (Wave 6 contract)

Serialize / deserialize use a flat binary format versioned at byte 0.

```
Format version 1 layout:
  [0]         format_version  : uint8_t = 1
  [1..7]      hw_version      : uint8_t[7]
  [8..14]     sw_version      : uint8_t[7]
  [15..21]    uid             : uint8_t[7]
  [22..26]    batch           : uint8_t[5]
  [27]        prod_week       : uint8_t
  [28]        prod_year       : uint8_t
  [29..32]    free_memory     : uint32_t LE
  [33..48]    master_key      : uint8_t[16]
  [49]        master_key_settings : uint8_t
  [50]        app_count       : uint8_t
  for each app (app_count times):
    [+0..+2]  app_id          : uint8_t[3]
    [+3]      key_settings    : uint8_t
    [+4]      key_count       : uint8_t (0..14)
    [+5]      file_count      : uint8_t (0..MAX_FILES_PER_APP)
    [+6..+6+key_count*16-1]   keys : uint8_t[key_count][16]
    for each file (file_count times):
      [+0]    file_id         : uint8_t
      [+1]    type            : uint8_t
      [+2]    comm            : uint8_t
      [+3..+4]access_rights   : uint16_t LE
      [+5..+12] settings_union : uint8_t[8] (fixed-size, type-tagged encoding)
      [+13..+14] data_len     : uint16_t LE (actual bytes stored, ≤ MAX_FILE_SIZE)
      [+15..+15+data_len-1] file_data : uint8_t[data_len]
```

`deserialize()` reads `format_version`; if not `1`, returns `-EBADMSG`. Field bounds
are checked against Kconfig limits before writing (`key_count > DESFIRE_MAX_KEYS` →
`-EBADMSG`; `file_count > CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP` → `-EBADMSG`;
`data_len > CONFIG_NFC_DESFIRE_MAX_FILE_SIZE` → `-EBADMSG`).

### 6.7 PSA Crypto Usage Summary

```c
/* Import ephemeral volatile AES-128 key for auth. */
psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
psa_set_key_bits(&attr, 128U);
psa_set_key_usage_flags(&attr,
    PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
psa_set_key_algorithm(&attr,
    PSA_ALG_ECB_NO_PADDING);   /* EV1 first step */
psa_import_key(&attr, card_key_bytes, 16U, &s_auth.psa_key_id);

/* For auth STEP2 (EV1 CBC mode): re-import or update algorithm attribute. */
psa_set_key_algorithm(&attr, PSA_ALG_CBC_NO_PADDING);
psa_import_key(&attr, card_key_bytes, 16U, &step2_key_id);

/* Random challenge generation. */
psa_generate_random(s_auth.rnd_b, DESFIRE_RND_SIZE);

/* Cleanup: always call on auth end, fail, field-off. */
if (s_auth.psa_key_id != PSA_KEY_ID_NULL) {
    (void)psa_destroy_key(s_auth.psa_key_id);
    s_auth.psa_key_id = PSA_KEY_ID_NULL;
}
```

Two separate key imports are needed: one ECB for encrypting `RndB` (STEP1), one CBC for encrypting/decrypting the combined challenge (STEP2). Alternatively, PSA multipart cipher API (`psa_cipher_operation_t`) can set the IV explicitly without needing two key imports. **Preferred approach:** single ECB key import; use `psa_cipher_encrypt()` for STEP1; destroy and re-import as CBC algorithm for STEP2. This avoids holding two key slots simultaneously.

Session key material (`s_auth.session_key`, `s_auth.enc_key`, `s_auth.mac_key`) is stored in BSS, not in PSA key store. Rationale: session keys are transient (cleared on field-off) and only used within `on_apdu` on the WQ thread — no cross-thread sharing.

---

## 7. Conventions Compliance

Per CONVENTIONS §12 checklist:

- [x] **Lifecycle matches §2:** Minimal (init/shutdown only). No `deinit`. No `start`/`stop`. ✓
- [x] **`config_t` / `stats_t` / `state_t` + three getters defined (§2):** `desfire_service_config_t` + `desfire_service_stats_t` + `desfire_service_state_t`; `get_config()`, `get_stats(out)`, `get_state()`. ✓
- [x] **State storage Pattern A (§2):** `s_state` is a plain enum; WQ thread only. Auth FSM is a separate `struct smf_ctx`-embedded state object. ✓
- [x] **Coupling matches §3 and §4:** Service registered with router via `aid_router_register()` in `nfc_stack.c`. Callbacks typed `_fn`, stored and invoked via `nfc_service_t` vtable. `user_ctx` named correctly (NULL for singleton). All callbacks are `@nfc_work_q`. ✓
- [x] **Buffer model per §5:** Inbound: borrows `apdu->data` zero-copy from net_buf (valid for on_apdu duration — data copied before return where needed). Outbound: `s_resp_buf[512]` file-static array, never a stack local. ✓
- [x] **Stats recipe per §6:** `static desfire_service_stats_t s_stats` + `static struct k_spinlock s_stats_lock` (NOT `K_SPINLOCK_DEFINE`). `STATS_RESET(s_stats)` first in `init()` body after `-EALREADY` guard. `STATS_INC` / `STATS_ERROR` on all paths. Copy-out getter via `STATS_COPY_OUT`. ✓
- [x] **Threading + annotations per §7:** All vtable callbacks `@nfc_work_q`. `init()`, `shutdown()`, `get_config()`, `get_stats()`, `get_state()` are `@caller_sync`. `get()` is `@threadsafe`. No sleeping. ✓
- [x] **Error codes per §9:** `-EALREADY` (double init), `-ENODEV` (shutdown before init), `-EINVAL` (NULL out), `-ENOSPC` (serialize max too small), `-EBADMSG` (deserialize corrupt). No invented codes. ✓
- [x] **Shell per §10:** `desfire_service_shell_cmds.c` registers `SHELL_CMD_REGISTER(desfire, ...)` with sub-commands `config`, `stats`, `state`. ✓
- [x] **persist_id per wave3 §1.1:** `NFC_PERSIST_ID_DESFIRE = 0x02U` (stable, from service.h table). `serialize` / `deserialize` non-NULL (DeSFire card data is the primary persistent state). ✓
- [x] **MISRA + DEV-ZEP citations (§11):** See below.

### MISRA C:2012 Notes + DEV-ZEP Citations

1. **`LOG_*` macros** in `desfire_service.c` → **DEV-ZEP-008** (Rule 20.1 / variadic macro).
2. **`shell_print` variadic macro** in `desfire_service_shell_cmds.c` → **DEV-ZEP-009** (Rule 20.1 / Rule 19.10).
3. **`ARG_UNUSED(user_ctx)`** in all vtable callbacks (signature fixed by `nfc_service_t`) → **DEV-ZEP-016** (Rule 2.7; unused parameter in framework callback).
4. **Zephyr SMF: `struct smf_ctx ctx` must be the first member** of `desfire_auth_session_t`. Zephyr's SMF internally accesses the user object via the `smf_ctx` pointer (guaranteed first-member layout per SMF ABI). Where `SMF_CTX(&s_auth)` is used, it expands to `&(s_auth).ctx` — plain member access, no CONTAINER_OF needed on the user side. However, Zephyr's internal SMF `smf_run_state()` implementation itself uses `CONTAINER_OF` on the `smf_ctx *` to reach the parent state → this is internal Zephyr code, covered by **DEV-ZEP-001** (Rule 11.3 / pointer arithmetic), but transparent to the user code in this module. User code only calls `smf_set_state(SMF_CTX(&s_auth), &state)` and `smf_run_state(SMF_CTX(&s_auth))` — both are standard function calls.
5. **`void *obj` cast in SMF state handlers** (`entry`/`run`/`exit` receive `void *obj`) — cast to `desfire_auth_session_t *`. This is a Rule 11.5 deviation ("A conversion should not be performed from pointer to void into pointer to object"). In Zephyr SMF this is the framework-mandated pattern (SMF passes the user object through `void *`). Deviation accepted: the pointer always points to `s_auth` (a static object of correct type); the cast is safe and the only way to use Zephyr SMF. No specific DEV-ZEP number — document inline. Pattern is identical to `k_work` callback `user_data` casts used across the codebase.
6. **`IS_ENABLED(CONFIG_NFC_SERVICE_DESFIRE)`** — evaluated only in `nfc_stack.c`, not in this service. No deviation needed here.
7. **PSA Crypto API return value checks:** All `psa_*` return values are checked and failures recorded via `STATS_ERROR`. Rule 17.7 (return values checked) is satisfied. On PSA failure, send `0x91 AE` (auth error) or `0x91 1E` (integrity error) as appropriate.
8. **`memcpy` / `memset`** in serialize / deserialize — Rule 21.6 (standard library use) is satisfied; these are legitimate uses under Zephyr's allowed stdlib subset.
9. **`switch` default clauses:** Every `switch` in dispatch and serialization has a `default` case. Rule 16.4. ✓
10. **Integer suffix rule (Rule 7.2):** All unsigned constants use the `U` suffix (e.g. `0x91U`, `16U`, `DESFIRE_RND_SIZE`). ✓
11. **No dynamic allocation in running phase:** All buffers are file-static (`s_resp_buf`, `s_pending_buf`, PSA volatile key is managed by PSA, not via `k_malloc`). Rule Dir 4.12 (no dynamic memory). ✓
12. **`BUILD_ASSERT` for sizing invariants** → **DEV-ZEP-015** (Rule 20.1): `BUILD_ASSERT(DESFIRE_SERVICE_MAX_SERIALIZED > sizeof(desfire_data_t), ...)` and `BUILD_ASSERT(sizeof(s_resp_buf) >= CONFIG_NFC_DESFIRE_MAX_FILE_SIZE + 32U, ...)`.

---

## 8. Tasks

### Phase 0 — File Scaffolding

- [ ] **Task 0.1 — Kconfig.** Create `src/nfc/services/desfire/Kconfig`. Content:

```kconfig
# NFC DeSFire service — tunables (service enable in src/nfc/Kconfig)

config NFC_DESFIRE_MAX_APPS
    int "Maximum DeSFire applications"
    depends on NFC_SERVICE_DESFIRE
    default 4
    range 1 28
    help
      Maximum number of DeSFire applications in the static data model.
      Each application reserves keys[14][16] + file_data[MAX_FILES][MAX_FILE_SIZE].
      RAM cost per app ≈ 224 + MAX_FILES_PER_APP * (16 + MAX_FILE_SIZE + 2 + 1) bytes.
      Flipper/real card limit is 28; keep at 4 unless you have ample RAM.

config NFC_DESFIRE_MAX_FILES_PER_APP
    int "Maximum files per DeSFire application"
    depends on NFC_SERVICE_DESFIRE
    default 8
    range 1 32
    help
      Maximum file slots per application. Each slot: 1 (id) + 24 (settings) +
      2 (data_len) + MAX_FILE_SIZE bytes.

config NFC_DESFIRE_MAX_FILE_SIZE
    int "Maximum DeSFire file data size in bytes"
    depends on NFC_SERVICE_DESFIRE
    default 256
    range 16 512
    help
      Maximum stored bytes per file. Bounded by CONFIG_NFC_APDU_BUF_SIZE (default 512)
      minus APDU overhead. Values > 496 require ADDITIONAL_FRAME chaining for ReadData.

config NFC_DESFIRE_FREE_MEMORY
    int "Static free-memory value reported by GetFreeMemory"
    depends on NFC_SERVICE_DESFIRE
    default 7520
    help
      Emulated free NV memory in bytes returned by GET_FREE_MEMORY (0x6E).
      7520 = typical EV1 2K card minus overhead. Adjust to match the card
      being emulated.
```

- [ ] **Task 0.2 — CMakeLists.txt.** Create `src/nfc/services/desfire/CMakeLists.txt`:

```cmake
# src/nfc/services/desfire/CMakeLists.txt
zephyr_library_named(nfc_desfire)
zephyr_library_sources(
    desfire_service.c
    desfire_service_shell_cmds.c
)
zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZEPHYR_BASE}/include
)
```

Parent `src/nfc/CMakeLists.txt` already has `if(CONFIG_NFC_SERVICE_DESFIRE) add_subdirectory(services/desfire) endif()` (wave4 §5.5). Confirm this exists and matches. **Commit.**

---

### Phase 1 — Types and Data Model (TDD-red phase)

- [ ] **Task 1.1 — `desfire_data.h`.** Create `src/nfc/services/desfire/desfire_data.h` with the full data model as specified in §1.5. Include guards, `#include <stdint.h>`, `#include <stdbool.h>`, `#include "desfire_service.h"`. No Zephyr kernel includes. **Compile-check only (no link).**

- [ ] **Task 1.2 — `desfire_service.h`.** Create `src/nfc/services/desfire/desfire_service.h` with:
  - `DESFIRE_CMD_*` constants (§1.1)
  - `DESFIRE_STATUS_*` constants (§1.2)
  - `DESFIRE_SW1`, key/size constants (§1.3)
  - `DESFIRE_SERVICE_MAX_SERIALIZED` (§1.3)
  - `desfire_file_type_t`, `desfire_comm_t` enums (§1.4)
  - `desfire_service_config_t`, `desfire_service_stats_t`, `desfire_service_state_t`, `desfire_auth_state_t` (§1.6)
  - Public API declarations (§2)
  Include `"framing/apdu_types.h"`, `"router/service.h"`, `<psa/crypto.h>`. **Commit.**

- [ ] **Task 1.3 — Write tests for the data model / helpers (TDD-red).** Create `tests/unit/nfc_desfire/test_desfire_frame.c`:
  - `test_send_desfire_status_ok`: verify that `{0x91, 0x00}` is what gets sent on status OK.
  - `test_send_desfire_response_with_data`: verify `{data..., 0x91, 0xAF}` formatting.
  - `test_rot_left_16`: verify byte rotation helper.
  - `test_session_key_derivation_ev1`: test EV1 session key = `{RndA[0..3] || RndB[0..3] || RndA[12..15] || RndB[12..15]}` with known test vectors.
  - `test_serialize_deserialize_roundtrip`: serialize a populated `desfire_data_t`, then deserialize into a fresh one; compare field by field.
  - `test_deserialize_version_mismatch`: `in[0] = 0xFF` → `-EBADMSG`.
  - `test_app_find_by_id`: pure array search; found and not-found cases.
  - `test_file_find_by_id`: pure array search in app; found and not-found cases.
  Run: `west twister -T tests/unit/nfc_desfire --platform qemu_cortex_m33 --no-sysbuild`. All should FAIL (red). (`native_sim` is Linux-CI-only; use `qemu_cortex_m33` locally or hardware Ztest on DK.) **Commit (failing tests).**

---

### Phase 2 — Core Implementation (TDD-green phase)

- [ ] **Task 2.1 — `desfire_service.c` skeleton.** Create the file with:
  - `#include "desfire_service.h"`, `#include "desfire_data.h"`, `#include <zephyr/smf.h>`, `#include <zephyr/logging/log.h>`, `#include "src/stats.h"`, etc.
  - All file-static state declarations (§4.1).
  - SMF state table skeleton (`k_auth_states[4]` with stub entry/run/exit).
  - Stub vtable callbacks (each calls `send_desfire_status(DESFIRE_STATUS_ILLEGAL_CMD)`).
  - `LOG_MODULE_REGISTER(desfire_svc, CONFIG_NFC_LOG_LEVEL)`.
  - Stub `desfire_service_init/shutdown/get/get_config/get_stats/get_state`. **Compile to zero warnings.** **Commit.**

- [ ] **Task 2.2 — `send_desfire_response` and `send_desfire_status` helpers.** Implement both using `s_resp_buf`. Add `BUILD_ASSERT` guards. Write helper for `rot_left_16(const uint8_t *in, uint8_t *out)`. Make the frame tests green. **Commit.**

- [ ] **Task 2.3 — `desfire_service_init` and `desfire_service_shutdown`.** Implement:
  - `init(cfg)`: `-EALREADY` guard, `STATS_RESET(s_stats)`, populate `s_config` from `cfg` (NULL → Kconfig defaults), populate `s_card_data.free_memory = CONFIG_NFC_DESFIRE_FREE_MEMORY`, set `s_state = INITIALIZED`. Initialize SMF: `smf_set_initial(SMF_CTX(&s_auth), &k_auth_states[DESFIRE_AUTH_STATE_IDLE])`.
  - `shutdown()`: idempotent (no ENODEV; no-op if already UNINITIALIZED), destroy PSA key if live, clear session, set `s_state = UNINITIALIZED`, return 0.
  - Three getters: pattern per CONVENTIONS §2 / §6 (copy-out for stats). **Commit.**

- [ ] **Task 2.4 — `on_select`.** Implement: reset session, send `{0x90, 0x00}`, `STATS_INC(select_count)`. **Commit.**

- [ ] **Task 2.5 — Pure lookup helpers.** Implement and verify with tests:
  - `app_find_by_id(const desfire_data_t *d, const uint8_t *app_id, int8_t *out_idx)` → 0 / -ENOENT.
  - `file_find_by_id(const desfire_application_t *app, uint8_t file_id, uint8_t *out_idx)` → 0 / -ENOENT.
  Make `test_app_find_by_id` and `test_file_find_by_id` green. **Commit.**

- [ ] **Task 2.6 — PICC-level commands without auth.** Implement:
  - `desfire_cmd_get_version()` — sends part 1 (hw_version[7] + 0x91AF); sets `s_pending_cmd = PENDING_GET_VERSION`, `s_pending_sub_frame = 1`.
  - `desfire_cmd_additional_frame_get_version()` — sends parts 2 and 3; clears pending on part 3.
  - `desfire_cmd_get_free_memory()` — sends `{free_mem_LE[4], 0x91, 0x00}`.
  - `desfire_cmd_get_key_settings()` — sends `{key_settings[1], max_keys[1], 0x91, 0x00}` (app or PICC level).
  - `desfire_cmd_get_key_version()` — validates key_no; sends `{key_ver[1], 0x91, 0x00}`.
  - `desfire_cmd_get_app_ids()` — serializes all app IDs into `s_resp_buf` as N×3 bytes + 0x9100.
  **Commit.**

- [ ] **Task 2.7 — `desfire_cmd_select_app`.** Validate Lc=3, extract 3-byte AID, call `app_find_by_id`; on success: set `s_selected_app_idx`, reset auth FSM to IDLE, send 0x9100; on failure: send 0x91A0. **Commit.**

- [ ] **Task 2.8 — App-context read commands.** Implement:
  - `desfire_cmd_get_file_ids()` — guard no-app (0x91A0); list `file_ids[0..file_count-1]` + 0x9100.
  - `desfire_cmd_get_file_settings()` — guard no-app + file lookup; serialize settings into 16-byte block + 0x9100.
  - `desfire_cmd_read_data()` — guard no-app + file lookup + type check (Standard/Backup only; else 0x91 1C) + auth check per `comm` field (DECISION-I) + offset/length from APDU data; copy file bytes into `s_pending_buf`; if ≤ one frame: send immediately; else: set `s_pending_cmd = PENDING_READ_DATA`, send first chunk + 0x91AF.
  - `desfire_cmd_get_value()` — guard no-app + file lookup + type check (Value only) + auth check; send `{value_LE[4], 0x91, 0x00}`.
  - `desfire_cmd_read_records()` — guard no-app + file lookup + type check (LinearRecord/CyclicRecord) + auth check; similar chunking to ReadData. **Commit.**

> **DECISION-I (auth check for file reads):** For `ReadData`, `GetValue`, `ReadRecords`: if file `comm == DESFIRE_COMM_PLAIN` → no auth required; if `comm == DESFIRE_COMM_MAC` or `comm == DESFIRE_COMM_ENCRYPTED` → require `s_auth.state == DESFIRE_AUTH_STATE_AUTHENTICATED`, else respond `0x91AE`. Rationale: this is the spec minimum. Full per-key access-rights checking (4-bit key index per file) would require a key-to-authentication mapping that adds complexity without value for an emulator where the card content is provisioned by the operator. Flag as simplification.

---

### Phase 3 — Auth FSM (TDD-critical)

- [ ] **Task 3.1 — Auth test vectors (TDD-red).** Add to `test_desfire_frame.c`:
  - `test_ev1_auth_step1`: given a known card key and a seeded RndB (mock `psa_generate_random`), verify the encrypted output matches an expected value computed offline with the same key.
  - `test_ev1_auth_step2_verify`: given `enc_rnd_b`, reader's `enc_challenge = AES_CBC({RndA || rot(RndB)})`, verify the implementation accepts it and produces correct `enc_rnd_a_rot`.
  - `test_ev1_session_key`: given `RndA` and `RndB`, verify `session_key = {RndA[0..3] || RndB[0..3] || RndA[12..15] || RndB[12..15]}`.
  - `test_ev1_auth_bad_rnd_b`: provide wrong `RndB` in reader's challenge → function returns auth failure.
  Use NIST AES-128 test vectors to verify the low-level PSA calls, then build the DeSFire-specific sequences. Run → all FAIL. **Commit.**

- [ ] **Task 3.2 — PSA crypto helpers.** Implement file-static functions:
  - `aes_ecb_encrypt(const uint8_t *key, const uint8_t *in, uint8_t *out)` → PSA_ALG_ECB_NO_PADDING.
  - `aes_cbc_encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *in, size_t len, uint8_t *out)`.
  - `aes_cbc_decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *in, size_t len, uint8_t *out)`.
  - `cmac_aes(const uint8_t *key, const uint8_t *msg, size_t msg_len, uint8_t *out)` → PSA_ALG_CMAC (for EV2).
  All functions return 0 on success, negative errno on PSA failure (`psa_status_t` mapped via a helper). **Commit.**

- [ ] **Task 3.3 — SMF auth states.** Implement `k_auth_states[]` with proper `run` functions for STEP1, STEP2, AUTHENTICATED (IDLE has no run logic). Implement `desfire_cmd_auth_aes()` (STEP1 entry) and the STEP2 path in `desfire_cmd_additional_frame()`. Make auth tests green. **Commit.**

- [ ] **Task 3.4 — EV2 auth.** Implement `desfire_cmd_auth_ev2_first()` and the EV2 branch in STEP2 (`is_ev2 = true`). Use `cmac_aes` for `SesAuthENC` / `SesAuthMAC` derivation. EV2NonFirst: check AUTHENTICATED state, then begin new auth round with different app key. Make any EV2 tests green. **Commit.**

- [ ] **Task 3.5 — Auth reset paths.** Implement `desfire_session_reset()` called from `on_field_off`, `on_deselect`, and `desfire_cmd_select_app()`. It: destroys PSA key if live, zeroes `s_auth.rnd_a/rnd_b/session_key/enc_key/mac_key`, calls `smf_set_state` to IDLE, sets `s_pending_cmd = PENDING_NONE`. **Commit.**

---

### Phase 4 — Persistence

- [ ] **Task 4.1 — `serialize` / `deserialize`.** Implement per §6.6. Write the round-trip test; make `test_serialize_deserialize_roundtrip` and `test_deserialize_version_mismatch` green. **Commit.**

---

### Phase 5 — Shell and Observability

- [ ] **Task 5.1 — `desfire_service_shell_cmds.c`.** Implement:

```c
SHELL_STATIC_SUBCMD_SET_CREATE(desfire_cmds,
    SHELL_CMD(config, NULL, "Print config", cmd_desfire_config),
    SHELL_CMD(stats,  NULL, "Print stats",  cmd_desfire_stats),
    SHELL_CMD(state,  NULL, "Print state",  cmd_desfire_state),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(desfire, &desfire_cmds, "DeSFire service", NULL);
```

`cmd_desfire_config`: prints max_apps, max_files_per_app, max_file_size.
`cmd_desfire_stats`: prints all `desfire_service_stats_t` fields.
`cmd_desfire_state`: prints state enum name + auth FSM state name + selected app index.
All use `shell_print` (DEV-ZEP-009), `ARG_UNUSED(argc/argv)` (DEV-ZEP-016). **Commit.**

---

### Phase 6 — Integration Verification

- [ ] **Task 6.1 — nfc_stack.c wiring (confirm).** Verify wave4 `nfc_stack.c` stubs compile correctly with the now-real `desfire_service_init/shutdown/get` signatures. Fix any signature mismatches. **Compile clean. Commit.**

- [ ] **Task 6.2 — BUILD_ASSERT guards.** Add in `desfire_service.c`:
  ```c
  BUILD_ASSERT(DESFIRE_SERVICE_MAX_SERIALIZED > sizeof(desfire_data_t),
               "DESFIRE_SERVICE_MAX_SERIALIZED too small for desfire_data_t");
  BUILD_ASSERT(sizeof(s_resp_buf) >= (size_t)CONFIG_NFC_DESFIRE_MAX_FILE_SIZE + 32U,
               "s_resp_buf too small for max file read response");
  BUILD_ASSERT((size_t)CONFIG_NFC_DESFIRE_MAX_APPS * DESFIRE_APP_ID_SIZE + 2U <= 256U,
               "GetApplicationIDs response exceeds single frame; implement chaining");
  ```
  **Compile clean. Commit.**

- [ ] **Task 6.3 — Full test suite run.** `west twister -T tests/unit/nfc_desfire --platform qemu_cortex_m33 --no-sysbuild`. All tests green. Document test coverage: frame builders, auth math (EV1), file lookup, serialize/deserialize round-trip. **Commit.**

- [ ] **Task 6.4 — ReadLints.** Run linter on all five desfire files. Fix any MISRA / style warnings. Verify DEV-ZEP suppressions cover all flagged macro uses. **Commit.**

- [ ] **Task 6.5 — DECISION flags review.** Walk through each DECISION-A through DECISION-L; confirm the implementation reflects the decision exactly; add inline `/* DECISION-X: ... */` comments at the relevant code site. **Commit.**

---

## Appendix A: ADDITIONAL_FRAME Dispatch Logic (Pseudocode)

```c
static void desfire_cmd_additional_frame(const nfc_apdu_t *apdu)
{
    /* Auth in progress (STEP1 waiting for reader's challenge) — auth data has lc>0. */
    if (s_auth.state == DESFIRE_AUTH_STATE_STEP1 && apdu->lc > 0U) {
        /* Route to SMF STEP2. */
        (void)smf_run_state(SMF_CTX(&s_auth));
        return;
    }

    /* Data chaining continuation — reader sends 0x90AF with lc==0. */
    if (s_pending_cmd == PENDING_GET_VERSION) {
        desfire_cmd_additional_frame_get_version();
        return;
    }
    if (s_pending_cmd == PENDING_READ_DATA ||
        s_pending_cmd == PENDING_READ_RECORDS) {
        desfire_cmd_send_pending_chunk();
        return;
    }

    /* No pending state and not in auth STEP1 — protocol error. */
    send_desfire_status(DESFIRE_STATUS_CMD_ABORTED);
    STATS_INC(&s_stats_lock, s_stats, illegal_cmd_count);
}
```

---

## Appendix B: AES-CBC IV Tracking for EV1 STEP2

DeSFire EV1 uses CBC chaining across auth steps. The IV for STEP2 encryption is the last
16-byte ciphertext block from the STEP2 decryption (reader's combined frame). This must
be captured during decryption and stored in `s_auth` for use in the STEP2 encrypt call.

Add field `uint8_t iv_for_enc[DESFIRE_RND_SIZE]` to `desfire_auth_session_t`:

```c
/* After decrypting reader's combined frame via AES-CBC: */
/* The last 16 bytes of the input ciphertext become the IV for the card's response. */
(void)memcpy(s_auth.iv_for_enc,
             apdu->data + apdu->lc - DESFIRE_RND_SIZE,
             DESFIRE_RND_SIZE);
```

Then in STEP2 encryption:
```c
aes_cbc_encrypt(card_key, s_auth.iv_for_enc, rot_left_rnd_a, DESFIRE_RND_SIZE, enc_out);
```
