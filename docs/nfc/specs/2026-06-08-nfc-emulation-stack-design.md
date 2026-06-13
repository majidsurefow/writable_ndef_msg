# NFC Emulation Stack Design (v2)

> **⚠ SUBORDINATED — This document is no longer the architecture of record.**
>
> The authoritative architecture entry point is:
> **[`docs/superpowers/specs/2026-06-12-nfc-stack-architecture.md`](./2026-06-12-nfc-stack-architecture.md)
> (v3, 2026-06-12)**
>
> This v2 document is **retained as the card-mode / NFCT first-slice implementation
> detail**: APDU framing, AID table, service interfaces, NFCT driver specifics,
> threading model, and Kconfig layout for the card-only slice remain valid and are
> referenced by the wave plans. For the overall architecture (dual-role, multi-backend,
> lane model, capability descriptor, file format), consult v3.

**Date:** 2026-06-08 (v1) · 2026-06-12 (v2)  
**Project:** writable_ndef_msg (nRF52840, Nordic NCS v3.2.4)  
**Status:** v2 — aligned with the LOCKED Wave 1–4 plans and user scope decisions. Where this spec and a locked wave plan differ on implementation detail, the wave plan governs. See Changelog (§14).

---

## 1. Scope

The nRF52840 NFCT peripheral is a **card emulator only** (NFC-A, ISO-DEP). It cannot poll or read cards. This stack implements the **credential/card side** — the nRF presents itself as a smart card that NFC readers tap.

The stack replaces the existing `src/nfc_emulation.c/h` and provides a layered architecture for multi-protocol ISO-DEP card emulation with UID rotation.

---

## 1a. Conventions Compliance

**All layers and services in this stack follow `docs/NFC_STACK_CONVENTIONS.md`** —
the binding adaptation of the firmware-wide design docs (API_DESIGN_CREED,
CALLBACK_REGISTRATION_GUIDE, STATS_API_DESIGN, NETWORK_BUFFERS, STACK_SPEC) to
this stack. Where this spec and the conventions doc overlap, the conventions doc
governs the *how* (lifecycle naming, struct/getter shape, callback pattern,
buffer model, stats recipe, threading annotations); this spec governs the *what*
(protocols, AIDs, layer responsibilities).

Key conventions that shape the API below:
- Lifecycle is `init` / `start` / `stop` / `shutdown` — **no `deinit`**.
- Every layer exposes `_config_t` / `_stats_t` / `_state_t` + `get_config` /
  `get_stats` / `get_state`.
- Callback context pointers are named `user_ctx`; callbacks registered after
  `init()`, before `start()`, wired in `nfc_stack.c`.
- Inbound fragment→APDU transport uses a FIXED `net_buf` pool; outbound responses
  use static service buffers.
- Every public function carries `@threadsafe` / `@isr_safe` / `@caller_sync`.

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
│  nfc_stack_init(cfg) / nfc_stack_start(uid) / nfc_stack_stop() │
│  nfc_stack_set_uid(uid) · nfc_stack_register_event_cb(cb, ctx) │
├─────────────────────────────────────────────────────────────────┤
│  src/nfc/nfc_stack.c/.h          ← public wiring layer         │
├────────────────┬────────────────────────────────────────────────┤
│  services/     │  ndef/   desfire/   ultralight/   emv/  aliro/ │
│                │  each exposes serialize() / deserialize()       │
├────────────────┴────────────────────────────────────────────────┤
│  store/    nfc_store.c/.h                                        │
│  load cb (PRIMARY: compiled-in .h / shell) → deserialize()      │
│  save cb (debug capture: shell hex dump) ← serialize()          │
├─────────────────────────────────────────────────────────────────┤
│  router/           aid_router.c/.h   service.h                  │
│  SELECT AID → registered service lookup → dispatch APDUs        │
├─────────────────────────────────────────────────────────────────┤
│  framing/          apdu_assembler.c/.h   apdu_types.h           │
│  complete APDU (net_buf) → parse → router (parsed nfc_apdu_t);  │
│  unref after dispatch                                            │
├─────────────────────────────────────────────────────────────────┤
│  hal/   nfc_transport.h (vendor-clean) + backend .c             │
│  T4T lifecycle · UID rotation · backend assembles APDU→net_buf  │
├─────────────────────────────────────────────────────────────────┤
│  backend: nrfxlib nfc_t4t_lib (NFC_HAL_BACKEND_NRFX, current)   │
│           PN7160/NCI (NFC_HAL_BACKEND_PN7160, future — §13)     │
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
    │   ├── nfc_transport.h           (vendor-clean public API — §6.1)
    │   ├── nfc_transport_nrfx.c      (nRFX backend — current)
    │   └── nfc_transport_pn7160.c    (PN7160 backend — future mini-wave, §13)
    ├── framing/
    │   ├── apdu_assembler.c
    │   ├── apdu_assembler.h
    │   └── apdu_types.h
    ├── router/
    │   ├── aid_router.c
    │   ├── aid_router.h
    │   └── service.h
    ├── store/
    │   ├── nfc_store.c
    │   ├── nfc_store.h
    │   ├── nfc_store_shell_cmds.c
    │   └── nfc_store_default_cards.h   (compiled-in card blobs — load stub source)
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

**Responsibility:** Own the NFC transport backend lifecycle, UID rotation, and deliver field events and complete C-APDUs upward. Nothing else.

**Backend architecture (vendor-clean HAL):** the public header `hal/nfc_transport.h`
is **vendor-clean** — no nrfxlib (or other vendor) types, includes, or constants
appear in it. The response length limit is a **transport-owned `#define`**
(e.g. `NFC_TRANSPORT_MAX_RESPONSE_LEN`), not a vendor constant. The
implementation is a backend file selected by a Kconfig backend choice (§9):

| Backend | File | Kconfig |
|---|---|---|
| nRF NFCT via nrfxlib `nfc_t4t_lib` (current) | `hal/nfc_transport_nrfx.c` | `NFC_HAL_BACKEND_NRFX` |
| NXP PN7160 (future mini-wave — §13) | `hal/nfc_transport_pn7160.c` | `NFC_HAL_BACKEND_PN7160` |

**Transport contract guarantees (backend-independent):**
- The transport delivers **complete C-APDUs** upward via `on_apdu` — fragment assembly is internal to the backend.
- Inbound fragments are **copied immediately** in the backend callback context; vendor buffer lifetimes are never relied upon.
- The buffer passed to `nfc_transport_send_response()` is **borrowed until the next transport event**; the caller keeps it valid and unmodified until then.

**Lifecycle states:**
```
UNINIT ──nfc_transport_init()──► IDLE
IDLE   ──nfc_transport_start()─► RUNNING
RUNNING──nfc_transport_stop()──► IDLE
IDLE   ──nfc_transport_shutdown()► UNINIT
```

**Public API:** (full lifecycle; state storage Pattern B / `atomic_t` — backend callback reads state)
```c
int  nfc_transport_init(const nfc_transport_config_t *cfg);            /* @caller_sync */
int  nfc_transport_register_callbacks(const nfc_transport_ops_t *ops,
                                      void *user_ctx);                 /* @caller_sync — after init, before start */
int  nfc_transport_start(const nfc_uid_t *uid);                       /* @caller_sync */
int  nfc_transport_stop(void);                                        /* @caller_sync */
int  nfc_transport_shutdown(void);                                    /* @caller_sync — replaces deinit */
int  nfc_transport_set_uid(const nfc_uid_t *uid);                     /* @threadsafe — emulation stop → set UID → restart */
int  nfc_transport_send_response(const uint8_t *buf, size_t len);     /* @threadsafe — borrows buf until next transport event */
const nfc_transport_config_t *nfc_transport_get_config(void);         /* @isr_safe — never NULL */
int  nfc_transport_get_stats(nfc_transport_stats_t *out);             /* @threadsafe — copy-out */
nfc_transport_state_t nfc_transport_get_state(void);                  /* @isr_safe */
```

**Stop vs. UID rotation:** `nfc_transport_stop()` **serializes against
`nfc_transport_set_uid()` via the HAL's UID mutex** — stop acquires the mutex
before stopping emulation. This closes the stop-vs-rotation race: a concurrent
`set_uid` either completes fully before the stop proceeds, or observes the
stopped state and fails cleanly with `-ENODEV`.

**Callbacks delivered upward (Pattern A ops struct, dispatch thread = `nfc_work_q`):**
```c
/* Function pointers ONLY — NO user_ctx member (Wave 1 locked form).
 * user_ctx is a SEPARATE argument to nfc_transport_register_callbacks(ops,
 * user_ctx), stored alongside and passed back to every handler. */
typedef struct {
    void (*on_field_on)(void *user_ctx);
    void (*on_field_off)(void *user_ctx);
    void (*on_apdu)(struct net_buf *apdu, void *user_ctx); /* complete APDU; CALLEE owns the ref */
} nfc_transport_ops_t;
```

**APDU assembly:** The backend callback allocates a FIXED-pool `net_buf` on the
first fragment, appends each fragment with `net_buf_add_mem` (immediate copy),
and on the final fragment enqueues the complete-APDU `net_buf` to `nfc_work_q`.
`on_apdu` then fires on the work thread with the assembled buffer; the callee
(framing) owns the ref and unrefs after dispatch. See
`docs/NFC_STACK_CONVENTIONS.md` §5.

**nRFX backend implementation notes** (`nfc_transport_nrfx.c` only — these
vendor specifics never appear in the public header):
- Raw PICC mode = `nfc_t4t_setup()` + `nfc_t4t_emulation_start()` with no payload set call. Header: `/opt/nordic/ncs/v3.2.4/nrfxlib/nfc/include/nfc_t4t_lib.h`
- The t4t callback fires from ISR context — treat as ISR: no sleep/blocking, FIXED-pool `K_NO_WAIT` allocation only.
- `DATA_IND` delivers PCB-stripped APDU fragments. `data` pointer must be copied immediately — lifetime undocumented.
- `NFC_T4T_DI_FLAG_MORE` set = more fragments follow. Clear = final fragment.
- `nfc_t4t_response_pdu_send(buf, len)` borrows `buf` until the next callback — the backend source of the generic borrow guarantee above.
- UID rotation: `nfc_t4t_emulation_stop()` → `nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, ...)` → `nfc_t4t_emulation_start()`.
- FWI set to 8 (max, ~5 s window) before first `emulation_start()` — backs the transport's response-time headroom.

**Stats struct:** exact stats fields are locked by the corresponding wave plan
(`wave1-hal.md` §1.3, `nfc_transport_stats_t`). Every drop path has a named
counter per CONVENTIONS §6; `error_count`/`last_error_code` are mandatory.

---

### 6.2 Framing Layer (`framing/`)

**Responsibility:** Receive the complete-APDU `net_buf` from the HAL `on_apdu`
callback, validate and parse the ISO 7816-4 structure, dispatch to the router,
and manage the `net_buf` ownership (unref after dispatch returns). Minimal
lifecycle (init/shutdown), Pattern A state (WQ thread only).

**What this layer does NOT do:** byte accumulation (the HAL backend does this),
PCB parsing, WTX, R-block handling, CRC — the transport backend handles all
ISO-DEP framing internally (§6.1 transport contract).

**APDU dispatch contract:**
- On `on_apdu(apdu_buf)`: the buffer already holds one complete C-APDU
  (assembled by the HAL backend). Parse CLA/INS/P1/P2/Lc/data/Le into
  `nfc_apdu_t`. Oversized APDUs cannot occur here (the backend caps at pool
  buffer size and responds `6700` / drops on overflow before enqueue).
- Forward the validated parse view to the router: `aid_router_dispatch(&apdu)`
  — the **parsed form** (`const nfc_apdu_t *`) is the locked signature (Wave 3
  DECISION-7). The router does **not** re-parse; framing's `nfc_apdu_t` is
  passed through unchanged to the service vtable.
- `net_buf_unref()` the buffer after dispatch returns (one owner — see
  CONVENTIONS §5).
- On `on_field_off`: notify the router; no assembly state to reset.

**Thread model:** Assembly + work submission happen in the HAL backend callback
(ISR context on the nRFX backend). Parsing, routing, and service logic run on
the dedicated high-priority `nfc_work_q`.
Response buffer lives in static service memory — valid from `send_response`
until the next callback.

**APDU types (`apdu_types.h`):**
```c
/* Inbound APDU bound = FIXED net_buf pool buffer size (CONVENTIONS §5),
 * not the 32767 extended-APDU ceiling. Oversize → 6700 + drop in the ISR. */
#define NFC_APDU_BUF_SIZE       CONFIG_NFC_APDU_BUF_SIZE   /* default 512 */

/* The full ISO 7816 status-word / CLA / INS / SELECT-P1 constant vocabulary is
 * locked by wave2-framing.md §1.1 — including NFC_SW_COMMAND_NOT_ALLOWED
 * (0x6986), used by the router's no-service-selected response. */

/* Parsed view over a complete C-APDU held in a net_buf. Points into the
 * net_buf's data — valid only during the dispatch call chain (until framing
 * unrefs the buffer). Services copy any data they need to keep. */
typedef struct {
    uint8_t        cla;
    uint8_t        ins;
    uint8_t        p1;
    uint8_t        p2;
    const uint8_t *data;     /* command data field; NULL when lc == 0 */
    uint16_t       lc;       /* command data length */
    uint16_t       le;       /* expected response length */
    bool           has_le;
    bool           extended; /* Lc/Le used extended (3-byte) encoding */
} nfc_apdu_t;
```

Extended-format parsing is gated by `CONFIG_NFC_APDU_EXTENDED_SUPPORT` (§9);
when disabled, extended APDUs are rejected with `6700`.

**Stats struct:** exact stats struct name and fields are locked by
`wave2-framing.md` §1.3 (`apdu_assembler_stats_t`).

---

### 6.3 Router Layer (`router/`)

**Responsibility:** Maintain an AID registration table. On SELECT AID, look up the matching service and call `on_select`. Route all subsequent APDUs to the active service until DESELECT or field-off.

**Service vtable (`service.h`):** (Pattern A; context pointer named `user_ctx`)
```c
typedef void (*nfc_service_on_select_fn)(const uint8_t *aid, size_t aid_len,
                                         void *user_ctx);
typedef void (*nfc_service_on_apdu_fn)(const nfc_apdu_t *apdu, void *user_ctx);
typedef void (*nfc_service_on_deselect_fn)(void *user_ctx);
typedef void (*nfc_service_on_field_off_fn)(void *user_ctx);

/* Persistence hooks — optional. NULL = service not persistable.
 * serialize: flatten the service's data model into out[0..max); set *out_len.
 * deserialize: restore the data model from in[0..len). Both return 0/-errno. */
typedef int (*nfc_service_serialize_fn)(uint8_t *out, size_t max,
                                        size_t *out_len, void *user_ctx);
typedef int (*nfc_service_deserialize_fn)(const uint8_t *in, size_t len,
                                          void *user_ctx);

typedef struct {
    nfc_service_on_select_fn    on_select;
    nfc_service_on_apdu_fn      on_apdu;     /* calls nfc_transport_send_response() internally */
    nfc_service_on_deselect_fn  on_deselect;
    nfc_service_on_field_off_fn on_field_off;
    nfc_service_serialize_fn    serialize;   /* NULLABLE — NULL = service not persistable */
    nfc_service_deserialize_fn  deserialize; /* NULLABLE — NULL = service not persistable */
    uint8_t                     persist_id;  /* stable id for TLV framing in a saved blob; 0 = not persistable */
    void *user_ctx;
} nfc_service_t;
```

The vtable is the CONVENTIONS §4 base form (four callbacks + `user_ctx`)
extended with the persistence fields per Wave 3 DECISION-A. **Persist IDs are
stable — do not renumber** (`NFC_PERSIST_ID_*` in `service.h`, wave3 §1.1):
NDEF `0x01`, DeSFire `0x02`, Ultralight `0x03`, EMV `0x04`, Aliro `0x05`.

**Service send pattern (Flipper-aligned):** Each service calls `nfc_transport_send_response(buf, len)` imperatively inside `on_apdu`. The service owns a static response buffer. This correctly handles both synchronous (NDEF, Ultralight) and deferred (Aliro — crypto on work queue) response paths.

**Router API:**
```c
int  aid_router_init(void);
int  aid_router_shutdown(void);   /* minimal lifecycle per CONVENTIONS §2 */
int  aid_router_register(const uint8_t *aid, size_t aid_len,
                          const nfc_service_t *svc);
void aid_router_clear(void);      /* deregisters all services — used by nfc_stack
                                   * profile switching at field-off (§7) */
void aid_router_dispatch(const nfc_apdu_t *apdu);  /* PARSED form — LOCKED
                                   * (Wave 3 DECISION-7). The router receives
                                   * framing's validated parse view and never
                                   * re-parses. */
void aid_router_field_off(void);
```

**Dispatch logic:**
- Every APDU is checked for SELECT-by-AID (`CLA=00 INS=A4 P1=04`, `lc > 0`). AID = `apdu->data[0..lc-1]`. Linear scan of the registration table, exact-length match.
- SELECT match: call `on_deselect` on the previously active service (if any), mark the matched service active, call `svc->on_select(aid, aid_len, user_ctx)`. **The matched service's `on_select` sends its own SELECT response** (FCI blob for EMV/Aliro, plain `9000` for NDEF/DeSFire) — **the router sends nothing on a match**.
- SELECT with no matching AID (or empty AID data field): **the router sends `6A82`** (File Not Found).
- Non-SELECT APDU with an active service: forward to the active service's `on_apdu`.
- Non-SELECT APDU with **no service selected**: **the router sends `6986`** (Command Not Allowed).
- `FIELD_OFF`: call active service `on_field_off`, clear the active service. The registration table is not cleared.

**Registration:** One AID per table entry. Multiple entries may point to the same `nfc_service_t` (used by Aliro — two AIDs, one service). Table size defined by `CONFIG_NFC_ROUTER_MAX_AIDS` (default 8).

**Stats struct:** exact stats struct name and fields are locked by
`wave3-router.md` §1.3 (`aid_router_stats_t`).

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

### 6.5 Store Layer (`store/`)

**Responsibility:** Load/save the emulated card's data as a flat blob through
*registered callbacks* (CALLBACK_REGISTRATION_GUIDE canonical pattern). This is
a **stub seam**, not a persistence backend.

**Load is the PRIMARY operation:** it provisions card images into the services
from a compiled-in defaults header (`nfc_store_default_cards.h`) or a shell
backend. This device has **no NFC reader mode**, so card content can never be
captured from physical cards — provisioning data always originates off-device.

**Save is DEMOTED to a debug/capture facility:** it exists to dump
reader-written state (e.g. NDEF content a phone wrote to us) for inspection or
re-provisioning. It is not a persistence path.

The default stubs require no filesystem — load reads a compiled-in `.h`, save
dumps to the shell. A real NVS/LittleFS backend is added later by registering a
different callback; nothing else changes.

**What it does:** For load, it fetches the blob from the registered **load**
callback and scatters each TLV entry to the matching service's `deserialize()`.
For save, it calls each service's `serialize()`, frames the blobs into one TLV
buffer keyed by `persist_id`, and hands the buffer to the registered **save**
callback. `serialize`/`deserialize` are **nullable per service** — services
with NULL hooks are silently skipped.

**Lifecycle gating:** both `nfc_stack_save()` and `nfc_stack_load()` return
**`-EBUSY` while the stack is STARTED** — save/load only run against a quiesced
stack (before `start()` or after `stop()`).

**Minimal lifecycle (init/shutdown), Pattern A state (caller/WQ thread).**

**API (`store/nfc_store.h`):**
```c
/* save sink: where the packed blob goes. Default stub = shell hex dump. */
typedef int (*nfc_store_save_fn)(const char *tag,
                                 const uint8_t *blob, size_t len, void *user_ctx);
/* load source: where the blob comes from. Default stub = compiled-in .h. */
typedef int (*nfc_store_load_fn)(const char *tag,
                                 uint8_t *out, size_t max, size_t *out_len,
                                 void *user_ctx);

int nfc_store_init(const nfc_store_config_t *cfg);                 /* @caller_sync */
int nfc_store_shutdown(void);                                      /* @caller_sync */
int nfc_store_register_save_cb(nfc_store_save_fn fn, void *user_ctx); /* @caller_sync; NULL = restore default stub */
int nfc_store_register_load_cb(nfc_store_load_fn fn, void *user_ctx); /* @caller_sync; NULL = restore default stub */

/* Pack svcs[].serialize() into one TLV blob → save cb. */
int nfc_store_save(const char *tag, const nfc_service_t *const *svcs, size_t n); /* @caller_sync */
/* Fetch via load cb → scatter TLV entries to matching svcs[].deserialize(). */
int nfc_store_load(const char *tag, const nfc_service_t *const *svcs, size_t n); /* @caller_sync */

const nfc_store_config_t *nfc_store_get_config(void);             /* never NULL */
int  nfc_store_get_stats(nfc_store_stats_t *out);                 /* copy-out */
nfc_store_state_t nfc_store_get_state(void);
```

**Default stubs:**
- **Load stub** (`nfc_store_load_stub`, PRIMARY): looks up `tag` in the
  compiled-in table in `nfc_store_default_cards.h` and copies the bytes out.
- **Save stub** (`nfc_store_save_stub`, debug capture): emits the blob to the
  shell as a pastable C array under a sentinel tag (e.g. `@@NFCDUMP@@ <tag>
  <hex>`), so reader-written state can be copied straight into
  `nfc_store_default_cards.h` for future provisioning.

**Shell (`nfc_store_shell_cmds.c`):** `nfc save <tag>` → `nfc_stack_save(tag)`;
`nfc load <tag>` → `nfc_stack_load(tag)`; plus `config`/`stats`/`state`.

**TLV blob format:** `[persist_id:u8][len:u16][bytes...]` repeated, so unknown
ids are skipped on load and services can be added without breaking old blobs.

---

## 7. Public Stack API (`nfc_stack.h`)

**Orchestration (Wave 4 locked):** `nfc_stack` registers its **own wrapper ops**
(`s_transport_ops`) with the HAL — not `apdu_assembler_get_ops()` directly.
Each wrapper handler chains to the corresponding `apdu_assembler_get_ops()`
handler **at runtime**, then adds stack-level logic: on field-off, the framing
chain runs first (router/service session cleanup), then the pending profile is
applied, then the application event callback fires. All cross-layer wiring
lives in `nfc_stack.c` (CONVENTIONS §3).

```c
/* One-time hardware and layer initialization. Call before nfc_stack_start().
 * Initializes framing, router, store, transport; eagerly inits ALL compiled-in
 * services (so a profile switch never inits a service on the work queue);
 * registers the wrapper ops with the HAL; registers the default profile's AIDs
 * (CONFIG_NFC_STACK_DEFAULT_PROFILE). cfg == NULL → built-in defaults. */
int nfc_stack_init(const nfc_stack_config_t *cfg);

/* Start field sensing with the given initial UID. */
int nfc_stack_start(const nfc_uid_t *uid);

/* Stop field sensing. */
int nfc_stack_stop(void);

/* Rotate UID: nrfxlib-level emulation stop → set NFCID1 → restart. The module
 * stays STARTED; the work queue is not torn down. Safe to call while running
 * (@threadsafe) — including from the event callback below. */
int nfc_stack_set_uid(const nfc_uid_t *uid);

/* Tear down. Legal from any state; performs implicit stop() if started. */
int nfc_stack_shutdown(void);

/* ── Application event callback ───────────────────────────────────────────────
 * Registered AFTER nfc_stack_init(), BEFORE nfc_stack_start(). Dispatch
 * thread: nfc_work_q — the callback must not block, sleep, or allocate.
 *
 * NFC_STACK_EVENT_FIELD_OFF fires AFTER (a) field-off session cleanup
 * (router/service on_field_off, active service cleared) AND (b) application of
 * any pending profile — the app observes the post-cleanup, post-switch state.
 *
 * Calling nfc_stack_set_uid() from the callback on FIELD_OFF is the CANONICAL
 * per-field-off UID-rotation pattern (deadlock-free; wave4 §6).
 * Calling nfc_stack_stop()/nfc_stack_shutdown() from the callback is FORBIDDEN
 * (they wait on the work queue the callback occupies — self-deadlock). */
typedef enum {
    NFC_STACK_EVENT_FIELD_ON,
    NFC_STACK_EVENT_FIELD_OFF,
} nfc_stack_event_t;

typedef void (*nfc_stack_event_cb_fn)(nfc_stack_event_t event, void *user_ctx);

int nfc_stack_register_event_cb(nfc_stack_event_cb_fn cb, void *user_ctx);

/* Profile enum — which services are registered in the router.
 * Only profiles whose Kconfig is enabled are valid at runtime.
 * ALL registers every compiled-in service. (Sentinel per wave4 §1.1.) */
typedef enum {
    NFC_PROFILE_NDEF,
    NFC_PROFILE_DESFIRE,
    NFC_PROFILE_ULTRALIGHT,
    NFC_PROFILE_EMV,
    NFC_PROFILE_ALIRO,
    NFC_PROFILE_ALL,
    NFC_PROFILE_COUNT_,   /* sentinel — do not use */
} nfc_profile_t;

/* Switch active profile. Sets an ATOMIC pending value and returns immediately
 * (@threadsafe). The switch is applied on the NEXT field-off, on nfc_work_q:
 * aid_router_clear() then re-registration of the new profile's AIDs.
 * Returns -ENOTSUP for a profile whose service is not compiled in.
 * If re-registration of the new profile fails, the OLD profile is re-registered
 * as a fallback — the router table is never left empty while STARTED.
 * In-memory only — does not persist across reboots. */
int nfc_stack_set_profile(nfc_profile_t profile);

/* Load / save the active profile's card data via the store layer (§6.5 —
 * load is the PRIMARY operation; save is a debug/capture facility).
 * load: nfc_store_load() → load cb → scatter to active services' deserialize().
 * save: gather active services' serialize() → nfc_store_save() → save cb.
 * Both return -EBUSY while the stack is STARTED. */
int nfc_stack_load(const char *tag);   /* @caller_sync; -EBUSY while STARTED */
int nfc_stack_save(const char *tag);   /* @caller_sync; -EBUSY while STARTED */
```

`main.c` usage:
```c
/* Canonical per-field-off UID rotation — runs on nfc_work_q; must not block. */
static void app_nfc_event_cb(nfc_stack_event_t event, void *user_ctx)
{
    if (event == NFC_STACK_EVENT_FIELD_OFF) {
        /* pending profile (if any) is already applied at this point */
        (void)nfc_stack_set_uid(&next_uid);
    }
}

nfc_stack_init(NULL);
nfc_stack_register_event_cb(app_nfc_event_cb, NULL);  /* after init, before start */
nfc_stack_load("my_card");      /* provision before start (-EBUSY while STARTED) */
nfc_stack_start(&initial_uid);
/* ... main loop ... */
/* on button press: nfc_stack_set_profile(NFC_PROFILE_ALIRO) — applied at next field-off */
```

---

## 8. Thread Model

(nRFX backend shown — ISR context, `DATA_IND` events. The §6.1 transport
contract guarantees are backend-independent. On the WQ, all upward events flow
through nfc_stack's wrapper ops, which chain to framing — §7; on field-off the
wrapper additionally applies any pending profile, then fires the application
event callback.)

```
ISR (NFCT interrupt)
  │
  ├── FIELD_ON  → submit k_work to nfc_work_q → router/services notified
  ├── FIELD_OFF → submit k_work to nfc_work_q → router/services reset
  │               → pending profile applied → app event cb (FIELD_OFF)
  └── DATA_IND  → alloc/append FIXED net_buf (immediate copy)
                  on final fragment → k_fifo_put → wake nfc_work_q
                                                               │
                                              nfc_work_q thread (high priority)
                                                               │
                                              framing: parse APDU (net_buf), unref after
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

**Inbound buffer:** FIXED `net_buf` pool (CONVENTIONS §5). The ISR allocates on
the first fragment and appends each fragment immediately (nrfxlib buffer lifetime
undocumented), enqueueing the complete APDU on the final fragment. One owner at a
time; the WQ unrefs after dispatch. Pool exhaustion → drop + stat, never assert.

---

## 9. Kconfig

**Sourcing mechanism:** the application's top-level `Kconfig` `rsource`s
`src/nfc/Kconfig`, which in turn `rsource`s the per-layer Kconfigs
(`hal/Kconfig`, `framing/Kconfig`, `router/Kconfig`, `services/*/Kconfig`,
`store/Kconfig`). The top-level `CMakeLists.txt` adds `src/nfc` via
`add_subdirectory(src/nfc)`; `src/nfc/CMakeLists.txt` adds each layer
subdirectory (wave4 §5.5).

**Symbol map** (full definitions — types, ranges, help text, `select`s — are
locked by `wave4-stack.md` §1.6 and the per-layer wave plans):

| Symbol | Default | Defined in | Introduced by |
|---|---|---|---|
| `NFC_STACK` (parent) | — | `src/nfc/Kconfig` | spec |
| `NFC_APDU_BUF_SIZE` | 512 | `src/nfc/Kconfig` | spec (stack-level per wave4 DECISION-5) |
| `NFC_APDU_POOL_COUNT` | 4 | `src/nfc/Kconfig` | spec (stack-level per wave4 DECISION-5) |
| `NFC_STACK_DEFAULT_PROFILE` | 0 (NDEF) | `src/nfc/Kconfig` | wave 4 |
| `NFC_SERVICE_NDEF / _DESFIRE / _ULTRALIGHT / _EMV / _ALIRO` | — | `src/nfc/Kconfig` | spec (PSA `select`s per wave4 §1.6) |
| `NFC_STORE` | y | `src/nfc/Kconfig` | spec |
| `NFC_NDEF_MAX_SIZE` | 256 | `src/nfc/Kconfig` | spec |
| `NFC_HAL_BACKEND_NRFX` / `NFC_HAL_BACKEND_PN7160` (choice) | NRFX | `src/nfc/hal/Kconfig` | HAL portability decision (§6.1) |
| `NFC_HAL_WORKQ_STACK_SIZE` | 2048 | `src/nfc/hal/Kconfig` | wave 1 |
| `NFC_HAL_WORKQ_PRIORITY` | 5 | `src/nfc/hal/Kconfig` | wave 1 |
| `NFC_APDU_EXTENDED_SUPPORT` | y | `src/nfc/framing/Kconfig` | wave 2 |
| `NFC_ROUTER_MAX_AIDS` | 8 | `src/nfc/router/Kconfig` | wave 3 |

**HAL backend choice** (`src/nfc/hal/Kconfig` — see §6.1):

```kconfig
choice NFC_HAL_BACKEND
    prompt "NFC HAL transport backend"
    default NFC_HAL_BACKEND_NRFX
    depends on NFC_STACK

config NFC_HAL_BACKEND_NRFX
    bool "nRF NFCT via nrfxlib nfc_t4t_lib"
    depends on NFC_T4T_NRFXLIB

config NFC_HAL_BACKEND_PN7160
    bool "NXP PN7160 (future — see §13)"

endchoice
```

The `NFC_T4T_NRFXLIB` dependency lives on the NRFX backend symbol, not on
`NFC_STACK` — the stack itself is backend-neutral (§6.1).

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
- Real persistence backend for save/load (NVS / LittleFS / Flipper `.nfc` files) — the store layer ships a stub seam (shell dump + compiled-in `.h`); a real backend is a later callback swap
- BLE-triggered UID rotation (separate concern, can call `nfc_stack_set_uid()`)
- DeSFire write operations (read + auth first; write is a follow-on)
- Aliro step-up phase AccessDocument (skeleton stub only in first implementation)

---

## 13. Non-goals and Future Extensions

- **Reader/initiator mode is an explicit non-goal of this library.** The stack
  is card-emulation only, end to end.
- **Role is determined by which transport API exists, NOT by backend choice.**
  Selecting `NFC_HAL_BACKEND_PN7160` does not add reader capability — it swaps
  the emulation backend behind the same `nfc_transport_*` (card-side) API.
- A future reader role would arrive as a **sibling `nfc_reader_*` transport**
  plus its own client-protocol stack, sharing only the `apdu_types.h`
  vocabulary with this library.
- The **PN7160 emulation backend is a future mini-wave**, to be planned against
  the PN7160 datasheet + NCI specification (`hal/nfc_transport_pn7160.c`,
  §6.1).

---

## 14. Changelog

**v2 — 2026-06-12** — alignment with the LOCKED Wave 1–4 plans and user scope
decisions:

1. §6.2/§6.3 — dispatch is the parsed form `aid_router_dispatch(const nfc_apdu_t *)`; framing forwards its validated parse view; the router never re-parses; raw-bytes form removed everywhere. (wave3 DECISION-7)
2. §6.1 — `nfc_transport_ops_t` carries function pointers only, no `user_ctx` member; `user_ctx` is a separate argument to `nfc_transport_register_callbacks(ops, user_ctx)`; `on_apdu(struct net_buf *, user_ctx)` with callee owning the ref. (wave1 DECISION-2)
3. §6.3 — service vtable confirmed as four callbacks + `user_ctx` + nullable `serialize`/`deserialize` + `persist_id` (stable IDs per wave3 §1.1); SELECT response ownership: the matched service's `on_select` sends its own response, the router sends nothing on a match, 6A82 for unmatched SELECT, 6986 for non-SELECT with no service selected. (wave3 DECISION-A/B/C)
4. §7 — nfc_stack registers its own wrapper ops with the HAL, chaining to `apdu_assembler_get_ops()` at runtime; application event callback added (`nfc_stack_event_t`, `nfc_stack_register_event_cb`), fired on `nfc_work_q` after field-off cleanup and pending-profile application; `set_uid` from the callback is the canonical per-field-off rotation pattern; stop/shutdown from the callback forbidden; profile switching: atomic pending value applied at next field-off via `aid_router_clear()` + re-register, `-ENOTSUP` for non-compiled profiles, eager init of all compiled-in services, old-profile fallback on re-registration failure. (wave4 DECISION-1/3/4/8; user decision: app event callback)
5. §6.5 — load is PRIMARY (provisioning from compiled-in defaults or shell; this device has no reader mode, so card content can never be captured from physical cards); save DEMOTED to a debug/capture facility for reader-written state; `nfc_stack_save/load` return `-EBUSY` while STARTED; serialize/deserialize hooks nullable per service. (user scope decisions: save demotion, -EBUSY rule)
6. §6.1 — vendor-clean `nfc_transport.h` (no nrfxlib types/includes/constants; transport-owned response length `#define`); backend files `nfc_transport_nrfx.c` (current) / `nfc_transport_pn7160.c` (future) behind the `NFC_HAL_BACKEND_*` Kconfig choice; transport contract guarantees stated generically; nrfxlib specifics moved to nRFX backend implementation notes. (user scope decision: HAL portability)
7. §13 added — non-goals and future extensions: reader mode is a non-goal; role = transport API, not backend choice; future `nfc_reader_*` sibling stack; PN7160 backend as a future mini-wave. (user scope decision: reader non-goal)
8. §6.1/§6.2/§6.3 — exact stats field lists replaced with deferrals to the locked wave plans (`nfc_transport_stats_t` per wave1 §1.3, `apdu_assembler_stats_t` per wave2 §1.3, `aid_router_stats_t` per wave3 §1.3). (wave1 DECISION-1, wave2 DECISION-3, wave3 DECISION-E)
9. §6.1 — `nfc_transport_stop()` serializes against `nfc_transport_set_uid()` via the HAL's UID mutex; stop acquires the mutex before stopping emulation. (cross-review finding: stop-vs-rotation race)
10. §9 — symbol table extended with `NFC_APDU_EXTENDED_SUPPORT` (wave 2), `NFC_STACK_DEFAULT_PROFILE` (wave 4), `NFC_ROUTER_MAX_AIDS` (wave 3), `NFC_HAL_WORKQ_STACK_SIZE/PRIORITY` (wave 1), and the HAL backend choice; sourcing mechanism stated explicitly (top-level Kconfig rsources `src/nfc/Kconfig` → per-layer Kconfigs; top-level CMake adds `src/nfc` via `add_subdirectory`). (waves 1–4; HAL portability)
11. Header/title block marked v2; this changelog added. (process)
