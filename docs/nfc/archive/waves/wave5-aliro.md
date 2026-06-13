# Wave 5e: Aliro Credential Service Implementation Plan

**Status:** DRAFT — 2026-06-12

**Layer:** `services/aliro`

**Files produced:**
- `src/nfc/services/aliro/aliro_service.h`
- `src/nfc/services/aliro/aliro_service.c`
- `src/nfc/services/aliro/aliro_service_shell_cmds.c`
- `src/nfc/services/aliro/CMakeLists.txt`
- `src/nfc/services/aliro/Kconfig`
- `tests/unit/nfc_aliro/` (ztest: FSM decision table + command-parser unit tests; Twister tag `sigmation.nfc.aliro`; `native_sim` is Linux-CI-only — use `qemu_cortex_m33` or DK locally)

**Depends on (all LOCKED — 2026-06-12):**
- `docs/superpowers/plans/wave1-hal.md`
- `docs/superpowers/plans/wave2-framing.md`
- `docs/superpowers/plans/wave3-router.md`
- `docs/superpowers/plans/wave4-stack.md`

**Sources consulted:**
1. `docs/NFC_STACK_CONVENTIONS.md` — BINDING; read in full
2. `docs/superpowers/plans/wave3-router.md` §1 — `service.h` vtable VERBATIM + persist-ID table (`NFC_PERSIST_ID_ALIRO = 0x05`)
3. `docs/superpowers/plans/wave4-stack.md` §1.5/§5.2 — ALIRO profile: two AIDs registered under one `nfc_service_t`; expected surface `aliro_service_init/shutdown/get`
4. `docs/nfc/archive/specs/2026-06-08-nfc-emulation-stack-design.md` (v2) §6.4.5, §6.5, §10, §12, §13
5. `aliro/interface_impl/crypto.cpp` — PSA call patterns (ECDH, HKDF-SHA-256, ECDSA-P256, AES-GCM, SHA-256, TRNG)
6. `aliro/platform/crypto/utils.h/.cpp` — key import/export patterns (`ImportPrivateKey`, `ImportPublicKey`, `ExportPublicKey`, `DestroyKey`, `PreserveKey`)
7. `aliro/storage/psa_key_ids.h` — key-ID namespace (range base, private key at `base+0x0000`, kpersistent range at `base+0x1000`)
8. `aliro/kpersistent_manager/kpersistent_manager.h` — Kpersistent abstract interface (C++; NOT reused)
9. `aliro/aliro_work/aliro_work.h/.cpp` — `AliroWorkInit/Submit` pattern (C with `extern "C"`; different queue — NOT used)
10. `aliro/aliro_work/Kconfig` — `DOOR_LOCK_ALIRO_WORKQUEUE_STACK_SIZE=5120`, `PRIORITY=10`
11. `aliro/CMakeLists.txt` + `aliro/Kconfig` — C++ build structure, DOOR_LOCK_* Kconfig gates
12. `docs/superpowers/plans/wave1-hal.md` §2 — `send_response` borrow contract; `nfc_work_q` owned by HAL
13. `docs/superpowers/plans/wave2-framing.md` §1 — `nfc_apdu_t`, `NFC_CLA_PROP=0x80`, status word constants
14. `src/stats.h` — `STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_RESET`, `STATS_COPY_OUT`
15. `~/.claude/rules/misra-coding-standards-zephyr-misra-deviations.md` — DEV-ZEP-001…018
16. `~/.claude/rules/misra-coding-standards-misra-c-2012.md` — MISRA C:2012

> **NOT consulted / explicitly excluded:** `src/nfc_emulation.c/.h`; `aliro/access_manager/` (reader-side AccessManager — incompatible); `aliro/platform/nfc/nfc_transport_rfal.cpp` (RFAL reader transport); `libaliro.a` (reader-only binary).

---

> **Architecture Framing — spec v3 (2026-06-12):** This service is the
> **NFCT/card-role first-slice** of the **Aliro** protocol module as defined by the
> [NFC Stack Architecture v3](../specs/2026-06-12-nfc-stack-architecture.md) (§4.3).
> A protocol module owns: data model (credential pubkey + config) · (de)serialize ·
> **listener** (this file, built under `CONFIG_NFC_ROLE_CARD`) · **poller**
> (reader role — RESERVED, not implemented in this slice).
> **Lane:** `NFC_LANE_ISO_DEP` — Type-4, dispatched via APDU framing (Wave 2) + AID
> router (Wave 3). **Kconfig:** `NFC_SERVICE_ALIRO` enables this protocol module; the
> listener compiles under `CONFIG_NFC_ROLE_CARD` (orthogonal, Wave 1). Existing symbol
> names are unchanged. **Serialize completeness:** output classified
> `NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED`
> for public-key + config fields; the credential private key is never in the blob
> (DECISION-6). A future poller blob would carry
> `NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL` — Aliro credentials are not key-cloneable
> (wave6 canonical C symbols — wave6-store.md §1.7 / spec v3 §4.5).

---

## 1. Types and Constants

All public types live in `aliro_service.h`. Internal session state lives in `aliro_service.c`. Every constant carries the `U` suffix.

### 1.1 Crypto Sizes

```c
/* ── Crypto primitive sizes (PSA / Aliro 0.9.4) ──────────────────────────── */
#define ALIRO_P256_PRIVATE_KEY_SIZE    32U  /* P-256 raw private key bytes             */
#define ALIRO_P256_PUBLIC_KEY_SIZE     65U  /* P-256 uncompressed public key (0x04+xy) */
#define ALIRO_P256_SIGNATURE_SIZE      64U  /* P-256 ECDSA signature (r||s)            */
#define ALIRO_NONCE_SIZE               16U  /* Reader/card NFC nonce bytes             */
#define ALIRO_AES_KEY_SIZE             32U  /* AES-256 session key bytes               */
#define ALIRO_AES_GCM_NONCE_SIZE       12U  /* AES-GCM nonce bytes                     */
#define ALIRO_AES_GCM_TAG_SIZE         16U  /* AES-GCM authentication tag bytes        */
#define ALIRO_ECDH_SHARED_SECRET_SIZE  32U  /* psa_raw_key_agreement P-256 output      */
```

### 1.2 Protocol Command Bytes

```c
/* ── Aliro APDU class / instruction bytes ────────────────────────────────── */
/* CLA=0x80 is NFC_CLA_PROP (wave2 apdu_types.h). */
#define ALIRO_INS_AUTH0    0x80U  /* AUTH0 command: reader nonce + reader eph pubkey  */
#define ALIRO_INS_AUTH1    0x81U  /* AUTH1 command: reader ECDSA signature            */
#define ALIRO_INS_EXCHANGE 0x82U  /* EXCHANGE command: AES-GCM encrypted payload      */
/* **DECISION-9**: EXCHANGE INS=0x82 is inferred from the 0x80/0x81/0x82 pattern and
 * cross-referenced with Aliro 0.9.4 implementation experience. The implementer MUST
 * verify this byte against the Aliro 0.9.4 NFC protocol specification before
 * integration testing with a real reader. */
```

### 1.3 AUTH0 / AUTH1 Expected Data Lengths

```c
/* Standard expedited phase AUTH0 command data (reader→card):
 *   reader_nonce (16 B) + reader_eph_pubkey (65 B) = 81 B
 * **DECISION-10**: The exact AUTH0 data layout (field order, any TID prefix,
 * TLV encoding) must be confirmed against Aliro 0.9.4 §X before implementation.
 * The sizes below are correct for standard expedited; fast/kpersistent path is
 * out of scope (spec §12 / §13).                                               */
#define ALIRO_AUTH0_DATA_SIZE  81U   /* reader_nonce(16) + reader_eph_pubkey(65) */
#define ALIRO_AUTH1_DATA_SIZE  64U   /* reader ECDSA signature r||s              */
/* **DECISION-10 cont.**: AUTH1 data may contain additional fields (e.g.
 * reader certificate, reader group ID). Size MUST be confirmed against spec.   */
```

### 1.4 AID Byte Arrays (source of truth: wave4-stack §1.5)

```c
/* Declared in aliro_service.c (static const at file scope, MISRA Dir 4.9) */
static const uint8_t k_aliro_expedited_aid[] = {
    0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x01U, 0x00U
};  /* 9 bytes — registered in wave4 as k_aliro_expedited_aid */

static const uint8_t k_aliro_stepup_aid[] = {
    0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x02U, 0x00U
};  /* 9 bytes — STEP-UP PHASE: stub only per spec §12 */
```

### 1.5 PSA Key Identifiers

```c
/* ── PSA persistent key IDs for the Aliro credential service ─────────────── */
/* **DECISION-5**: A dedicated PSA key ID range is defined in Kconfig to avoid
 * collision with aliro/ tree keys (DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE) and
 * any other application keys. Default 0x00002000 is in the PSA application
 * persistent key range (0x1..0x3FFFFFFF).
 *
 * If both the aliro/ tree (with DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE) and this
 * service are compiled into the same image (NOT expected — DECISION-1 excludes
 * this), the RANGES (not just bases) MUST NOT overlap — the door-lock range is
 * 64 KB wide (kKeyIdRangeSize = 0x10000 in aliro/storage/psa_key_ids.h). A
 * BUILD_ASSERT in aliro_service.c checks full range disjointness — see §7.5. */

/* Credential static private key (P-256, persistent, SIGN_MESSAGE).
 * Provisioned via shell command 'aliro provision <privkey_hex>'.
 * Key slot is WRITE-ONCE per session — shell provision replaces it.           */
#define NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE  \
    ((psa_key_id_t)(CONFIG_NFC_ALIRO_CREDENTIAL_PSA_KEY_BASE + 0U))

/* Ephemeral keys: volatile (not persistent). PSA key ID stored in session
 * state (s_session.card_eph_key_id) and destroyed at session end.             */
```

### 1.6 Serialization Limit

```c
/* Aliro service serialized blob:
 *   Byte  0:     format version (0x01)
 *   Bytes 1-2:   supported features flags (2 B)
 *   Bytes 3-4:   protocol version major.minor (2 B; e.g. 0x00 0x09 for 0.9)
 *   Bytes 5-69:  credential static public key (65 B P-256 uncompressed)
 *   Total: 70 B — headroom to 128 B for future additions.
 *
 * **DECISION-6 (private key never in blob)**: the credential private key lives
 * in a PSA persistent slot. It is NEVER written to the store blob. Only public
 * material and config flags are serialized. The Wave 6 store layer hands a blob
 * of max ALIRO_SERVICE_MAX_SERIALIZED bytes to serialize/deserialize.          */
#define ALIRO_SERVICE_MAX_SERIALIZED  128U
```

### 1.7 `aliro_service_config_t` (frozen after `init()`)

```c
typedef struct {
    /**
     * Aliro protocol version to advertise in SELECTResponse.
     * Default: {0x00, 0x09} for Aliro 0.9.4.
     * **DECISION-11**: exact encoding of version fields in SELECTResponse TLV
     * must be confirmed against Aliro 0.9.4 §SELECT_RESPONSE.
     */
    uint8_t protocol_version_major;
    uint8_t protocol_version_minor;
} aliro_service_config_t;

#define ALIRO_SERVICE_CONFIG_DEFAULT \
    ((aliro_service_config_t){ .protocol_version_major = 0x00U, \
                               .protocol_version_minor = 0x09U })
```

### 1.8 `aliro_service_stats_t`

```c
typedef struct {
    uint32_t select_expedited_count;   /**< SELECT expedited AID received           */
    uint32_t select_stepup_count;      /**< SELECT step-up AID received (stub)      */
    uint32_t auth0_count;              /**< AUTH0 commands accepted (state correct)  */
    uint32_t auth1_count;              /**< AUTH1 commands accepted (state correct)  */
    uint32_t exchange_count;           /**< EXCHANGE commands accepted               */
    uint32_t session_complete_count;   /**< EXCHANGE completed → DONE               */
    uint32_t auth_reader_fail_count;   /**< AUTH1 reader verification failed (6982)  */
    uint32_t crypto_busy_reject_count; /**< APDU rejected: crypto in-flight (6985)  */
    uint32_t state_reject_count;       /**< APDU rejected: wrong FSM state (6985)   */
    uint32_t session_reset_count;      /**< Sessions reset by field_off/deselect     */
    uint32_t crypto_error_count;       /**< PSA API returned non-SUCCESS             */
    uint32_t error_count;              /**< Mandatory — STATS_ERROR                  */
    int32_t  last_error_code;          /**< Mandatory — last errno                   */
} aliro_service_stats_t;
```

### 1.9 `aliro_service_state_t` (Pattern A, Minimal lifecycle)

```c
typedef enum {
    ALIRO_SERVICE_STATE_UNINITIALIZED = 0,  /**< Before init()  */
    ALIRO_SERVICE_STATE_INITIALIZED,        /**< After init()   */
} aliro_service_state_t;
```

### 1.10 Session FSM States

```c
/* SMF state identifiers — referenced by both FSM table and test mocks. */
typedef enum {
    ALIRO_SESSION_STATE_IDLE         = 0,
    ALIRO_SESSION_STATE_AWAIT_AUTH0  = 1,
    ALIRO_SESSION_STATE_AWAIT_AUTH1  = 2,
    ALIRO_SESSION_STATE_AWAIT_EXCHANGE = 3,
    ALIRO_SESSION_STATE_DONE         = 4,
    ALIRO_SESSION_STATE_ERROR        = 5,
    ALIRO_SESSION_STATE_COUNT_,      /**< Sentinel — array sizing only */
} aliro_session_state_id_t;
```

> **SMF array-order invariant:** Zephyr SMF dispatches by array subscript. The
> `static const struct smf_state k_states[]` table in `aliro_service.c` MUST list
> its entries in exactly this enum order; reordering the enum without the array
> (or vice versa) causes silent state misdispatch. Guard with
> `BUILD_ASSERT(ARRAY_SIZE(k_states) == ALIRO_SESSION_STATE_COUNT_)`.

---

## 2. Public API

All public functions are in `aliro_service.h`. Internal helpers are file-static in `aliro_service.c`.

```c
/* ────────────────────────────────────────────────────────────────────────────
 * src/nfc/services/aliro/aliro_service.h
 *
 * Aliro credential emulation service (wave5e).
 *
 * Minimal lifecycle (init/shutdown). Pattern A state storage.
 * Per-session SMF FSM: IDLE→AWAIT_AUTH0→AWAIT_AUTH1→AWAIT_EXCHANGE→DONE/ERROR.
 * Deferred crypto: on_apdu submits work to nfc_work_q; response sent from
 * the work handler. FWI=8 (~5s) provides headroom for ECDH+HKDF (~10ms).
 * ────────────────────────────────────────────────────────────────────────── */
#ifndef SRC_NFC_SERVICES_ALIRO_ALIRO_SERVICE_H
#define SRC_NFC_SERVICES_ALIRO_ALIRO_SERVICE_H

#include <stdint.h>
#include <stddef.h>
#include "router/service.h"

/* Exported types (§1) — full definitions; no forward typedefs for anonymous-struct typedefs */

typedef struct {
    uint8_t protocol_version_major;
    uint8_t protocol_version_minor;
} aliro_service_config_t;

#define ALIRO_SERVICE_CONFIG_DEFAULT \
    ((aliro_service_config_t){ .protocol_version_major = 0x00U, \
                               .protocol_version_minor = 0x09U })

typedef struct {
    uint32_t select_expedited_count;
    uint32_t select_stepup_count;
    uint32_t auth0_ok_count;
    uint32_t auth1_ok_count;
    uint32_t exchange_ok_count;
    uint32_t state_reject_count;
    uint32_t crypto_busy_reject_count;
    uint32_t error_count;
} aliro_service_stats_t;

typedef enum {
    ALIRO_SERVICE_STATE_UNINITIALIZED = 0,
    ALIRO_SERVICE_STATE_INITIALIZED,
} aliro_service_state_t;

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize the Aliro service.
 *
 * Resets all session state. Does NOT provision crypto keys — use the shell
 * command 'aliro provision' after init.
 *
 * @param cfg  Configuration; NULL → ALIRO_SERVICE_CONFIG_DEFAULT.
 * @retval  0        Success; state → INITIALIZED.
 * @retval -EALREADY Already INITIALIZED.
 * @retval -EINVAL   PSA crypto subsystem not ready (psa_crypto_init failed).
 * @caller_sync
 */
int aliro_service_init(const aliro_service_config_t *cfg);

/**
 * @brief Shut down the Aliro service.
 *
 * Destroys any live ephemeral PSA key, resets session state, transitions to
 * UNINITIALIZED. Does NOT destroy the persistent credential private key slot.
 *
 * @retval  0      Success.
 * @retval -ENODEV Already UNINITIALIZED (idempotent — returns 0 actually; see §3).
 * @caller_sync
 */
int aliro_service_shutdown(void);

/* ── Service instance (router registration) ─────────────────────────────── */

/**
 * @brief Return the singleton nfc_service_t for AID router registration.
 *
 * Returns a pointer to the file-static service vtable. Valid after init().
 * The same pointer is registered under BOTH Aliro AIDs (wave4-stack §1.5).
 * The service distinguishes expedited vs step-up phase from the AID bytes
 * passed to on_select().
 *
 * @retval non-NULL always (even before init() — pointer is compile-time constant).
 * @threadsafe (read-only global pointer)
 */
const nfc_service_t *aliro_service_get(void);

/* ── Observability ───────────────────────────────────────────────────────── */

/**
 * @brief Copy out current stats under spinlock.
 * @retval  0       Success.
 * @retval -EINVAL  out == NULL.
 * @threadsafe
 */
int aliro_service_get_stats(aliro_service_stats_t *out);

/**
 * @brief Return current lifecycle state.
 * @retval ALIRO_SERVICE_STATE_UNINITIALIZED / INITIALIZED.
 * @threadsafe (reads plain enum, caller-sync writes only)
 */
aliro_service_state_t aliro_service_get_state(void);

/**
 * @brief Return pointer to the frozen config (valid after init(); NULL before).
 * @threadsafe
 */
const aliro_service_config_t *aliro_service_get_config(void);

/* ── Provisioning (called from shell or nfc_stack_load) ─────────────────── */

/**
 * @brief Import a P-256 credential private key into PSA persistent storage.
 *
 * Stores the key at NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE. If a key already
 * exists at that slot, it is destroyed first and replaced.
 *
 * @param privkey_raw  32-byte raw P-256 private key (big-endian).
 * @param len          Must be ALIRO_P256_PRIVATE_KEY_SIZE (32).
 * @retval  0        Key imported successfully.
 * @retval -EINVAL   privkey_raw == NULL or len != 32.
 * @retval -EIO      PSA psa_import_key failed.
 * @retval -ENODEV   Service not INITIALIZED.
 * @caller_sync
 *
 * **DECISION-6**: Raw private key bytes are passed only through this function —
 * never stored in RAM beyond the PSA import call, never written to the store blob.
 */
int aliro_service_provision_credential(const uint8_t *privkey_raw, size_t len);

#endif /* SRC_NFC_SERVICES_ALIRO_ALIRO_SERVICE_H */
```

---

## 3. Contracts

### `aliro_service_init(cfg)`
- **Pre:** PSA crypto subsystem initialized (`psa_crypto_init()` called by application startup, or by nfc_stack).
- **Post (0):** `s_state = INITIALIZED`; `STATS_RESET(s_stats)` (first statement after `-EALREADY` guard); `s_config` populated (cfg or default); session FSM reset to IDLE.
- **Errors:**
  - `-EALREADY` — `s_state == INITIALIZED`. Stats NOT reset (CONVENTIONS §6).
  - Failures from PSA init are deferred — the service does not call `psa_crypto_init()` itself; it assumes the caller has done so. If crypto is unavailable, the first PSA call in the crypto work handler will fail and the session will ERROR.

### `aliro_service_shutdown()`
- **Pre:** any state.
- **Post (0):** any live ephemeral PSA key (`s_session.card_eph_key_id != 0`) is destroyed; session FSM reset to IDLE; `s_state = UNINITIALIZED`.
- **Idempotent:** UNINITIALIZED → return 0.

### `aliro_service_get()`
- Always returns `&s_service` (file-static, compile-time const pointer). Never NULL.

### `aliro_service_get_stats(out)`
- **Pre:** none (callable before init).
- **Errors:** `-EINVAL` if `out == NULL`.
- Uses `STATS_COPY_OUT(&s_stats_lock, s_stats, out)`.

### `aliro_service_get_state()`
- Never fails. Returns `s_state` (plain enum, caller-sync lifecycle, no lock needed — see §4).

### `aliro_service_get_config()`
- Returns `&s_config` unconditionally (never NULL), per CONVENTIONS §2.

### `aliro_service_provision_credential(privkey_raw, len)`
- **Pre:** `s_state == INITIALIZED`.
- **Post (0):** PSA key at `NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE` is populated.
- **Implementation notes:**
  1. Validate `privkey_raw != NULL && len == ALIRO_P256_PRIVATE_KEY_SIZE`.
  2. Attempt `psa_destroy_key(NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE)` (ignore `PSA_ERROR_INVALID_HANDLE` — key may not exist yet).
  3. Call `psa_import_key()` with `PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)`, `PSA_ALG_ECDSA(PSA_ALG_SHA_256)`, usage `PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_EXPORT`, `PSA_KEY_LIFETIME_PERSISTENT`, id `NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE`.
  4. After the import call, zero the local copy of the key material immediately (`memset` the input pointer is the caller's responsibility; this function does not retain a copy).
- **Errors:** `-EINVAL` (bad args), `-ENODEV` (UNINITIALIZED), `-EIO` (PSA failure).

---

## 4. Internal State

### 4.1 Module-Level State (file-static in `aliro_service.c`)

```c
/* ── Lifecycle / config (caller-sync — no concurrent write once INITIALIZED) */
static aliro_service_state_t  s_state  = ALIRO_SERVICE_STATE_UNINITIALIZED;
static aliro_service_config_t s_config;

/* ── Stats (spinlock-guarded) */
static aliro_service_stats_t  s_stats;
static struct k_spinlock       s_stats_lock;   /* NOT K_SPINLOCK_DEFINE */

/* ── Response buffer (BSS, file-static — NOT a static local; MISRA Rule 8.9)
 * Owned by the service; borrowed by nfc_t4t_response_pdu_send until the next
 * transport callback (wave1-hal §2 borrow contract).                          */
static uint8_t s_resp_buf[CONFIG_NFC_ALIRO_RESPONSE_BUF_SIZE];

/* ── Service vtable (compile-time constant; returned by aliro_service_get()) */
static const nfc_service_t s_service;   /* definition at §4.4 */

/* ── Session state (nfc_work_q only; no concurrent writer once field is ON)  */
static aliro_session_t s_session;

/* ── Deferred-crypto work item (submitted to nfc_work_q) */
static struct k_work s_crypto_work;

/* ── In-flight guard (atomic so on_apdu and a hypothetical second dispatch
 * can both read it safely on the single-threaded nfc_work_q; see §7.3)        */
static atomic_t s_crypto_inflight;  /* 0 = idle, 1 = work item queued/running */

/* ── Credential public key (65 bytes uncompressed P-256 point)
 * Precedence rule: if deserialized from store (aliro_deserialize writes this),
 * use it directly for SELECTResponse; otherwise derive on-demand via
 * psa_export_public_key(NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE, ...) per §7.8.
 * All-zero = not yet loaded (either no deserialization or no PSA key). */
static uint8_t  s_credential_pubkey[ALIRO_P256_PUBLIC_KEY_SIZE]; /* 65 bytes */
```

### 4.2 Session State Structure

```c
/* Per-NFC-session crypto context. All fields are nfc_work_q-only AFTER
 * on_select fires. Zeroed by session_reset() on field_off / deselect.         */
typedef struct {
    struct smf_ctx smf;                        /* SMF context; MUST be first    */
    aliro_session_state_id_t current_state_id; /* for test observability        */

    /* Data copied in on_apdu (net_buf lifetime = duration of on_apdu only) */
    uint8_t card_nonce[ALIRO_NONCE_SIZE];          /* generated in on_select    */
    uint8_t reader_nonce[ALIRO_NONCE_SIZE];        /* copied from AUTH0 data    */
    uint8_t reader_eph_pubkey[ALIRO_P256_PUBLIC_KEY_SIZE]; /* copied AUTH0 data */
    uint8_t auth1_sig[ALIRO_P256_SIGNATURE_SIZE];  /* copied from AUTH1 data    */

    /* PSA volatile keys (destroyed on session_reset) */
    psa_key_id_t card_eph_key_id;   /* 0 = not generated; generated in AUTH0 work */
    psa_key_id_t session_aes_key_id;/* 0 = not derived; derived in AUTH1 work     */

    /* Card ephemeral public key (exported after psa_generate_key) */
    uint8_t card_eph_pubkey[ALIRO_P256_PUBLIC_KEY_SIZE];

    /* Pending command tag (used by crypto work handler to know which response to build) */
    uint8_t pending_ins; /* ALIRO_INS_AUTH0, ALIRO_INS_AUTH1, or ALIRO_INS_EXCHANGE */

    /* EXCHANGE payload copy (if needed for AES-GCM) */
    uint8_t exchange_data[CONFIG_NFC_ALIRO_EXCHANGE_BUF_SIZE];
    size_t  exchange_data_len;
} aliro_session_t;
```

### 4.3 Full Session FSM — SMF State Machine

The SMF (Zephyr State Machine Framework) is used per CONVENTIONS §2. The FSM has **no hierarchy** (all states are top-level siblings). Entry/exit hooks are used for initialization and cleanup. The `smf_run_state()` call happens at the start of each vtable dispatch (`on_select`, `on_apdu`).

```
States and transitions
──────────────────────
IDLE ──── SELECT(expedited AID) ──────────────────────────────► AWAIT_AUTH0
IDLE ──── SELECT(step-up AID) ──── send SW 6A81 ─────────────► IDLE (unchanged)
IDLE ──── any non-SELECT APDU ──── send SW 6985 ─────────────► ERROR

AWAIT_AUTH0 ─── AUTH0 ─── copy data; submit crypto_work; NO response yet; stay AWAIT_AUTH0
               ↳ crypto_work handler: send SELECTResponse ──► AWAIT_AUTH1
AWAIT_AUTH0 ─── any other CLA/INS ─── send SW 6985 ──────────► ERROR
AWAIT_AUTH0 ─── crypto_inflight: reject with SW 6985 ─────────► ERROR

AWAIT_AUTH1 ─── AUTH1 ─── copy sig; submit crypto_work; NO response yet; stay AWAIT_AUTH1
               ↳ crypto_work handler: verify reader auth, send response
                 reader auth OK ────────────────────────────────► AWAIT_EXCHANGE
                 reader auth fails: send SW 6982 ───────────────► ERROR
AWAIT_AUTH1 ─── any other ─── send SW 6985 ──────────────────► ERROR

AWAIT_EXCHANGE ─── EXCHANGE ─── copy data; submit crypto_work; NO response yet; stay AWAIT_EXCHANGE
               ↳ crypto_work handler: send encrypted response ─► DONE
AWAIT_EXCHANGE ─── any other ─── send SW 6985 ───────────────► ERROR

DONE ─── any APDU ─── send SW 6985 ──────────────────────────► ERROR

ERROR ─── any APDU ─── send SW 6985 ─────────────────────────► ERROR (stay)

──── FIELD_OFF (any state) ─────────────────────────────────── → IDLE (reset)
──── on_deselect (any state) ───────────────────────────────── → IDLE (reset)
```

> **Authoritative source:** The Command × FSM Decision Table (§8) is the authoritative
> record of all state transitions. State transitions for AUTH0/AUTH1/EXCHANGE occur
> **inside the crypto work handler** (running on `nfc_work_q` after `k_work_submit`);
> `on_apdu` only copies data, submits the work item, and returns without sending a
> response or advancing the FSM.

**Reset trigger:** `session_reset()` is called unconditionally from both `on_field_off` and `on_deselect`. It:
1. Destroys any live `s_session.card_eph_key_id` via `psa_destroy_key()`.
2. Destroys any live `s_session.session_aes_key_id` via `psa_destroy_key()`.
3. Clears `s_crypto_inflight` to 0 (`atomic_set`).
4. `memset(&s_session, 0, sizeof(s_session))`.
5. Re-initializes the SMF to IDLE state via `smf_set_initial`.
6. Increments `session_reset_count`.

**Error-state behavior:** Once in ERROR, all APDU callbacks respond with SW `6985` (Conditions Not Satisfied). The session stays in ERROR until `session_reset()` is called (i.e., until field-off or deselect). This matches CONVENTIONS §2: "session state reset on on_deselect/on_field_off and on any protocol error (→ ERROR until field-off)".

### 4.4 Service Vtable (compile-time constant)

```c
static const nfc_service_t s_service = {
    .on_select    = aliro_on_select,
    .on_apdu      = aliro_on_apdu,
    .on_deselect  = aliro_on_deselect,
    .on_field_off = aliro_on_field_off,
    .serialize    = aliro_serialize,
    .deserialize  = aliro_deserialize,
    .persist_id   = NFC_PERSIST_ID_ALIRO,   /* 0x05 — wave3-router §1.1 table */
    .user_ctx     = NULL,                    /* service uses file-static state */
};
```

---

## 5. Integration Points

### 5.1 Downward calls (this service → lower layers)

| Function | Header | When | Notes |
|----------|--------|------|-------|
| `nfc_transport_send_response(buf, len)` | `hal/nfc_transport.h` | From `s_crypto_work` handler | `buf = s_resp_buf`; owned by service; valid until next transport callback |
| `nfc_transport_submit_work(work)` | `hal/nfc_transport.h` | From `on_apdu` (on `nfc_work_q`) | **DECISION-3**: already satisfied by wave1 (prototype 8 of 12, task 14); `@threadsafe`; NULL-check inside. |
| `psa_generate_random(buf, len)` | `<psa/crypto.h>` | `on_select` | Generate `card_nonce` |
| PSA crypto APIs (see §6) | `<psa/crypto.h>` | `s_crypto_work` handler | ECDH, HKDF, ECDSA sign, AES-GCM |

### 5.2 Upward calls (lower layers → this service via vtable)

| Caller | Vtable slot | Thread | Notes |
|--------|-------------|--------|-------|
| `aid_router_dispatch` | `on_select` | `nfc_work_q` | Called when SELECT AID matches either `k_aliro_expedited_aid` or `k_aliro_stepup_aid` |
| `aid_router_dispatch` | `on_apdu` | `nfc_work_q` | All non-SELECT APDUs while this service is active |
| `nfc_stack.c` / framing field-off chain | `on_deselect` | `nfc_work_q` | Another service selected |
| `nfc_stack.c` / framing field-off chain | `on_field_off` | `nfc_work_q` | RF field removed |
| `nfc_store` (Wave 6) | `serialize` | `@caller_sync` | Only while stack STOPPED |
| `nfc_store` (Wave 6) | `deserialize` | `@caller_sync` | Only while stack STOPPED |

### 5.3 Registration in `nfc_stack.c` (Wave 4 §5.2, case `NFC_PROFILE_ALIRO`)

Wave 4 already allocates this:
```c
case NFC_PROFILE_ALIRO:
    rc = aid_router_register(k_aliro_expedited_aid,
                             sizeof(k_aliro_expedited_aid),
                             aliro_service_get());
    if (rc == 0) {
        rc = aid_router_register(k_aliro_stepup_aid,
                                 sizeof(k_aliro_stepup_aid),
                                 aliro_service_get());
    }
    break;
```

Both calls pass the SAME `nfc_service_t *` — the router allows multiple AIDs per service instance.

### 5.4 RESERVED — `aliro_poller` (Reader Role)

> **Not implemented in this slice.** When a reader-capable backend becomes available
> (`CONFIG_NFC_ROLE_READER` + ST25R3916/RFAL or PN7160), the Aliro protocol module
> gains an `aliro_poller` component that attempts to read public credential fields from
> a physical Aliro-capable device into the same data model that this listener serves.
> **Key-cloneability caveat:** a poller can capture the credential public key and
> configuration metadata (reader public key, flags, AID version) but **never the
> credential private key** — Aliro credentials are not key-cloneable by design.
> The serialized blob would carry a `NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL` completeness flag (spec v3 §4.5 / wave6-store.md §1.7).
> Note: Aliro uses mutual-auth at the protocol level; a reader-role poller would only
> observe the public half of the exchange.
> **Header/interface placeholder only** — no `aliro_poller` API is declared or
> compiled in this slice. When `CONFIG_NFC_ROLE_READER` is added, the poller source
> compiles under that gate, orthogonal to `CONFIG_NFC_ROLE_CARD`.

---

## 6. Deferred-Crypto Work Pattern (Binding)

### 6.1 Overview

CONVENTIONS §4 permits both synchronous and deferred responses. For AUTH0/AUTH1/EXCHANGE, ECDH+HKDF (P-256) on nRF54L15 via **CRACEN** (`CONFIG_PSA_CRYPTO_DRIVER_CRACEN`) takes on the order of ~10 ms — too long for the synchronous callback path (exact timing to-be-measured on target; was ~10 ms on nRF52840/CC310 and CRACEN is in the same class). The deferred pattern is **binding** for these three commands:

```
on_apdu(AUTH0/AUTH1/EXCHANGE)  [running on nfc_work_q]
  │
  ├─ 1. Check s_crypto_inflight (atomic_get). If 1: send SW 6985, STATS_INC crypto_busy_reject_count; return.
  ├─ 2. Validate FSM state (smf_run_state decision). If wrong state: send SW 6985, transition to ERROR; return.
  ├─ 3. Copy APDU data fields into s_session static buffers (CRITICAL: net_buf lifetime ends when on_apdu returns).
  ├─ 4. Set s_session.pending_ins = apdu->ins.
  ├─ 5. atomic_set(&s_crypto_inflight, 1).
  ├─ 6. nfc_transport_submit_work(&s_crypto_work).   ← queued to nfc_work_q
  └─ 7. return.   ← NO nfc_transport_send_response here

s_crypto_work_handler()  [next item on nfc_work_q; runs after on_apdu returns]
  │
  ├─ dispatch on s_session.pending_ins:
  │     AUTH0   → auth0_crypto_handler()
  │     AUTH1   → auth1_crypto_handler()
  │     EXCHANGE → exchange_crypto_handler()
  ├─ Build response in s_resp_buf.
  ├─ nfc_transport_send_response(s_resp_buf, resp_len).   ← ACTUAL response
  └─ atomic_set(&s_crypto_inflight, 0).
```

### 6.2 Self-Submit Analysis

`on_apdu` runs as part of the `s_apdu_work` work item on `nfc_work_q`. Submitting `s_crypto_work` to the same queue is the **self-submit pattern** (task description: "approved"). Since Zephyr work queue items execute sequentially on a single thread:

1. `s_apdu_work` runs to completion (calls `on_apdu`, which returns).
2. `s_crypto_work` runs next — guarantees AUTH0/AUTH1/EXCHANGE crypto completes BEFORE any subsequent APDU dispatch from the same fifo drain.
3. The reader physically cannot send AUTH1 until it receives AUTH0Response. Because AUTH0Response is sent from `s_crypto_work`, AUTH1 can only arrive AFTER `s_crypto_work` completes. Therefore `s_crypto_inflight` will be 0 when AUTH1's `on_apdu` runs.

The `s_crypto_inflight` guard is therefore a **defensive assertion**, not a functional requirement in normal flow. It protects against adversarial readers that violate the Aliro protocol sequence.

### 6.3 Latency Budget vs FWI=8

| Operation | Worst-case latency (nRF54L15/CRACEN; to-be-measured on target — prior nRF52840/CC310 numbers shown for reference) |
|-----------|-------------------------------|
| psa_generate_key (P-256 ephemeral) | ~5 ms |
| psa_raw_key_agreement (ECDH P-256) | ~7 ms |
| psa_key_derivation (HKDF-SHA-256) | ~2 ms |
| psa_sign_message (ECDSA P-256) | ~7 ms |
| psa_aead_encrypt (AES-256-GCM, ~32 B) | ~1 ms |
| **AUTH0 total** (keygen + export) | **~6 ms** |
| **AUTH1 total** (ECDH + HKDF + sign) | **~17 ms** |
| **EXCHANGE total** (AES-GCM decrypt+encrypt) | **~3 ms** |

FWI=8 grants approximately **4800 ms** per APDU exchange (ISO-DEP formula: 2^FWI × 256 / 13.56 MHz × NFC carrier). All operations fit within this budget with > 200× margin.

**nfc_work_q stack:** The default `NFC_HAL_WORKQ_STACK_SIZE=2048` (wave1-hal §1.6) covers PSA ECDH+HKDF+sign stack frames. Wave 1 documents that the stack is "sized for Aliro crypto on-WQ". Thread Analyzer peak measurement required before production (creed §11).

---

## 7. Implementation Notes

### 7.1 DECISION-1: Language Boundary — C Service, PSA Calls Reimplemented in C

**Problem:** `aliro/interface_impl/crypto.cpp` and `aliro/platform/crypto/utils.cpp` are C++ files in the `Aliro::Interface::Crypto` and `DoorLock::Crypto` namespaces. They depend on C++ types (`std::array`, template functions), C++ Kconfig gates (`CONFIG_DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE`, `CONFIG_NCS_ALIRO_BLE_UWB`), and the `NCS_ALIRO` module structure which builds as part of the Aliro C++ application tree (`aliro/CMakeLists.txt` adds to `app` target only).

**Decision:** The service (`aliro_service.c`) is **pure C** and **reimplements the needed PSA calls directly**. It does NOT link against the aliro/ C++ objects. Rationale:
1. PSA Crypto API (`psa/crypto.h`) is pure C — no C++ required.
2. The aliro/ Kconfig gates (`DOOR_LOCK_*`, `CHIP`, `CONFIG_NCS_ALIRO_*`) are for a full BLE+UWB+Matter door-lock application. Pulling them in would make the NFC service depend on a large application-level Kconfig tree.
3. The C++ objects (`crypto.cpp`) use `std::array` and lambdas; wrapping them with `extern "C"` is brittle and would expose implementation internals.
4. The 6 PSA calls needed (generate_key, export_public_key, raw_key_agreement, key_derivation, sign_message, aead_encrypt/decrypt) are simple enough to write inline in C.

**Result:** `aliro_service.c` uses PSA headers directly. The `aliro/` directory is a **reference** (PSA call pattern, key attribute construction), not a linked dependency.

### 7.2 DECISION-2: `nfc_work_q` vs `AliroWorkQ`

`aliro_work.h` exposes `AliroWorkInit/AliroWorkSubmit` for the aliro/ application's own work queue. The NFC service uses **`nfc_work_q` (the HAL-owned queue)** instead, for three reasons:
1. CONVENTIONS §7: "nfc_work_q is a named work queue owned by the HAL — never the system WQ." All NFC service dispatch already runs on this queue; crypto work must also run here to maintain ordering guarantees.
2. A separate Aliro work queue would add another OS thread and stack allocation, while `nfc_work_q` provides the ~5s FWI budget.
3. The `aliro/aliro_work/` files are C++ objects (`extern "C"` interface) built into the aliro/ CMake target, not the `src/nfc/` tree.

### 7.3 DECISION-3: HAL Work Queue Accessor

**Already satisfied by wave1.** `nfc_transport_submit_work` is prototype 8 of 12 in
the wave1-hal public API (wave1 Task 14). It is annotated `@threadsafe` and includes
a NULL-check guard on the work pointer before calling `k_work_submit_to_queue`. No
wave1 update is required. Task 12 below confirms the prototype is in place.

```c
/**
 * @brief Submit a work item to nfc_work_q.
 *
 * @param work  Initialized k_work item.
 * @retval  0       Work item submitted or already pending.
 * @retval -ENODEV  Transport not STARTED (nfc_work_q not running).
 * @threadsafe  (k_work_submit_to_queue is ISR-safe)
 */
int nfc_transport_submit_work(struct k_work *work);  /* @threadsafe; NULL-check inside */
```

### 7.4 DECISION-4: In-Flight Guard SW

When `on_apdu` receives an AUTH0/AUTH1/EXCHANGE while `s_crypto_inflight == 1`, the service responds with **SW `6985`** (Conditions of Use Not Satisfied — `NFC_SW_CONDITIONS_NOT_SATISFIED` from `apdu_types.h`). The FSM transitions to ERROR. Rationale: SW 6985 indicates the command cannot be executed in the current state; it is more informative than SW 6F00 (unknown error) and does not disclose internal crypto state.

### 7.5 DECISION-5: PSA Key ID Allocation

```kconfig
config NFC_ALIRO_CREDENTIAL_PSA_KEY_BASE
    hex "Base PSA persistent key ID for the Aliro credential service"
    default 0x00002000
    depends on NFC_SERVICE_ALIRO
    help
      Allocates PSA persistent key slots starting at this ID for the NFC
      Aliro credential. Currently one slot is used (credential private key
      at base+0). Must not overlap with CONFIG_DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE
      if the aliro/ door-lock application tree is also in the build.
```

A `BUILD_ASSERT` in `aliro_service.c` guards against **range overlap** (not just base
equality — the door-lock range is 64 KB wide, `kKeyIdRangeSize = 0x10000` in
`aliro/storage/psa_key_ids.h`; a base of e.g. `0x40001` differs from `0x40000` yet
lands inside the door-lock range) when both symbols are defined:
```c
#ifdef CONFIG_DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE
BUILD_ASSERT(
    (CONFIG_NFC_ALIRO_CREDENTIAL_PSA_KEY_BASE <
     CONFIG_DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE) ||
    (CONFIG_NFC_ALIRO_CREDENTIAL_PSA_KEY_BASE >=
     CONFIG_DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE + 0x10000U),
    "NFC_ALIRO PSA key base falls inside the DOOR_LOCK_ALIRO 64KB key range");
#endif
```
(The NFC service currently uses a single slot at base+0, so checking the base
against the door-lock range is sufficient; widen the check if more slots are added.)

### 7.6 DECISION-6: Provisioning Boundary — Private Key in PSA Only

The `serialize()` output contains:
- Format version (1 byte)
- Feature flags (2 bytes)
- Protocol version (2 bytes)
- Credential static **public** key (65 bytes)

The credential **private** key is NEVER in the serialize blob. It lives exclusively in a PSA persistent slot (`NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE`). The `deserialize()` function restores public key / config data only. The private key must be re-provisioned after a factory reset via `aliro_service_provision_credential()` or the shell command.

> **Integration prerequisite (writable_ndef_msg — nRF54L15):**
> This repo has `CONFIG_PSA_CRYPTO_DRIVER_CRACEN=y` (CRACEN engine) and KMU when built for nRF54L15. Before Aliro can store the credential private key persistently across resets, either:
> (a) `CONFIG_MBEDTLS_PSA_CRYPTO_STORAGE_C=y` must be enabled in this repo's `prj.conf` (plus any required `CONFIG_SETTINGS=y`/`CONFIG_NVS=y` backend), or
> (b) keys must be imported volatile (non-persistent) and re-provisioned on every boot.
> **Flag as an integration prerequisite before Aliro wave implementation.**
>
> **PSA key-ID range registration:** No collision exists today (this repo allocates no application PSA keys). When Aliro lands, the reserved range (`NFC_ALIRO_CREDENTIAL_PSA_KEY_BASE` default `0x2000`) **must be registered** in this repo's Kconfig / key-allocation comment to prevent future collisions with other modules.

**PSA key attributes for the credential private key:**
```c
psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_type(&attrs, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
psa_set_key_bits(&attrs, 256U);
psa_set_key_algorithm(&attrs, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_SIGN_MESSAGE);
psa_set_key_lifetime(&attrs, PSA_KEY_LIFETIME_PERSISTENT);
psa_set_key_id(&attrs, NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE);
```

**PSA key attributes for the ephemeral P-256 key (AUTH0, volatile):**
```c
psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_type(&attrs, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
psa_set_key_bits(&attrs, 256U);
psa_set_key_algorithm(&attrs, PSA_ALG_ECDH);
psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_DERIVE);
/* psa_set_key_lifetime: volatile (default — no psa_set_key_id call) */
```

**PSA key attributes for the session AES key (AUTH1, volatile):**
```c
psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_type(&attrs, PSA_KEY_TYPE_AES);
psa_set_key_bits(&attrs, 256U);
psa_set_key_algorithm(&attrs, PSA_ALG_GCM);
psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
/* volatile — derived via HKDF from shared secret */
```

### 7.7 DECISION-7: Step-Up AID Handling

Spec §12: "Aliro step-up phase AccessDocument — skeleton stub only in first implementation."

When `on_select` is called with `k_aliro_stepup_aid`:
- Send SW `6A81` (Function Not Supported — `NFC_SW_FUNC_NOT_SUPPORTED` from `apdu_types.h`).
- Do NOT change session state (remain in IDLE).
- Increment `select_stepup_count`.

The stub response is 2 bytes: `{ NFC_SW1(NFC_SW_FUNC_NOT_SUPPORTED), NFC_SW2(NFC_SW_FUNC_NOT_SUPPORTED) }` placed in `s_resp_buf`.

### 7.8 DECISION-8: SELECTResponse Encoding

**DECISION-11 (wire format):** The exact TLV/CBOR structure of the Aliro SELECTResponse FCI (File Control Information) MUST be verified against the Aliro 0.9.4 NFC specification §SELECT_RESPONSE before integration testing. The implementation plan documents the expected content:

- A proprietary FCI TLV (tag `6F`, per ISO 7816-4 §11.2) or a direct TLV depending on Aliro spec.
- Protocol version descriptor: at minimum one version `{major, minor}`.
- Card endpoint public key (static credential pubkey, 65 bytes uncompressed) — used by the reader to identify the card.
- Trailing SW 9000.

Placeholder implementation for test scaffolding:
```c
/* Minimal SELECTResponse: version(2) + static pubkey(65) + SW(2) = 69 bytes.
 * DECISION-11: TLV tags not yet confirmed. Replace with spec-correct encoding. */
static size_t build_select_response(uint8_t *buf, size_t max)
{
    if (max < (2U + ALIRO_P256_PUBLIC_KEY_SIZE + 2U)) { return 0U; }
    buf[0] = s_config.protocol_version_major;
    buf[1] = s_config.protocol_version_minor;
    /* copy credential static pubkey if available */
    /* ... psa_export_public_key(NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE, ...) */
    buf[2U + ALIRO_P256_PUBLIC_KEY_SIZE + 0U] = NFC_SW1(NFC_SW_OK);
    buf[2U + ALIRO_P256_PUBLIC_KEY_SIZE + 1U] = NFC_SW2(NFC_SW_OK);
    return 2U + ALIRO_P256_PUBLIC_KEY_SIZE + 2U;
}
```

### 7.9 DECISION-9: EXCHANGE INS

Assumed `0x82U` based on the contiguous 0x80/0x81/0x82 Aliro pattern. Must be confirmed against the Aliro 0.9.4 spec before integration testing.

### 7.10 AUTH1 Transcript Construction

The transcript signed by both the reader (in AUTH1 command) and the card (in AUTH1 response) is:
```
transcript = reader_nonce(16) || card_nonce(16) || reader_eph_pubkey(65) || card_eph_pubkey(65)
           = 162 bytes
```
**DECISION-12:** Exact transcript construction (field ordering, any additional fields such as AID or session context) MUST be confirmed against Aliro 0.9.4 §AUTH1 before implementation. The 162-byte form above is standard for PAKE-style NFC protocols but Aliro may include additional context.

### 7.10a Build-Time Gate for Unverified Protocol Constants

DECISIONs 9–12 are inferred and must be checked against the Aliro 0.9.4 spec. To
prevent a build with unverified constants silently reaching hardware testing, the
service Kconfig adds:

```kconfig
config NFC_ALIRO_PROTOCOL_VERIFIED
    bool "Aliro protocol constants verified against the 0.9.4 spec"
    default n
    help
      Set to y only after DECISION-9..12 (EXCHANGE INS, AUTH0 layout,
      SELECTResponse TLV, AUTH1 transcript) have been verified against the
      Aliro 0.9.4 NFC protocol specification.
```

and `aliro_service.c` emits a compile-time warning while it is unset:

```c
#if !IS_ENABLED(CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED)
#warning "Aliro protocol constants (DECISION-9..12) are UNVERIFIED against the 0.9.4 spec"
#endif
```

### 7.11 MISRA C:2012 and DEV-ZEP Deviations

| Rule / Deviation | Location | Justification |
|-----------------|----------|---------------|
| **MISRA Rule 8.9** (no static local variables) | `s_resp_buf`, `s_session`, `s_crypto_work`, `s_stats` | File-static — all at file scope, NOT inside functions. Compliant. |
| **DEV-ZEP-001** (`CONTAINER_OF`) | Not used; service does not recover `k_work *` → parent struct (SMF context is first field; `smf_run_state` receives `&s_session.smf` directly). | N/A |
| **DEV-ZEP-008** (`LOG_*` variadic macros) | `LOG_MODULE_REGISTER(aliro_service, ...)` + `LOG_ERR/WRN/INF/DBG` throughout | Pre-approved; MISRA Rule 20.10 deviation. |
| **DEV-ZEP-009** (`shell_print` variadic) | `aliro_service_shell_cmds.c` | Pre-approved. |
| **DEV-ZEP-013** (`K_NO_WAIT`, `K_MSEC` compound literals) | `psa_generate_random` / `k_work_submit_to_queue` not using timeout macros directly; if K_NO_WAIT appears in future work scheduling, cite this. | Document if used. |
| **DEV-ZEP-016** (`ARG_UNUSED`) | Shell command handlers (`user_ctx` parameter of vtable callbacks is NULL — `(void)user_ctx` instead of `ARG_UNUSED` avoids the macro). | Use `(void)user_ctx` explicitly. |
| **SMF macros** (`SMF_CREATE_STATE`) | Session FSM state definitions | Zephyr SMF API; pre-approved per analogous `services/desfire` precedent (CONVENTIONS §2 table). DEV-ZEP-007 (function-pointer to `void *` coercions in SMF internals — suppress at the state definition site). |
| **MISRA Rule 14.4** (Boolean controlling expression) | `if (status == PSA_SUCCESS)` — `PSA_SUCCESS` is 0 (int); use `if (status == (psa_status_t)PSA_SUCCESS)` or define a helper to satisfy strict bool checking. | PSA standard C integer returns — annotate suppression if tool flags. |
| **MISRA Rule 17.7** (return value checked) | Every PSA API return value is checked; `nfc_transport_send_response` and `nfc_transport_submit_work` return values are checked; failures go to `STATS_ERROR`. | Fully compliant. |
| **MISRA Rule 15.5** (function with single point of exit) | `auth0_crypto_handler` / `auth1_crypto_handler` use a `goto cleanup` exit path for proper PSA resource destruction. This deviation (Rule 15.1) is pre-approved for resource-cleanup patterns in embedded C — cite as project-level deviation. | Alternative: structured nested `if/else` acceptable; `goto cleanup` preferred for clarity. |

---

## 8. Command × FSM Decision Table

The **complete** dispatch table. Every non-empty cell in the 10×6 matrix is fully specified. SW values reference `apdu_types.h` constants.

| APDU (CLA INS) | IDLE | AWAIT_AUTH0 | AWAIT_AUTH1 | AWAIT_EXCHANGE | DONE | ERROR |
|---|---|---|---|---|---|---|
| `SELECT` expedited AID (vtable `on_select`) | ✅ Send SELECTResponse; generate card_nonce; `smf→AWAIT_AUTH0`; STATS_INC select_expedited_count | reset then same as IDLE (on_deselect called first) | reset then same as IDLE | reset then same as IDLE | reset then same as IDLE | reset then same as IDLE |
| `SELECT` step-up AID (vtable `on_select`) | Send SW `6A81`; stay IDLE; STATS_INC select_stepup_count | reset→IDLE, send SW `6A81` | reset→IDLE, send SW `6A81` | reset→IDLE, send SW `6A81` | reset→IDLE, send SW `6A81` | reset→IDLE, send SW `6A81` |
| `AUTH0` (CLA=`0x80` INS=`0x80`) | SW `6985`; →ERROR; STATS_INC state_reject_count | ✅ Copy reader data; submit crypto_work; `s_crypto_inflight=1`; NO response yet | SW `6985`; →ERROR; STATS_INC state_reject_count | SW `6985`; →ERROR | SW `6985`; →ERROR | SW `6985`; →ERROR (stay) |
| `AUTH1` (CLA=`0x80` INS=`0x81`) | SW `6985`; →ERROR | SW `6985`; →ERROR | ✅ Copy reader sig; submit crypto_work; `s_crypto_inflight=1`; NO response yet | SW `6985`; →ERROR | SW `6985`; →ERROR | SW `6985`; →ERROR (stay) |
| `EXCHANGE` (CLA=`0x80` INS=`0x82`) | SW `6985`; →ERROR | SW `6985`; →ERROR | SW `6985`; →ERROR | ✅ Copy payload; submit crypto_work; `s_crypto_inflight=1`; NO response yet | SW `6985`; →ERROR | SW `6985`; →ERROR (stay) |
| Any unsupported INS (CLA=`0x80`) | SW `6D00`; →ERROR | SW `6D00`; →ERROR | SW `6D00`; →ERROR | SW `6D00`; →ERROR | SW `6D00`; →ERROR | SW `6985`; →ERROR (stay) |
| Any CLA ≠ `0x80` and ≠ SELECT | SW `6E00`; →ERROR | SW `6E00`; →ERROR | SW `6E00`; →ERROR | SW `6E00`; →ERROR | SW `6E00`; →ERROR | SW `6985`; →ERROR (stay) |
| In-flight guard triggered (`s_crypto_inflight==1`) | — | SW `6985`; →ERROR; STATS_INC crypto_busy_reject_count | SW `6985`; →ERROR; STATS_INC crypto_busy_reject_count | SW `6985`; →ERROR; STATS_INC crypto_busy_reject_count | — | — |
| **AUTH0 crypto_work result → success** | — | Send AUTH0Response (card_eph_pubkey + card_nonce + SW 9000); `smf→AWAIT_AUTH1`; STATS_INC auth0_count; `s_crypto_inflight=0` | — | — | — | — |
| **AUTH1 crypto_work result → reader auth FAIL** | — | — | Send SW `6982`; `smf→ERROR`; STATS_INC auth_reader_fail_count; `s_crypto_inflight=0` | — | — | — |
| **AUTH1 crypto_work result → success** | — | — | Send AUTH1Response (card_sig + SW 9000); `smf→AWAIT_EXCHANGE`; STATS_INC auth1_count; `s_crypto_inflight=0` | — | — | — |
| **EXCHANGE crypto_work result → success** | — | — | — | Send ExchangeResponse (AES-GCM output + SW 9000); `smf→DONE`; STATS_INC exchange_count, session_complete_count; `s_crypto_inflight=0` | — | — |
| **Any crypto_work PSA error** | — | Send SW `6F00`; `smf→ERROR`; STATS_INC crypto_error_count; STATS_ERROR code; `s_crypto_inflight=0` | ← same | ← same | — | — |
| **FIELD_OFF** (`on_field_off`) | `session_reset()`; stay IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE |
| **DESELECT** (`on_deselect`) | `session_reset()`; stay IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE | `session_reset()`→IDLE |

**SW constants used** (all from `framing/apdu_types.h`):

| Constant | Value | Meaning |
|----------|-------|---------|
| `NFC_SW_OK` | `0x9000` | Normal processing |
| `NFC_SW_FUNC_NOT_SUPPORTED` | `0x6A81` | Step-up not implemented |
| `NFC_SW_SECURITY_STATUS` | `0x6982` | Reader auth failed |
| `NFC_SW_CONDITIONS_NOT_SATISFIED` | `0x6985` | Wrong state / in-flight reject |
| `NFC_SW_INS_NOT_SUPPORTED` | `0x6D00` | Unknown INS (CLA=80) |
| `NFC_SW_CLA_NOT_SUPPORTED` | `0x6E00` | Unknown CLA |
| `NFC_SW_UNKNOWN` | `0x6F00` | Unexpected PSA error |

---

## 9. Conventions Compliance Checklist

- [x] **Lifecycle matches §2** — Minimal (`init`/`shutdown` only); `shutdown` not `deinit`; states `UNINITIALIZED`/`INITIALIZED`.
- [x] **`config_t`/`stats_t`/`state_t` + three getters** — `aliro_service_config_t`/`stats_t`/`state_t` defined §1.7–1.9; `get_config` (always &s_config, never NULL per CONVENTIONS §2), `get_stats` (copy-out via `STATS_COPY_OUT`), `get_state` (never fails).
- [x] **State storage Pattern A per §2** — plain enum `s_state`; lifecycle is `@caller_sync`; no ISR reads `s_state`.
- [x] **Coupling matches §3 / §4** — callbacks invoke via `nfc_service_t` vtable (`_fn`/`_cb` naming, `user_ctx`, NULL pointer is `NULL` compile-time const, all callbacks non-NULL); `nfc_transport_send_response` is downward direct call; no cross-layer header includes for wiring.
- [x] **Buffer model per §5** — static `s_resp_buf` (file-scope BSS), valid until next transport callback; no `net_buf` usage in response path. APDU data copied from `nfc_apdu_t.data` into session static buffers before `on_apdu` returns (net_buf lifetime constraint).
- [x] **Stats recipe per §6** — `static aliro_service_stats_t s_stats; static struct k_spinlock s_stats_lock;` (NOT `K_SPINLOCK_DEFINE`); `STATS_RESET` as first statement in `init()` body after `-EALREADY` guard; copy-out getter via `STATS_COPY_OUT`; every silent-drop path has a named counter.
- [x] **Threading + annotations per §7** — `on_select/on_apdu/on_deselect/on_field_off` annotated `@nfc_work_q`; `serialize/deserialize` annotated `@caller_sync`; lifecycle functions `@caller_sync`; `get_stats` `@threadsafe`.
- [x] **Error codes per §9** — errno-only in C return values; ISO 7816 SW in response bytes; no conflation.
- [x] **Shell per §10** — `aliro_service_shell_cmds.c` registers `aliro config`, `aliro stats`, `aliro state`, `aliro provision <hex>` subcommands; never inside `aliro_service.c`.
- [x] **MISRA notes + DEV-ZEP citations per §11** — §7.11 above; SMF deviation cited; PSA returns checked per Rule 17.7; `goto cleanup` pattern for PSA resource destruction (Rule 15.1 deviation, project-level).

---

## 10. Tasks

### Scaffolding

- [ ] **Task 1 — Directory structure and stub files.**
  Create:
  ```
  src/nfc/services/aliro/
    aliro_service.h      (types + API per §2)
    aliro_service.c      (file-static stubs — empty function bodies returning -ENOTSUP)
    aliro_service_shell_cmds.c  (SHELL_CMD_REGISTER skeleton)
    CMakeLists.txt
    Kconfig
  tests/unit/nfc_aliro/
    CMakeLists.txt
    testcase.yaml
    src/main.c
  ```

  **`CMakeLists.txt`** (service):
  ```cmake
  # src/nfc/services/aliro/CMakeLists.txt
  zephyr_library()
  zephyr_library_sources(
      aliro_service.c
      aliro_service_shell_cmds.c
  )
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/src/nfc)
  zephyr_library_link_libraries(psa_interface)
  ```

  **`Kconfig`** (service — tunables only; `NFC_SERVICE_ALIRO` lives in `src/nfc/Kconfig`):
  ```kconfig
  # src/nfc/services/aliro/Kconfig
  if NFC_SERVICE_ALIRO

  config NFC_ALIRO_CREDENTIAL_PSA_KEY_BASE
      hex "Base PSA persistent key ID for the NFC Aliro credential service"
      default 0x00002000
      range 0x00001000 0x3FFFFFFF
      help
        PSA persistent key ID for the credential P-256 private key
        (base+0). Must not overlap with DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE
        if the aliro/ application tree is also in the build.

  config NFC_ALIRO_RESPONSE_BUF_SIZE
      int "Static response buffer size (bytes)"
      default 256
      range 72 512
      help
        Size of the file-static response buffer s_resp_buf.
        Must accommodate the largest Aliro response:
        AUTH0Response = card_eph_pubkey(65) + card_nonce(16) + SW(2) = 83 B.
        SELECTResponse = version(2) + pubkey(65) + SW(2) = 69 B.
        AUTH1Response = sig(64) + SW(2) = 66 B.
        Default 256 provides headroom for DECISION-11 TLV additions.

  config NFC_ALIRO_EXCHANGE_BUF_SIZE
      int "EXCHANGE command payload buffer size (bytes)"
      default 128
      range 16 512
      help
        Size of the static buffer for the EXCHANGE command's encrypted payload.
        Copied from on_apdu into s_session.exchange_data.

  endif # NFC_SERVICE_ALIRO
  ```

  **Commit:** "nfc/aliro: add service directory skeleton"

- [ ] **Task 2 — Types, constants, config/stats/state structs.**
  Populate `aliro_service.h` exactly per §1 and §2 (all `typedef`/`#define`/struct/enum/function declarations). Write `aliro_service.c` stub bodies that:
  - `init()`: return `-EALREADY` if already initialized; else return 0.
  - `shutdown()`: return 0.
  - `get()`: return `&s_service` (vtable all-NULL placeholders for now).
  - `get_stats()`: call `STATS_COPY_OUT`.
  - `get_state()`: return `s_state`.
  - `get_config()`: return `&s_config` if INITIALIZED else NULL.

  Verify no linter errors. **Commit.**

### TDD — FSM Decision Table

- [ ] **Task 3 — (TDD) FSM decision table — pure logic.**
  Implement a test-only shim that replaces `nfc_transport_send_response` and records `(buf, len)` calls, and replaces `nfc_transport_submit_work` and records submitted work items.

  Write `tests/unit/nfc_aliro/src/main.c` with the following ztest cases. Each test calls the service vtable directly (bypasses the router):
  > **Platform note:** tests run as `qemu_cortex_m33` or hardware Ztest on DK (built `--no-sysbuild`). `native_sim` is Linux-CI-only in this repo.

  **FSM state tests:**
  - `test_init_state_idle`: after `aliro_service_init(NULL)`, call `aliro_service_get()`, invoke `on_field_off` → verify no crash, session in IDLE.
  - `test_select_expedited_transitions_to_await_auth0`: call `on_select(k_aliro_expedited_aid, 9, NULL)` → verify `send_response` called, response ends with `0x90 0x00`, session state = `AWAIT_AUTH0`.
  - `test_select_stepup_stays_idle`: call `on_select(k_aliro_stepup_aid, 9, NULL)` → response = `0x6A 0x81`, session state = `IDLE`.
  - `test_auth0_in_idle_rejects_6985`: call `on_select(expedited)` then reset to IDLE (call `on_field_off`); then send AUTH0 APDU → response = `0x69 0x85`, session → ERROR.
  - `test_auth0_in_await_auth0_submits_work`: call `on_select(expedited)`, then `on_apdu(AUTH0_apdu)` → verify `submit_work` called with `&s_crypto_work`, `send_response` NOT called, `s_crypto_inflight == 1`.
  - `test_auth1_in_await_auth0_rejects`: in AWAIT_AUTH0, call `on_apdu(AUTH1)` → SW `6985`, →ERROR.
  - `test_exchange_in_await_auth1_rejects`: in AWAIT_AUTH1, call `on_apdu(EXCHANGE)` → SW `6985`, →ERROR.
  - `test_inflight_guard_rejects`: set `s_crypto_inflight = 1` via test helper; call `on_apdu(AUTH0)` in AWAIT_AUTH0 → SW `6985`, `crypto_busy_reject_count++`.
  - `test_field_off_resets_any_state`: from each FSM state (AWAIT_AUTH0, AWAIT_AUTH1, ERROR), call `on_field_off` → session state = IDLE, session_reset_count incremented.
  - `test_deselect_resets_session`: from AWAIT_AUTH1, call `on_deselect` → session state = IDLE.
  - `test_unknown_ins_cla80_rejects_6d00`: in AWAIT_AUTH0, send CLA=0x80 INS=0xFF → SW `6D00`, →ERROR.
  - `test_unknown_cla_rejects_6e00`: in AWAIT_AUTH0, send CLA=0x00 INS=0xA4 (not SELECT) → SW `6E00`, →ERROR.
  - `test_error_state_all_apdus_reject_6985`: transition to ERROR; send AUTH0, AUTH1, EXCHANGE, unknown INS → all return SW `6985`; state stays ERROR.
  - `test_done_state_rejects_6985`: transition to DONE (via test helper); send any APDU → SW `6985`.

  All tests pass. **Commit:** "nfc/aliro: add FSM decision table tests (TDD)"

### Lifecycle + Stats

- [ ] **Task 4 — Module init/shutdown + stats recipe.**
  Implement `aliro_service_init()` and `aliro_service_shutdown()` exactly:
  - `init()`: check `-EALREADY`; then `STATS_RESET(s_stats)` (first statement after guard); copy config; call `smf_set_initial(&s_session.smf, &k_aliro_states[ALIRO_SESSION_STATE_IDLE])`; `s_state = INITIALIZED`; return 0.
  - `shutdown()`: check UNINITIALIZED (return 0 idempotent); call `session_reset()` (destroys any live PSA keys); `s_state = UNINITIALIZED`; return 0.

  Verify:
  - `test_init_shutdown_cycle`: init → INITIALIZED; get_state OK; shutdown → UNINITIALIZED; shutdown again → 0 (idempotent).
  - `test_stats_reset_on_init`: inject non-zero stats; init → stats zeroed.
  - `test_stats_not_reset_on_double_init`: init once; inject stats; init again → -EALREADY; stats unchanged.

  **Commit:** "nfc/aliro: implement lifecycle init/shutdown"

### on_select

- [ ] **Task 5 — `on_select` implementation.**
  Implement `aliro_on_select(aid, aid_len, user_ctx)`:
  1. Call `session_reset()` (unconditional — reset any prior state; handles the case where the same service is re-selected mid-session).
  2. Compare `aid[0..aid_len-1]` against `k_aliro_stepup_aid`. If match: build `{0x6A, 0x81}` in `s_resp_buf`; call `nfc_transport_send_response`; `STATS_INC(select_stepup_count)`; return.
  3. (Expedited AID) Generate `card_nonce`: `psa_generate_random(s_session.card_nonce, ALIRO_NONCE_SIZE)`. On PSA error: send SW `6F00`, `STATS_ERROR`, stay IDLE, return.
  4. Build SELECTResponse per §7.8 using `build_select_response(s_resp_buf, sizeof(s_resp_buf))`.
  5. `smf_set_state(&s_session.smf, &k_aliro_states[ALIRO_SESSION_STATE_AWAIT_AUTH0])`.
  6. `nfc_transport_send_response(s_resp_buf, resp_len)`.
  7. `STATS_INC(select_expedited_count)`.

  **Tests** (extend Task 3 suite):
  - `test_select_expedited_calls_generate_random`: verify PSA random is called.
  - `test_select_stepup_never_calls_random`: step-up → no PSA call.
  - `test_select_expedited_response_length`: response length = 2 + 65 + 2 = 69 (per §7.8 stub).

  **Commit:** "nfc/aliro: implement on_select"

### Deferred Crypto — AUTH0

- [ ] **Task 6 — `on_apdu` dispatch skeleton + AUTH0 work item.**

  Implement `aliro_on_apdu(apdu, user_ctx)`:
  1. Read `atomic_get(&s_crypto_inflight)`. If 1: send SW `6985`; `STATS_INC(crypto_busy_reject_count)`; `smf_set_state(ERROR)` (helper `session_set_error(SW_6985)`); return.
  2. Run `smf_run_state(&s_session.smf)` — the SMF entry function dispatches on current state + APDU type.
  3. The SMF state run functions for each state check `apdu->cla` and `apdu->ins` and either:
     - Accept (copy data, submit work, set inflight).
     - Reject (build SW, send response, transition to ERROR).

  Implement `auth0_crypto_handler()` (called from `s_crypto_work` when `pending_ins == ALIRO_INS_AUTH0`):
  ```
  1. Parse AUTH0 data from s_session.reader_nonce/reader_eph_pubkey (already copied in on_apdu).
  2. psa_generate_key(ephemeral P-256 ECDH, &s_session.card_eph_key_id).
  3. psa_export_public_key(s_session.card_eph_key_id, s_session.card_eph_pubkey, 65, &exported_len).
  4. Build AUTH0Response: s_session.card_eph_pubkey(65) || s_session.card_nonce(16) || SW(2).
  5. smf_set_state(AWAIT_AUTH1).
  6. nfc_transport_send_response(s_resp_buf, resp_len).
  7. STATS_INC(auth0_count).
  8. atomic_set(&s_crypto_inflight, 0).
  On any PSA error: send SW 6F00; smf_set_state(ERROR); STATS_ERROR(code); atomic_set inflight=0.
  ```

  **Tests** (mock PSA calls with test shims that return PSA_SUCCESS and fill fixed output):
  - `test_auth0_work_builds_response_83bytes`: after on_apdu(AUTH0) + run_work → send_response called with len=83.
  - `test_auth0_work_transitions_to_await_auth1`: FSM state after work = AWAIT_AUTH1.
  - `test_auth0_work_psa_error_sends_6f00`: mock generate_key → PSA_ERROR_GENERIC_ERROR → response = SW `6F00`, state = ERROR.
  - `test_auth0_work_clears_inflight`: after work completes, `s_crypto_inflight == 0`.

  **Commit:** "nfc/aliro: implement on_apdu dispatch + AUTH0 deferred crypto"

### Deferred Crypto — AUTH1

- [ ] **Task 7 — AUTH1 work item (ECDH + HKDF + ECDSA sign).**

  Implement `auth1_crypto_handler()`:
  ```
  1. Parse AUTH1 signature from s_session.auth1_sig (64B, copied in on_apdu).
  2. Construct transcript (§7.10): reader_nonce||card_nonce||reader_eph_pubkey||card_eph_pubkey.
  3. Import reader ephemeral pubkey as volatile PSA key (temp_reader_eph_key_id).
  4. Verify reader ECDSA sig over transcript using temp_reader_eph_key_id.
      - On PSA_ERROR_INVALID_SIGNATURE: send SW 6982; smf→ERROR; STATS_INC auth_reader_fail_count; goto cleanup.
  5. psa_raw_key_agreement(PSA_ALG_ECDH, s_session.card_eph_key_id, reader_eph_pubkey, 65, shared_secret, 32).
  6. Derive AES-256 session key from shared_secret via HKDF-SHA-256:
      psa_key_derivation_setup(PSA_ALG_HKDF(PSA_ALG_SHA_256));
      input_bytes(SALT, NULL, 0);           [or nonces-based salt — DECISION-12]
      input_key(SECRET, shared_secret_key_id);
      input_bytes(INFO, "Aliro-Session-Key", 17);  [— DECISION-12: exact info string]
      output_key(AES-256, &s_session.session_aes_key_id).
  7. psa_sign_message(NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                      transcript, 162, card_sig, 64, &sig_len).
  8. Build AUTH1Response: card_sig(64) || SW(2).
  9. smf_set_state(AWAIT_EXCHANGE).
  10. nfc_transport_send_response(s_resp_buf, 66).
  11. STATS_INC(auth1_count).
  cleanup:
      psa_destroy_key(temp_reader_eph_key_id);
      psa_destroy_key(s_session.card_eph_key_id);  [ephemeral done; only session key remains]
      atomic_set(&s_crypto_inflight, 0).
  ```

  **Tests** (mock PSA chain):
  - `test_auth1_reader_sig_verified_transitions_await_exchange`.
  - `test_auth1_reader_sig_fail_sends_6982_and_errors`.
  - `test_auth1_ecdh_fail_sends_6f00`.
  - `test_auth1_clears_eph_key_after_completion`.
  - `test_auth1_response_length_66bytes`.

  **Commit:** "nfc/aliro: implement AUTH1 deferred crypto (ECDH + HKDF + ECDSA)"

### Deferred Crypto — EXCHANGE

- [ ] **Task 8 — EXCHANGE work item (AES-GCM).**

  Implement `exchange_crypto_handler()`:
  ```
  1. AES-256-GCM decrypt s_session.exchange_data using s_session.session_aes_key_id.
     Nonce and AAD placement: DECISION-12 (confirm from Aliro spec).
     On PSA_ERROR_INVALID_SIGNATURE (bad tag): send SW 6982; smf→ERROR; goto cleanup.
  2. Process decrypted command (minimal: always respond "access granted" status byte 0x00).
  3. AES-256-GCM encrypt response status byte.
  4. Build ExchangeResponse: IV(12) || ciphertext || tag(16) || status_byte(1) || SW(2).
     **DECISION-9 cont.**: exact ExchangeResponse layout from Aliro 0.9.4 spec required.
  5. smf_set_state(DONE).
  6. nfc_transport_send_response(s_resp_buf, resp_len).
  7. STATS_INC(exchange_count); STATS_INC(session_complete_count).
  cleanup:
      psa_destroy_key(s_session.session_aes_key_id);
      atomic_set(&s_crypto_inflight, 0).
  ```

  **Tests:**
  - `test_exchange_decrypts_and_responds_done`.
  - `test_exchange_bad_tag_sends_6982`.
  - `test_exchange_clears_session_aes_key`.

  **Commit:** "nfc/aliro: implement EXCHANGE deferred crypto (AES-GCM)"

### Provisioning + Serialize/Deserialize

- [ ] **Task 9 — `aliro_service_provision_credential` + serialize/deserialize.**

  Implement provisioning per §3. Implement serialize/deserialize per §1.6:

  **`aliro_serialize(out, max, out_len, user_ctx)`:**
  ```c
  if (max < ALIRO_SERVICE_MAX_SERIALIZED) return -ENOSPC;
  uint8_t pubkey[ALIRO_P256_PUBLIC_KEY_SIZE];
  size_t pubkey_len = 0U;
  psa_status_t st = psa_export_public_key(NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE,
                                          pubkey, sizeof(pubkey), &pubkey_len);
  if (st != PSA_SUCCESS) { /* key not provisioned: serialize zeros */ memset(pubkey, 0, sizeof(pubkey)); pubkey_len = ALIRO_P256_PUBLIC_KEY_SIZE; }
  out[0] = 0x01U;  /* format version */
  out[1] = 0x00U;  /* features high byte */
  out[2] = 0x00U;  /* features low byte */
  out[3] = s_config.protocol_version_major;
  out[4] = s_config.protocol_version_minor;
  memcpy(&out[5], pubkey, ALIRO_P256_PUBLIC_KEY_SIZE);
  *out_len = 5U + ALIRO_P256_PUBLIC_KEY_SIZE;  /* = 70 */
  return 0;
  ```

  **`aliro_deserialize(in, len, user_ctx)`:**
  Checks format version byte, restores `s_config.protocol_version_*`, stores the
  deserialized public key into `s_credential_pubkey` (§4 module state) for
  SELECTResponse building. Does NOT restore private key. Precedence rule: once
  `s_credential_pubkey` is populated by deserialization, it is used for
  SELECTResponse; if all-zero (no deserialization), SELECTResponse derives the
  pubkey on-demand via `psa_export_public_key` per §7.8.

  **Tests:**
  - `test_provision_imports_psa_key`.
  - `test_provision_replace_existing`.
  - `test_serialize_format_version_1`.
  - `test_serialize_pubkey_zeros_if_not_provisioned`.
  - `test_deserialize_restores_version`.
  - `test_deserialize_bad_version_returns_ebadmsg`.

  **Commit:** "nfc/aliro: provision, serialize, deserialize"

### Shell Commands

- [ ] **Task 10 — Shell subcommands.**

  In `aliro_service_shell_cmds.c`:
  ```c
  SHELL_STATIC_SUBCMD_SET_CREATE(aliro_cmds,
      SHELL_CMD(config,    NULL, "Print config",   cmd_aliro_config),
      SHELL_CMD(stats,     NULL, "Print stats",    cmd_aliro_stats),
      SHELL_CMD(state,     NULL, "Print state",    cmd_aliro_state),
      SHELL_CMD(provision, NULL,
                "Import credential private key: provision <32-byte hex>",
                cmd_aliro_provision),
      SHELL_SUBCMD_SET_END);
  SHELL_CMD_REGISTER(aliro, &aliro_cmds, "Aliro credential service", NULL);
  ```

  - `cmd_aliro_config`: `shell_print` the frozen config fields.
  - `cmd_aliro_stats`: `shell_print` all stats fields.
  - `cmd_aliro_state`: `shell_print` the state enum name.
  - `cmd_aliro_provision`: parse `argv[1]` as 64-char hex string → 32 bytes; call `aliro_service_provision_credential(bytes, 32)`. Print success or error.

  **Tests** (command-parser unit tests — pure logic, no PSA needed):
  - `test_provision_cmd_parser_valid_64char_hex`: verify hex decode of `"0102...1f20"` (64 chars) → correct 32 bytes.
  - `test_provision_cmd_parser_odd_length_rejects`.
  - `test_provision_cmd_parser_non_hex_char_rejects`.
  - `test_provision_cmd_parser_short_input_rejects`.

  **Commit:** "nfc/aliro: shell commands including provision"

### Integration

- [ ] **Task 11 — Wire into `src/nfc/Kconfig` and `src/nfc/CMakeLists.txt`.**

  The Wave 4 plan (§5.5) already defines the CMake and Kconfig hooks:
  ```cmake
  # src/nfc/CMakeLists.txt (already defined by Wave 4 §5.5)
  if(CONFIG_NFC_SERVICE_ALIRO)
      add_subdirectory(services/aliro)
  endif()
  ```
  ```kconfig
  # src/nfc/Kconfig (PSA_WANT_ALG_HKDF added by wave4 §1.6 — capstone F-04; all other selects already present)
  config NFC_SERVICE_ALIRO
      bool "Aliro credential service"
      depends on NFC_STACK
      select PSA_WANT_ALG_ECDH
      select PSA_WANT_ALG_HKDF
      select PSA_WANT_ALG_SHA_256
      select PSA_WANT_ALG_ECDSA
      select PSA_WANT_ALG_GCM
      select PSA_WANT_KEY_TYPE_ECC_KEY_PAIR
      rsource "services/aliro/Kconfig"
  ```

  Add the `rsource "services/aliro/Kconfig"` line under `NFC_SERVICE_ALIRO` and the CMake directive. Confirm wave4's existing stubs in `nfc_stack.c` now resolve (`aliro_service_get()` is defined). **Commit.**

- [ ] **Task 12 — Verify `nfc_transport_submit_work` in HAL (already satisfied by wave1, prototype 8 of 12, task 14).**

  Confirm `src/nfc/hal/nfc_transport.h` already declares:
  ```c
  int nfc_transport_submit_work(struct k_work *work);  /* @threadsafe; NULL-check inside */
  ```
  and `src/nfc/hal/nfc_transport_nrfx.c` provides the implementation. No code changes needed. **Verify only; no commit required.**

- [ ] **Task 13 — Build smoke test (all-services config, nRF54L15 DK).**
  Build targeting `nrf54l15dk/nrf54l15/cpuapp` with:
  ```
  CONFIG_NFC_STACK=y
  CONFIG_NFC_SERVICE_ALIRO=y
  CONFIG_NFC_SERVICE_NDEF=y
  CONFIG_NFC_STACK_DEFAULT_PROFILE=5   # NFC_PROFILE_ALL
  CONFIG_NFC_ROUTER_MAX_AIDS=8
  CONFIG_PSA_CRYPTO_DRIVER_CRACEN=y
  PSA_WANT_ALG_ECDH=y
  PSA_WANT_ALG_HKDF=y
  PSA_WANT_ALG_SHA_256=y
  PSA_WANT_ALG_ECDSA=y
  PSA_WANT_ALG_GCM=y
  PSA_WANT_KEY_TYPE_ECC_KEY_PAIR=y
  ```
  Confirm: no linker errors; `aliro_service_get()` resolves in `nfc_stack.c`; `ALIRO_SERVICE_MAX_SERIALIZED=128` BUILD_ASSERT passes (if added). **Commit.**

- [ ] **Task 14 — MISRA / linter sweep.**
  Run cppcheck (or project linter) on `aliro_service.c` and `aliro_service_shell_cmds.c`. Verify:
  - No static local variables (`s_*` are file-scope — compliant).
  - All `switch` statements have `default`.
  - All PSA return codes checked.
  - No implicit function declarations.
  - DEV-ZEP suppression comments at each `LOG_*`, `shell_print`, SMF macro site.
  Confirm `BUILD_ASSERT` PSA-key-base-overlap guard fires on intentional collision (unit test). **Commit.**

- [ ] **Task 15 — On-target integration test (nRF54L15 DK with Aliro-compatible reader app).**
  Provision a test P-256 keypair via shell. Tap with a known-good Aliro reader app (Android/iOS with Aliro SDK). Verify:
  - SELECT → SELECTResponse received.
  - AUTH0 → AUTH0Response within FWI window (~5s measured < 20ms actual).
  - AUTH1 → AUTH1Response with valid ECDSA signature.
  - EXCHANGE → ExchangeResponse; reader shows "Access Granted".
  Run Thread Analyzer; update `NFC_HAL_WORKQ_STACK_SIZE` if peak exceeds default 2048 B.
  Document measured latencies in the HAL README. **Commit.**

---

## Appendix A: PSA Call Sequence — AUTH0 Work Handler

```c
/* auth0_crypto_handler() — runs on nfc_work_q after AUTH0 on_apdu */
static void auth0_crypto_handler(void)
{
    psa_status_t     st       = PSA_SUCCESS;
    psa_key_id_t     eph_id   = PSA_KEY_ID_NULL;
    size_t           pub_len  = 0U;
    size_t           resp_len = 0U;

    /* 1. Generate card ephemeral P-256 key pair (ECDH usage) */
    psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attrs, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attrs, 256U);
    psa_set_key_algorithm(&attrs, PSA_ALG_ECDH);
    psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_DERIVE);
    st = psa_generate_key(&attrs, &eph_id);
    if (st != PSA_SUCCESS) { goto err; }
    s_session.card_eph_key_id = eph_id;

    /* 2. Export card ephemeral public key (65 B uncompressed) */
    st = psa_export_public_key(eph_id, s_session.card_eph_pubkey,
                               ALIRO_P256_PUBLIC_KEY_SIZE, &pub_len);
    if (st != PSA_SUCCESS || pub_len != ALIRO_P256_PUBLIC_KEY_SIZE) { goto err; }

    /* 3. Build AUTH0Response:
     *    card_eph_pubkey(65) || card_nonce(16) || SW_OK(2) = 83 bytes */
    memcpy(&s_resp_buf[0], s_session.card_eph_pubkey, ALIRO_P256_PUBLIC_KEY_SIZE);
    memcpy(&s_resp_buf[ALIRO_P256_PUBLIC_KEY_SIZE], s_session.card_nonce, ALIRO_NONCE_SIZE);
    s_resp_buf[ALIRO_P256_PUBLIC_KEY_SIZE + ALIRO_NONCE_SIZE + 0U] = NFC_SW1(NFC_SW_OK);
    s_resp_buf[ALIRO_P256_PUBLIC_KEY_SIZE + ALIRO_NONCE_SIZE + 1U] = NFC_SW2(NFC_SW_OK);
    resp_len = ALIRO_P256_PUBLIC_KEY_SIZE + ALIRO_NONCE_SIZE + 2U; /* = 83 */

    smf_set_state(SMF_CTX(&s_session), &k_aliro_states[ALIRO_SESSION_STATE_AWAIT_AUTH1]);
    (void)nfc_transport_send_response(s_resp_buf, resp_len);
    STATS_INC(&s_stats_lock, s_stats, auth0_count);
    atomic_set(&s_crypto_inflight, 0);
    return;

err:
    STATS_ERROR(&s_stats_lock, s_stats, -(int32_t)st);
    STATS_INC(&s_stats_lock, s_stats, crypto_error_count);
    s_resp_buf[0] = NFC_SW1(NFC_SW_UNKNOWN);
    s_resp_buf[1] = NFC_SW2(NFC_SW_UNKNOWN);
    (void)nfc_transport_send_response(s_resp_buf, 2U);
    smf_set_state(SMF_CTX(&s_session), &k_aliro_states[ALIRO_SESSION_STATE_ERROR]);
    atomic_set(&s_crypto_inflight, 0);
}
```

---

## Appendix B: Open DECISIONs Summary

| ID | Topic | Status |
|----|-------|--------|
| **DECISION-1** | Language boundary: pure-C service, PSA calls reimplemented inline | **Resolved** |
| **DECISION-2** | Use `nfc_work_q` not `AliroWorkQ` | **Resolved** |
| **DECISION-3** | `nfc_transport_submit_work` HAL accessor | **Resolved — already satisfied by wave1** (prototype 8 of 12, task 14; `@threadsafe`, NULL-check present) |
| **DECISION-4** | In-flight guard rejects with SW `6985` | **Resolved** |
| **DECISION-5** | PSA key ID base at `0x2000`; Kconfig-tunable; BUILD_ASSERT enforces full range-disjointness vs the DOOR_LOCK 64KB range | **Resolved** |
| **DECISION-6** | Private key never in store blob; serialize only public key + config | **Resolved** |
| **DECISION-7** | Step-up AID: stub SW `6A81` until spec §12 items are in scope | **Resolved** |
| **DECISION-8** | SELECTResponse sent by `on_select` (router DECISION-B); step-up returns `6A81` | **Resolved** |
| **DECISION-9** | EXCHANGE INS=`0x82` — **must verify vs Aliro 0.9.4 spec** | ⚠️ Unverified |
| **DECISION-10** | AUTH0 data layout: reader_nonce(16) || reader_eph_pubkey(65) — **must verify** | ⚠️ Unverified |
| **DECISION-11** | SELECTResponse TLV encoding — **must verify vs spec §SELECT_RESPONSE** | ⚠️ Unverified |
| **DECISION-12** | AUTH1 transcript construction + HKDF info string — **must verify vs spec §AUTH1** | ⚠️ Unverified |

> The four unverified decisions are gated at build time by
> `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED` (§7.10a): until it is set to `y`, the build
> emits a `#warning` so unverified constants cannot silently reach hardware testing.
