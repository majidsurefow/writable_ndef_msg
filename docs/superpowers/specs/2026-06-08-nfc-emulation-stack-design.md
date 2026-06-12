# NFC Emulation Stack Design

**Date:** 2026-06-08  
**Project:** writable_ndef_msg (nRF52840, Nordic NCS v3.2.2)  
**Status:** Approved for implementation — updated with profile switching and reference paths

---

## 1. Scope

The nRF52840 NFCT peripheral is a **card emulator only** (NFC-A, ISO-DEP). It cannot poll or read cards. This stack implements the **credential/card side** — the nRF presents itself as a smart card that NFC readers tap.

The stack replaces the existing `src/nfc_emulation.c/h` and provides a layered architecture for multi-protocol ISO-DEP card emulation with UID rotation.

---

## 2. Hardware Constraints

| Capability | Status |
|---|---|
| NFC-A card emulation (ISO-DEP / T4T) | ✅ Supported |
| UID rotation (4, 7, 10 byte NFCID1) | ✅ Supported |
| Multi-protocol via AID dispatch | ✅ Supported |
| NFC reader / polling mode | ❌ Not supported (no external reader chip) |
| NFC-B, NFC-F, NFC-V | ❌ Not supported (hardware limitation) |
| MF Classic (ISO 14443-3A + Crypto1) | ❌ Not supported (no T2T command path) |
| FeliCa, ISO 15693, ST25TB | ❌ Not supported (wrong RF type) |

---

## 3. Protocol Coverage

| Service | AID | Implementation | Notes |
|---|---|---|---|
| NDEF T4T | `D2 76 00 00 85 01 01` | From scratch | Raw PICC mode only (no `ndef_rwpayload_set`) |
| MF DeSFire EV1/EV2 | `D2 76 00 00 85 01 00` (PICC master AID) | From scratch | Full AES auth (EV1: `0xAA`, EV2: `0x71`/`0x77`); internal app selection via `0x5A` |
| MF Ultralight | No AID | NDEF wrapper only | `uint8_t pages[N][4]` data model; T2T commands not possible on ISO-DEP hardware |
| EMV PPSE | `2P AY.SYS.DDF01` = `32 50 41 59 2E 53 59 53 2E 44 44 46 30 31` | Reuse `nfc/helpers/nfc_emv_parser.c` | Payment card emulation |
| Aliro credential | Expedited: `A0 00 00 08 58 01 01 01 00`<br>Step-up: `A0 00 00 08 58 01 01 02 00` | From scratch | PSA crypto reused from `aliro/interface_impl/crypto.cpp` |

---

## 4. Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  main.c                                                         │
│  nfc_stack_init() / nfc_stack_start(uid) / nfc_stack_stop()    │
│  nfc_stack_set_uid(uid)                                         │
├─────────────────────────────────────────────────────────────────┤
│  src/nfc/nfc_stack.c/.h          ← public wiring layer         │
├────────────────┬────────────────────────────────────────────────┤
│  services/     │  ndef/   desfire/   ultralight/   emv/  aliro/ │
├────────────────┴────────────────────────────────────────────────┤
│  router/           aid_router.c/.h   service.h                  │
│  SELECT AID → registered service lookup → dispatch APDUs        │
├─────────────────────────────────────────────────────────────────┤
│  framing/          apdu_assembler.c/.h   apdu_types.h           │
│  DATA_IND fragments → complete APDU → router                    │
│  router response → nfc_t4t_response_pdu_send()                  │
├─────────────────────────────────────────────────────────────────┤
│  hal/              nfc_transport.c/.h                           │
│  T4T lifecycle · UID rotation · raw field events up             │
├─────────────────────────────────────────────────────────────────┤
│  nrfxlib nfc_t4t_lib   (libnfc_t4t.a — closed binary)          │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. Directory Structure

```
src/
├── main.c                          (unchanged public entry point)
├── stats.h                         (existing — used by all layers)
└── nfc/
    ├── nfc_stack.c
    ├── nfc_stack.h
    ├── hal/
    │   ├── nfc_transport.c
    │   └── nfc_transport.h
    ├── framing/
    │   ├── apdu_assembler.c
    │   ├── apdu_assembler.h
    │   └── apdu_types.h
    ├── router/
    │   ├── aid_router.c
    │   ├── aid_router.h
    │   └── service.h
    └── services/
        ├── ndef/
        │   ├── ndef_service.c
        │   └── ndef_service.h
        ├── desfire/
        │   ├── desfire_service.c
        │   ├── desfire_service.h
        │   └── desfire_data.h
        ├── ultralight/
        │   ├── ultralight_service.c
        │   └── ultralight_service.h
        ├── emv/
        │   ├── emv_service.c
        │   └── emv_service.h
        └── aliro/
            ├── aliro_service.c
            └── aliro_service.h
```

---

## 6. Layer Specifications

### 6.1 HAL Layer (`hal/`)

**Responsibility:** Own the nrfxlib T4T library lifecycle, UID rotation, and deliver raw events upward. Nothing else.

**Lifecycle states:**
```
UNINIT ──nfc_transport_init()──► IDLE
IDLE   ──nfc_transport_start()─► RUNNING
RUNNING──nfc_transport_stop()──► IDLE
IDLE   ──nfc_transport_deinit()► UNINIT
```

**Public API:**
```c
int  nfc_transport_init(const nfc_transport_callbacks_t *cbs);
int  nfc_transport_start(const nfc_uid_t *uid);
int  nfc_transport_stop(void);
void nfc_transport_deinit(void);
int  nfc_transport_set_uid(const nfc_uid_t *uid); /* stop → set NFCID1 → start */
int  nfc_transport_send_response(const uint8_t *buf, size_t len);
int  nfc_transport_get_stats(nfc_transport_stats_t *out);
```

**Callbacks delivered upward:**
```c
typedef struct {
    void (*on_field_on)(void *ctx);
    void (*on_field_off)(void *ctx);
    void (*on_fragment)(void *ctx, const uint8_t *data, size_t len, bool more); /* DATA_IND */
    void *ctx;
} nfc_transport_callbacks_t;
```

**Key nrfxlib contracts respected:**
- Raw PICC mode = `nfc_t4t_setup()` + `nfc_t4t_emulation_start()` with no payload set call. Header: `/opt/nordic/ncs/v3.2.4/nrfxlib/nfc/include/nfc_t4t_lib.h`
- `DATA_IND` delivers PCB-stripped APDU fragments. `data` pointer must be copied immediately — lifetime undocumented.
- `NFC_T4T_DI_FLAG_MORE` set = more fragments follow. Clear = final fragment.
- `nfc_t4t_response_pdu_send(buf, len)` borrows `buf` until the next callback — caller must hold it valid.
- UID rotation: `nfc_t4t_emulation_stop()` → `nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, ...)` → `nfc_t4t_emulation_start()`.
- FWI set to 8 (max, ~5s window) before first `emulation_start()`.
- Callback fires from ISR context.

**Stats struct:**
```c
typedef struct {
    uint32_t field_on_count;
    uint32_t field_off_count;
    uint32_t fragment_rx_count;
    uint32_t response_tx_count;
    uint32_t uid_rotation_count;
    uint32_t error_count;
    int32_t  last_error_code;
} nfc_transport_stats_t;
```

---

### 6.2 Framing Layer (`framing/`)

**Responsibility:** Accumulate `DATA_IND` fragments into a complete APDU, bounds-check, forward to router. Accept flat response buffer from router, call `nfc_transport_send_response()`.

**What this layer does NOT do:** PCB parsing, WTX, R-block handling, CRC — nrfxlib handles all of this internally.

**APDU accumulation contract:**
- On each `on_fragment(data, len, more=true)`: copy `data[0..len-1]` into static assembly buffer, append.
- On `on_fragment(data, len, more=false)`: append final fragment. If assembled length > `NFC_APDU_MAX_LEN` (32767), respond with SW `6700` (Wrong Length) and discard.
- Forward complete APDU to `aid_router_dispatch()`.
- On `on_field_off`: reset assembly buffer, notify router.

**Thread model:** Fragment copy and work submission happen in ISR. All assembly, routing, and service logic run in a dedicated high-priority k_work queue (`nfc_work_q`). Response buffer lives in static service memory — valid from work submission until the next `DATA_IND` callback.

**APDU types (`apdu_types.h`):**
```c
#define NFC_APDU_MAX_LEN        32767U   /* ISO 7816-4 extended APDU max */
#define NFC_SW_OK               0x9000U
#define NFC_SW_FILE_NOT_FOUND   0x6A82U
#define NFC_SW_WRONG_LENGTH     0x6700U
#define NFC_SW_INS_NOT_SUPPORTED 0x6D00U
#define NFC_SW_CONDITIONS_NOT_SATISFIED 0x6985U
#define NFC_SW_SECURITY_STATUS  0x6982U

typedef struct {
    uint8_t  buf[NFC_APDU_MAX_LEN];
    uint16_t len;
} nfc_apdu_buf_t;
```

**Stats struct:**
```c
typedef struct {
    uint32_t apdu_assembled_count;
    uint32_t apdu_oversized_count;
    uint32_t response_sent_count;
    uint32_t error_count;
    int32_t  last_error_code;
} nfc_framing_stats_t;
```

---

### 6.3 Router Layer (`router/`)

**Responsibility:** Maintain an AID registration table. On SELECT AID, look up the matching service and call `on_select`. Route all subsequent APDUs to the active service until DESELECT or field-off.

**Service vtable (`service.h`):**
```c
typedef void (*nfc_service_on_select_fn)(void *ctx,
                                         const uint8_t *aid, size_t aid_len);
typedef void (*nfc_service_on_apdu_fn)(void *ctx,
                                       const uint8_t *cmd, size_t cmd_len);
typedef void (*nfc_service_on_deselect_fn)(void *ctx);
typedef void (*nfc_service_on_field_off_fn)(void *ctx);

typedef struct {
    nfc_service_on_select_fn    on_select;
    nfc_service_on_apdu_fn      on_apdu;     /* calls nfc_transport_send_response() internally */
    nfc_service_on_deselect_fn  on_deselect;
    nfc_service_on_field_off_fn on_field_off;
    void *ctx;
} nfc_service_t;
```

**Service send pattern (Flipper-aligned):** Each service calls `nfc_transport_send_response(buf, len)` imperatively inside `on_apdu`. The service owns a static response buffer. This correctly handles both synchronous (NDEF, Ultralight) and deferred (Aliro — crypto on work queue) response paths.

**Router API:**
```c
int  aid_router_init(void);
int  aid_router_register(const uint8_t *aid, size_t aid_len,
                          const nfc_service_t *svc);
void aid_router_clear(void);      /* deregisters all services — called by nfc_stack_set_profile() */
void aid_router_dispatch(const uint8_t *apdu, size_t len);
void aid_router_field_off(void);
```

**Dispatch logic:**
- First APDU in a session: check for SELECT AID (`CLA=00 INS=A4 P1=04`). Extract AID from Lc+data field. Linear scan of registration table. If match: call `svc->on_select(aid, aid_len)`, mark as active service.
- Subsequent APDUs: forward to active service `on_apdu`.
- If no service matched SELECT: respond with `6A82` (File Not Found).
- `FIELD_OFF`: call active service `on_field_off`, clear active service.

**Registration:** One AID per table entry. Multiple entries may point to the same `nfc_service_t` (used by Aliro — two AIDs, one service). Table size defined by `CONFIG_NFC_ROUTER_MAX_AIDS` (default 8).

**Stats struct:**
```c
typedef struct {
    uint32_t select_matched_count;
    uint32_t select_unmatched_count;
    uint32_t apdu_routed_count;
    uint32_t field_off_count;
    uint32_t error_count;
    int32_t  last_error_code;
} nfc_router_stats_t;
```

---

### 6.4 Services Layer (`services/`)

All services follow the same vtable. Each service owns a static response buffer. Each service calls `nfc_transport_send_response()` imperatively when the response is ready.

---

#### 6.4.1 NDEF Service (`services/ndef/`)

**AID:** `D2 76 00 00 85 01 01` (NFC Forum T4T NDEF Application)

**Scope:** Implements the T4T NDEF file system: Capability Container (CC) file and NDEF data file. Supports read and write. Compatible with any NFC-Forum T4T reader.

**Commands handled:**
- `SELECT AID` — respond with `9000`
- `SELECT FILE` (by file ID) — select CC (`E103`) or NDEF file (`E104`)
- `READ BINARY` — return file contents
- `UPDATE BINARY` — write NDEF file contents (if write access permitted)

**Data:** Static NDEF buffer (size `CONFIG_NFC_NDEF_MAX_SIZE`, default 256 bytes). NLEN field (first 2 bytes) maintained by service.

**Implementation:** Written from scratch. Must use raw PICC mode — do not call `nfc_t4t_ndef_rwpayload_set()` as that bypasses `DATA_IND` entirely.

---

#### 6.4.2 DeSFire Service (`services/desfire/`)

**AID:** `D2 76 00 00 85 01 00` (7 bytes — NXP DeSFire PICC master AID, registered with the router). Once the router dispatches to this service, internal DeSFire application selection uses `SELECT_APPLICATION (0x5A)` with a 3-byte app ID handled entirely within the service.

**Scope:** Full DeSFire EV1/EV2 emulation with AES authentication.

**Commands handled:**
- `GET_VERSION (0x60)` — hardware/software version, UID, batch number
- `GET_FREE_MEMORY (0x6E)`
- `GET_KEY_SETTINGS (0x45)`
- `GET_KEY_VERSION (0x64)`
- `GET_APPLICATION_IDS (0x6A)`
- `SELECT_APPLICATION (0x5A)`
- `GET_FILE_IDS (0x6F)`
- `GET_FILE_SETTINGS (0xF5)`
- `READ_DATA (0xBD)` — with optional AES encryption
- `GET_VALUE (0x6C)`
- `READ_RECORDS (0xBB)`
- `AUTHENTICATE_AES (0xAA)` — EV1 mutual authentication
- `AuthenticateEV2First (0x71)` — EV2 auth phase 1
- `AuthenticateEV2NonFirst (0x77)` — EV2 subsequent auth
- Continuation frame `0xAF` — for multi-frame responses

**Auth state machine (per session):**
```
IDLE → AUTH_STEP1 → AUTH_STEP2 → AUTHENTICATED
```
- EV1: 3-pass AES mutual auth. Session key derived from card key + RndA + RndB.
- EV2: SesAuthENC + SesAuthMAC derived via CMAC-AES from shared secret.

**Crypto:** PSA Crypto API — AES-128 ECB/CBC (`psa_cipher_*`), CMAC (`psa_mac_*`). Already available on nRF52840.

**Data model (`desfire_data.h`):**
```c
typedef struct { uint8_t data[3]; } desfire_app_id_t;

typedef struct {
    uint8_t type;            /* Standard, Backup, Value, Record, TransactionMAC */
    uint8_t comm_settings;   /* Plain, MAC, Encrypted */
    uint16_t access_rights;
    union { /* file type specific size/limits */ } settings;
} desfire_file_settings_t;

typedef struct {
    desfire_app_id_t    app_id;
    uint8_t             key_settings;
    uint8_t             key_count;
    uint8_t             keys[14][16];    /* up to 14 AES-128 keys */
    desfire_file_settings_t file_settings[32];
    uint8_t             file_data[32][256]; /* file contents */
    uint8_t             file_count;
} desfire_application_t;

typedef struct {
    uint8_t               hw_version[7];
    uint8_t               sw_version[7];
    uint8_t               uid[7];
    uint8_t               batch[5];
    desfire_application_t apps[28];
    uint8_t               app_count;
    uint32_t              free_memory;
} desfire_data_t;
```

**Reference:** Flipper Zero `mf_desfire_poller_defs.h` for command byte values and data model field layout.

---

#### 6.4.3 Ultralight Service (`services/ultralight/`)

**AID:** None — presented as NDEF content via NDEF service. Not a standalone ISO-DEP applet.

**Scope:** NDEF data wrapper only. Stores Ultralight page data (`uint8_t pages[N][4]`) and exposes it as a T4T NDEF file. T2T commands (`READ 0x30`, `WRITE 0xA2`, etc.) are not possible on ISO-DEP hardware.

**Implementation:** The Ultralight service registers no AID. Instead, the NDEF service reads its page contents to construct the NDEF payload. `ultralight_service_get_ndef(buf, max_len)` is called by the NDEF service at init or when the NDEF file is regenerated.

---

#### 6.4.4 EMV Service (`services/emv/`)

**AID:** `2P AY.SYS.DDF01` = `32 50 41 59 2E 53 59 53 2E 44 44 46 30 31` (PPSE — Proximity Payment System Environment)

**Scope:** EMV PPSE directory emulation. Returns a list of payment application AIDs to the reader. Sufficient for basic payment card detection by readers.

**Commands handled:**
- `SELECT PPSE AID` — return PPSE FCI with registered payment AIDs
- Per-payment-AID SELECT and GPO (GET PROCESSING OPTIONS) — stub responses

**Reuse:** `nfc/helpers/nfc_emv_parser.c` for TLV parsing. AID database from `nfc/resources/nfc/assets/aid.nfc`.

---

#### 6.4.5 Aliro Credential Service (`services/aliro/`)

**AIDs:**
- Expedited phase: `A0 00 00 08 58 01 01 01 00`
- Step-up phase: `A0 00 00 08 58 01 01 02 00`

Both AIDs registered separately in the router, both pointing to the same `nfc_service_t` instance. The service distinguishes phases from the AID in `on_select`.

**Scope:** Aliro 0.9.4 credential (card) side. Responds to reader-initiated SELECT and AUTH0/AUTH1 handshake. Grants access by completing the cryptographic session.

**State machine:**
```
IDLE
  → AWAIT_AUTH0     (after SELECT expedited AID → send SELECTResponse)
  → AWAIT_AUTH1     (after AUTH0 → send AUTH0Response with card ephemeral pubkey)
  → AWAIT_EXCHANGE  (after AUTH1 → send AUTH1Response with ECDSA signature)
  → DONE / ERROR    (after EXCHANGE → send ExchangeResponse)
```

**Standard session — 4 APDU round-trips:**
1. `SELECT AID (expedited)` → `SELECTResponse` (protocol versions, card info)
2. `AUTH0 (CLA=80 INS=80)` → `AUTH0Response` (card ephemeral P-256 pubkey)
3. `AUTH1 (CLA=80 INS=81)` → `AUTH1Response` (ECDSA signature over auth data)
4. `EXCHANGE` → `ExchangeResponse` (encrypted status)

**Fast session (pre-established keys) — 2-3 round-trips:** SELECT + AUTH0 with cryptogram + EXCHANGE.

**Step-up phase (if AccessDocument required):**
- Additional SELECT (step-up AID) + REQUEST DOCUMENT → CBOR-encoded AccessDocument.

**Crypto operations (all via PSA Crypto API):**

| Operation | PSA API | Purpose |
|---|---|---|
| ECDH P-256 | `psa_raw_key_agreement(PSA_ALG_ECDH)` | Shared secret from ephemeral keys |
| HKDF-SHA-256 | `psa_key_derivation_setup(PSA_ALG_HKDF(...))` | Session key derivation |
| ECDSA P-256 sign | `psa_sign_message(PSA_ALG_ECDSA(PSA_ALG_SHA_256))` | AUTH1 signature |
| AES-256-GCM | `psa_aead_encrypt/decrypt(PSA_ALG_GCM)` | EXCHANGE payload |
| SHA-256 | `psa_hash_compute(PSA_ALG_SHA_256)` | Digest operations |
| TRNG | `psa_generate_random()` | Ephemeral key nonces |

**Reuse from `aliro/`:**
- `aliro/interface_impl/crypto.cpp` — all PSA wrappers (`GenerateEphemeralKeyPair`, `RawKeyAgreement`, `DeriveSharedKey`, `AeadEncrypt`, `AeadDecrypt`, `GenerateSignature`, `VerifySignature`)
- `aliro/platform/crypto/utils.h/.cpp` — `ImportPrivateKey`, `ImportPublicKey`, `ExportPublicKey`
- `aliro/storage/psa_key_ids.h` — extend with credential key slot
- `aliro/aliro_work/aliro_work.h` — shared Aliro work queue

**Not reused:** `libaliro.a` (reader-only), `nfc_transport_rfal.cpp` (reader RF), `AccessManager`.

**Threading:** Crypto operations (ECDH + HKDF ~10ms on nRF52840) run on `nfc_work_q`. `on_apdu` submits work; response sent from work handler after crypto completes. FWI=8 (~5s) provides ample headroom.

---

## 7. Public Stack API (`nfc_stack.h`)

```c
/* One-time hardware and layer initialization. Call before nfc_stack_start(). */
int nfc_stack_init(void);

/* Start field sensing with the given initial UID. */
int nfc_stack_start(const nfc_uid_t *uid);

/* Stop field sensing. */
int nfc_stack_stop(void);

/* Rotate UID: stop → set NFCID1 → start. Safe to call while running. */
int nfc_stack_set_uid(const nfc_uid_t *uid);

/* Tear down. Call nfc_stack_stop() first. */
void nfc_stack_deinit(void);

/* Profile enum — which services are registered in the router.
 * Only profiles whose Kconfig is enabled are valid at runtime.
 * ALL registers every compiled-in service. */
typedef enum {
    NFC_PROFILE_NDEF,
    NFC_PROFILE_DESFIRE,
    NFC_PROFILE_ULTRALIGHT,
    NFC_PROFILE_EMV,
    NFC_PROFILE_ALIRO,
    NFC_PROFILE_ALL,
} nfc_profile_t;

/* Switch active profile. Pending flag set immediately; applied on next field-off.
 * In-memory only — does not persist across reboots.
 * Internally calls aid_router_clear() then re-registers the new profile's AIDs. */
int nfc_stack_set_profile(nfc_profile_t profile);
```

`main.c` usage:
```c
nfc_stack_init();
nfc_stack_start(&initial_uid);
/* ... main loop ... */
/* on button press: nfc_stack_set_profile(NFC_PROFILE_ALIRO) */
/* on field-off event: profile applied, call nfc_stack_set_uid() to rotate */
```

---

## 8. Thread Model

```
ISR (NFCT interrupt)
  │
  ├── FIELD_ON  → submit k_work to nfc_work_q → router/services notified
  ├── FIELD_OFF → submit k_work to nfc_work_q → router/services reset
  └── DATA_IND  → copy fragment bytes to static staging buf → submit k_work
                                                               │
                                              nfc_work_q thread (high priority)
                                                               │
                                              framing: assemble APDU
                                                               │
                                              router: dispatch to service
                                                               │
                                              service: process → build response
                                                in static resp_buf[]
                                                               │
                                              nfc_transport_send_response()
                                                → nfc_t4t_response_pdu_send()
                                                  (buf borrowed until next callback)
```

**Staging buffer:** Static `uint8_t` array in the framing layer, sized `NFC_APDU_MAX_LEN`. ISR copies each fragment immediately (nrfxlib buffer lifetime undocumented). Work handler assembles from staging buffer.

---

## 9. Kconfig

```kconfig
config NFC_STACK
    bool "NFC emulation stack"
    depends on NFC_T4T_NRFXLIB
    help
      Layered NFC card emulation stack (hal/framing/router/services).

config NFC_SERVICE_NDEF
    bool "NDEF T4T service"
    depends on NFC_STACK

config NFC_SERVICE_DESFIRE
    bool "MF DeSFire EV1/EV2 service"
    depends on NFC_STACK
    select PSA_WANT_ALG_AES
    select PSA_WANT_ALG_CMAC

config NFC_SERVICE_ULTRALIGHT
    bool "MF Ultralight NDEF wrapper service"
    depends on NFC_STACK && NFC_SERVICE_NDEF

config NFC_SERVICE_EMV
    bool "EMV PPSE service"
    depends on NFC_STACK

config NFC_SERVICE_ALIRO
    bool "Aliro credential service"
    depends on NFC_STACK
    select PSA_WANT_ALG_ECDH
    select PSA_WANT_ALG_ECDSA
    select PSA_WANT_ALG_SHA_256
    select PSA_WANT_ALG_GCM
    select PSA_WANT_KEY_TYPE_ECC_KEY_PAIR

config NFC_ROUTER_MAX_AIDS
    int "Maximum registered AIDs"
    default 8
    depends on NFC_STACK

config NFC_NDEF_MAX_SIZE
    int "NDEF file buffer size in bytes"
    default 256
    depends on NFC_SERVICE_NDEF
```

---

## 10. Reuse Map

| Source file | Full path | Used in | How |
|---|---|---|---|
| `aliro/interface_impl/crypto.cpp` | `/Users/majidfaroud/writable_ndef_msg/aliro/interface_impl/crypto.cpp` | `services/aliro/` | PSA crypto wrappers — reused as-is |
| `aliro/platform/crypto/utils.h/.cpp` | `/Users/majidfaroud/writable_ndef_msg/aliro/platform/crypto/utils.h` | `services/aliro/` | Key import/export |
| `aliro/storage/psa_key_ids.h` | `/Users/majidfaroud/writable_ndef_msg/aliro/storage/psa_key_ids.h` | `services/aliro/` | Extended with credential key slot |
| `aliro/aliro_work/aliro_work.h` | `/Users/majidfaroud/writable_ndef_msg/aliro/aliro_work/aliro_work.h` | `services/aliro/` | Shared work queue |
| `nfc/helpers/nfc_emv_parser.c` | `/Users/majidfaroud/writable_ndef_msg/nfc/helpers/nfc_emv_parser.c` | `services/emv/` | TLV/APDU parsing |
| `nfc/resources/nfc/assets/aid.nfc` | `/Users/majidfaroud/writable_ndef_msg/nfc/resources/nfc/assets/aid.nfc` | `services/emv/` | AID lookup database |
| `nfc/plugins/supported_cards/*.c` | `/Users/majidfaroud/writable_ndef_msg/nfc/plugins/supported_cards/` | Future card data parsing | Pure C parsers (Gallagher, HID, transit) |
| `src/stats.h` | `/Users/majidfaroud/writable_ndef_msg/src/stats.h` | All layers | Unified spinlock-guarded stats |
| `flipperzero mf_desfire` | `/Users/majidfaroud/flipperzero/lib/nfc/protocols/mf_desfire/` | `services/desfire/` | Command byte constants, data model reference |
| `flipperzero mf_ultralight` | `/Users/majidfaroud/flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_listener.c` | `services/ultralight/` | Page data model reference (`uint8_t pages[N][4]`) |
| nrfxlib nfc headers | `/opt/nordic/ncs/v3.2.4/nrfxlib/nfc/include/` | `hal/` | `nfc_t4t_lib.h`, `nfc_platform.h`, `nrf_nfc_errno.h` |
| Zephyr kernel headers | `/opt/nordic/ncs/v3.2.4/zephyr/include/` | All layers | k_work, k_spinlock, Kconfig |

---

## 11. What Is Not Implemented

| Protocol | Reason |
|---|---|
| MF Classic | Crypto1 cipher + ISO 14443-3A anticollision — hardware impossible on nRF NFCT |
| FeliCa | NFC-F technology — hardware impossible |
| ISO 15693 / SLIX | NFC-V technology — hardware impossible |
| ISO 14443-3B / ST25TB | NFC-B technology — hardware impossible |
| MF Ultralight T2T commands | ISO 14443-3A T2T level — below the ISO-DEP layer nrfxlib exposes |
| Aliro reader/door side | Requires NFC polling — no external reader chip |

---

## 12. Out of Scope

- GUI / display output (no screen on this target)
- NFC file save/load (no filesystem)
- BLE-triggered UID rotation (separate concern, can call `nfc_stack_set_uid()`)
- DeSFire write operations (read + auth first; write is a follow-on)
- Aliro step-up phase AccessDocument (skeleton stub only in first implementation)
