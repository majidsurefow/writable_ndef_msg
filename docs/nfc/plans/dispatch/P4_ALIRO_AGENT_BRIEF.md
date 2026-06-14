# P4 — Aliro AUTH Chain + Stronger Loopback Compare — Agent Dispatch Brief

**Priority:** #4 (Aliro half)  
**Goal:** Extend poller beyond SELECT, implement AUTH1 listener path, multi-step fixtures, deep loopback compare.  
**Flipper has ZERO Aliro support — all goldens from wave5 + synthetic fixtures + optional NCS audit.**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper Aliro?** | **NO** — `grep -ri aliro` under `flipperzero/` = 0 matches; no protocol module, no `.nfc` |
| **NCS reference?** | **YES (local, gitignored)** — `/Users/majidfaroud/writable_ndef_msg/aliro/` — behavioral ref only, no GPL paste |
| **Authoritative spec?** | `docs/nfc/archive/waves/wave5-aliro.md` + Aliro 0.9.4 (gated by `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED`) |
| **Loopback today?** | **SELECT-only** — false confidence; AUTH chain untested E2E |
| **Listener Tier C?** | **Half-done** — AUTH0 deferred stub works; AUTH1 falls through to `6D00` |

---

## Exact files and functions to edit

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Extend poller** | `src/nfc/protocols/aliro/aliro_poller.c` | `aliro_poller_read()` L60–91 — add AUTH0 (+ optional AUTH1 observe) |
| **Implement AUTH1** | `src/nfc/protocols/aliro/aliro_listener.c` | `aliro_listener_on_apdu()` L106–108; `aliro_listener_crypto_handler()` L36–55 |
| **Deepen loopback compare** | `tests/unit/nfc_reader/src/test_virtual_loopback.c` | `aliro_compare()` L733–753, `build_aliro_poller_expected()` L756–765, `test_virtual_loopback_aliro()` L767+ |
| **Multi-step fixtures** | `tests/fixtures/aliro/` | Add `AUTH0_req.inc`, `AUTH0_rsp.inc`, `AUTH1_*.inc`; extend `Aliro_mock.h` |
| **Expand Tier C tests** | `tests/unit/nfc_aliro/src/test_listener.c` | Port wave5 §8 named cases |
| **Expand poller tests** | `tests/unit/nfc_aliro/src/test_poller.c` | AUTH0 TX/RX after poller extension |
| **PSA test shims** | `tests/common/` (new mocks) | Deterministic crypto vectors when `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=n` |
| **Spec** | `docs/nfc/archive/waves/wave5-aliro.md` | FSM, APDU catalog, transcript layout |

### Infrastructure already ready (do not rewrite)

- `tests/common/src/nfc_virtual_rf.c` — multi-APDU routing SELECT → `on_select`, others → `on_apdu`
- `tests/common/src/nfc_work_spy.c` — `nfc_transport_submit_work()` runs handler **synchronously**
- `tests/common/src/nfc_virtual_loopback.c` — full poller↔listener harness

---

## Wire format bytes / parity targets

### Expedited AID (SELECT)

```
AID: A0 00 00 08 58 01 01 01 00
SELECT TX: 00 A4 04 00 09 A0 00 00 08 58 01 01 01 00
SELECT R-APDU (listener): 00 00 || nonce(16)   [no 90 00 trailer in expedited path]
```

**Poller today** (`aliro_poller.c` L63–64, L74–88): SELECT only; copies R-APDU into `transcript`.

### AUTH0 (wave5)

```
CLA: 0x80 (ALIRO_CLA_PROP)
INS: 0x80 (ALIRO_INS_AUTH0)
Payload: ~81 B — reader nonce + reader ephemeral P-256 pubkey
Listener response (target): 00 00 || vehicle nonce(16) || 0x04 || credential_pubkey(64) → AWAIT_AUTH1
```

**Listener stub today** (`aliro_listener.c` L47–52): `0xAA` nonce, `0xBB` pubkey placeholders — replace with deterministic test vectors.

### AUTH1 (wave5)

```
INS: 0x81 (ALIRO_INS_AUTH1)
Listener: copy reader sig → crypto work → ECDH/HKDF/verify/sign
Transcript field order: per wave5 DECISION-9..12 (162 B layout)
```

**Listener today** (`aliro_listener.c` L106–108): state check only → falls through to `NFC_SW_INS_NOT_SUPPORTED` L116.

### EXCHANGE (deferred in listener)

```
INS: 0x82 — not implemented; state must be AWAIT_EXCHANGE
```

### Loopback expected transcript (today vs target)

| Phase | Today (`build_aliro_poller_expected`) | Target |
|-------|--------------------------------------|--------|
| SELECT | 18 B: `00 00` + nonce `0x11…` | Same |
| AUTH0 | Not sent by poller | Append AUTH0 R-APDU to transcript |
| AUTH1 | Not sent | Optional observe-only append (READ_ONLY_PARTIAL) |

---

## Current bugs (file:line)

| Bug | Location | Impact |
|-----|----------|--------|
| Poller SELECT-only | `aliro_poller.c` L60–91 | No AUTH0/AUTH1 APDUs |
| Loopback compare transcript-only | `test_virtual_loopback.c` L744–750 | Ignores pubkey, features, protocol fields |
| Expected builder SELECT-only | `test_virtual_loopback.c` L756–765 | 18-byte nonce transcript only |
| AUTH1 unimplemented | `aliro_listener.c` L106–108, L116 | `6D00` on AUTH1 |
| AUTH0 stub crypto | `aliro_listener.c` L49–51 | `0xAA`/`0xBB` placeholders |
| Poller mock single step | `tests/fixtures/aliro/Aliro_mock.h` | SELECT-only script |
| Tier C tests incomplete | `tests/unit/nfc_aliro/src/test_listener.c` | 4 tests vs ~17+ in wave5 §8 |
| Store golden hand-crafted | `tests/fixtures/store/Aliro_card.inc` | Not from Flipper (expected) |

---

## DO NOT touch

- Flipper tree — no Aliro exists; do not add Flipper `.nfc` to `tests/fixtures/nfc/flipper/`
- `flipper_nfc_to_fixture.py` — no Aliro converter path
- GPL paste from `aliro/` NCS tree — read-only behavioral audit
- EMV files — separate P4 EMV brief
- `src/nfc/protocols/ultralight/**` — P1
- Product PSA crypto integration beyond test shims unless explicitly scoped

---

## Staged commits (exact message templates)

### Commit 1 — Multi-step fixtures

```
tests/aliro: add AUTH0/AUTH1 fixture scripts and extend Aliro_mock.h

Synthetic step arrays for virtual RF and poller mocks per wave5-aliro.md.
```

### Commit 2 — Poller AUTH0

```
nfc/aliro: send AUTH0 after expedited SELECT in aliro_poller_read

Append AUTH0 response to transcript; prepare for AUTH1 observe path.
```

### Commit 3 — Listener AUTH1

```
nfc/aliro: implement AUTH1 handler with deferred crypto work

Replace 0xAA/0xBB stub with deterministic test vectors under
CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=n.
```

### Commit 4 — Tier C test expansion

```
tests/aliro: port wave5 listener cases for AUTH0/AUTH1 matrix

Add test_auth1_in_await_auth0_rejects, test_auth0_work_builds_response_83bytes, etc.
```

### Commit 5 — Loopback AUTH chain

```
tests/nfc_reader: extend aliro loopback compare for AUTH chain

Add test_virtual_loopback_aliro_auth_chain; compare transcript + metadata fields.
```

### Commit 6 — Docs

```
docs/nfc: document Aliro golden sources (wave5, not Flipper)

Update aliro/CONTEXT.md with NCS parity deltas (read-only audit notes).
```

---

## Twister verify commands

```bash
REPO=/Users/majidfaroud/writable_ndef_msg

# Aliro unit (model + poller + listener)
west twister -T "$REPO/tests/unit/nfc_aliro" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Loopback suite (shell_off config includes virtual loopback)
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.shell_off -v

# Filter Aliro loopback
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v \
  --testcase-filter "*loopback_aliro*"
```

**Pass criteria:** `sample.nfc.unit.nfc_aliro.{model,poller,listener}` green; new AUTH chain loopback test passes; existing `test_select_expedited` / `test_auth0_submits_work` unchanged or updated intentionally.

---

## Caller sweep checklist

- [ ] `src/nfc/reader/nfc_reader_poller_registry.c` — Aliro poller registration
- [ ] `tests/unit/nfc_aliro/src/test_poller.c` — poller detect/read
- [ ] `tests/unit/nfc_aliro/src/test_model.c` — serialize layout
- [ ] `tests/unit/nfc_reader/src/test_store_aliro_roundtrip.c` (if exists) or store envelope usage
- [ ] `tests/common/src/nfc_virtual_rf.c` — APDU routing after multi-step
- [ ] `tests/common/src/nfc_work_spy.c` — sync work drain for AUTH0/AUTH1
- [ ] `src/nfc/protocols/aliro/aliro.c` — transcript serialize/deserialize bounds
- [ ] Kconfig: `CONFIG_NFC_ALIRO_MAX_TRANSCRIPT`, `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED`

---

## Deferred scope

- EXCHANGE (`0x82`) listener + poller path
- Full PSA production crypto (vs test shim)
- HIL PN7160 capture of real credential transcript
- Aliro 0.9.4 spec sign-off under `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=y`
- Step-up AID path (`aliro_stepup_aid` → `6A81` in `aliro_listener.c` L72–74)
- Flipper fixture parity row (N/A — mark "(proprietary)" permanently)
