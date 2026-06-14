# P5 — DESFire Deeper Compare + Poller/Golden Alignment — Agent Dispatch Brief

**Priority:** #5  
**Goal:** Port Flipper keyless poller SM (PICC→apps→files), expand mocks/compare, optional Flipper `.nfc` capture.  
**Mocks use our APDU+0x91 format — NOT Flipper raw command bytes.**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper DESFire poller?** | **YES** — full SM in `flipperzero/lib/nfc/protocols/mf_desfire/` |
| **Flipper listener?** | **NO** — `NULL` in listener registry (poller-only) |
| **Local DESFire `.nfc`?** | **NO** in `tests/fixtures/nfc/flipper/` — must capture from hardware or use `NfcFileFormats.md` example |
| **Poller vs golden?** | **MISALIGNED** — poller PICC-only; golden model has 1 app + 1 file |
| **Compare depth?** | **SHALLOW** — UID + `free_memory` + `master_key_settings` only |
| **Framing?** | Flipper sends **native cmd bytes** on ISO-DEP blocks; we use **full APDUs** `90 INS …` + trailing `91 xx` — mocks must match **ours** |

---

## Exact files and functions to edit

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Port poller SM** | `src/nfc/protocols/desfire/desfire_poller.c` | Replace `desfire_poller_read()` L170–217 with state machine |
| **Add internal I/O** | `src/nfc/protocols/desfire/desfire_poller_i.c` (new) | Chunked `0x91AF` send, command wrappers as APDUs |
| **Expand listener** | `src/nfc/protocols/desfire/desfire_listener.c` | Add GET_APPLICATION_IDS, GET_FILE_IDS, GET_FILE_SETTINGS |
| **Deepen loopback compare** | `tests/unit/nfc_reader/src/test_virtual_loopback.c` | `desfire_compare()` L507–528 |
| **Fix poller tests** | `tests/unit/nfc_desfire/src/test_poller.c` | `test_read_golden()` L52–66, `test_read_partial_without_keys()` L68–74 |
| **Expand mocks** | `tests/fixtures/desfire/Desfire_mock.h` | `desfire_Desfire_read_steps[]` — add app/file transceives |
| **Capture golden** | `tests/fixtures/nfc/flipper/MfDesfire_EV1_sample.nfc` (new) | From Flipper dump or pm3 example |
| **Generator** | `scripts/nfc/flipper_nfc_to_fixture.py` OR manual pipeline | DESFire path (currently absent) |
| **Fix stale doc** | `src/nfc/protocols/desfire/CONTEXT.md` L15 | Claims poller reads apps/files — **false** |
| **Optional verify applet** | `src/nfc/applets/nfc_applet_verify_compare.c` | Add DESFire dispatch |

### Flipper reference (read-only)

- `flipperzero/lib/nfc/protocols/mf_desfire/mf_desfire_poller.c` — SM L49–202
- `flipperzero/lib/nfc/protocols/mf_desfire/mf_desfire_poller_i.c` — native cmd I/O
- `flipperzero/documentation/file_formats/NfcFileFormats.md` L212–275 — `.nfc` schema

---

## Wire format bytes / parity targets

### Our APDU stack (keep — PN7160 aligned)

| Step | Our TX (APDU) | Response SW |
|------|---------------|-------------|
| ISO SELECT DESFire AID | `00 A4 04 00 07 D2 76 00 00 85 01 00` | `90 00` |
| GET_KEY_VERSION(0) | `90 64 00 00 01 00 00` | payload + `91 00` |
| GET_VERSION | `90 60 00 00 00` | 3× `91 AF` chain |
| GET_FREE_MEMORY | `90 6E 00 00 00` | 4 B LE + `91 00` |
| GET_KEY_SETTINGS | `90 45 00 00 00` | + `91 00` |
| GET_APPLICATION_IDS | `90 6A 00 00 00` | + `91 00` |
| SELECT_APPLICATION | `90 5A 00 00 03 [AID×3]` | + `91 00` |
| GET_FILE_IDS | `90 6F 00 00 00` | + `91 00` |
| GET_FILE_SETTINGS | `90 F5 00 00 01 [file_id]` | + `91 00` |
| READ_DATA | `90 BD 00 00 [Le]` | chunked `91 AF` |

**Mock today** (`Desfire_mock.h`): **7 steps, PICC-only** — no app/file transceives.

### Flipper native commands (reference only — transform to APDU when porting)

| Cmd | Native byte | Flipper function |
|-----|-------------|------------------|
| GET_VERSION | `0x60` | `mf_desfire_poller_read_version` |
| GET_KEY_VERSION | `0x64` + key | `mf_desfire_poller_read_key_version` |
| Continuation | raw `0xAF` | `mf_desfire_send_chunks` |

### Golden hand model (`Desfire_mock.h`)

```
UID: 04 .. 06 (7 B)
free_memory: 0x1D60
master_key_settings: 0x0B
App AID: 01 02 03
File id: 01, access 0x0E0E (free read), payload "HELLO DESFIRE!!" (15 B)
```

### Auth policy (Flipper partial capture)

- Never authenticates; closed directory → PICC metadata only; per-file access ≠ `0x0E` → skip with log

### Store envelope flags (target)

| Capture depth | Flags |
|---------------|-------|
| Poller PICC-only | `NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL` |
| Full keyless | `READER_CAPTURED \| READ_ONLY_PARTIAL` |
| Hand golden | `HAND_AUTHORED \| EMULATION_COMPLETE` |

---

## Current bugs (file:line)

| Bug | Location | Impact |
|-----|----------|--------|
| Poller stops at PICC | `desfire_poller.c` L170–217 | `app_count` always 0 after read |
| Compare ignores apps/files | `test_virtual_loopback.c` L518–526 | Loopback passes with shallow PICC match |
| Poller test expects `app_count==0` | `test_poller.c` L68–74 | Encodes wrong golden expectation |
| Listener has app/file cmds | `desfire_listener.c` | Can serve golden if model loaded — poller never captures |
| Clone path loses app data | `nfc_reader_poller_registry.c` L300–324 area | Poller output `app_count=0` → store clone incomplete |
| Mock 7-step PICC-only | `Desfire_mock.h` | No app/file steps |
| No Flipper `.nfc` fixture | `tests/fixtures/nfc/flipper/README.md` | Parity row blocked |
| Stale CONTEXT claim | `desfire/CONTEXT.md` L15 | Docs say poller reads apps/files |
| verify applet NDEF-only | `nfc_applet_verify_compare.c` | No DESFire compare |

---

## DO NOT touch

- `src/nfc/protocols/ultralight/**` — P1
- DESFire listener crypto / full auth — stub reject path stays for v1
- Copy Flipper raw-byte scripts verbatim into mocks — must APDU-wrap
- Flipper submodule sources (read-only)
- EMV / Aliro poller paths

---

## Staged commits (exact message templates)

### Commit 1 — Flipper golden capture (optional first)

```
tests/fixtures: add MfDesfire_EV1_sample.nfc and flipper README row

Captured from Flipper dump or NfcFileFormats.md example; no poller change yet.
```

### Commit 2 — Poller state machine

```
nfc/desfire: port keyless poller SM for apps and files

Implement desfire_poller_state_t; GET_APPLICATION_IDS, SELECT_APPLICATION,
GET_FILE_IDS/SETTINGS, READ_DATA with 0x0E access gate; APDU 91 AF chunking.
```

### Commit 3 — Listener command parity

```
nfc/desfire: add GET_APPLICATION_IDS and file metadata listener commands

Enable loopback SELECT → READ_DATA golden path end-to-end.
```

### Commit 4 — Fixtures + mocks

```
tests/desfire: extend Desfire_mock.h with app/file transceive steps

Align desfire_Desfire_read_steps[] with golden model app 01 02 03 / file 01.
```

### Commit 5 — Deep compare + tests

```
tests/desfire: assert full model in poller and loopback compare

Replace shallow desfire_compare; fix test_read_partial expectations.
```

### Commit 6 — Docs

```
docs/nfc: fix desfire CONTEXT and harmonization capture note

Poller now reads apps/files when access allows; note reader-captured path.
```

---

## Twister verify commands

```bash
REPO=/Users/majidfaroud/writable_ndef_msg

west twister -T "$REPO/tests/unit/nfc_desfire" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.shell_off -v \
  --testcase-filter "*loopback_desfire*"

west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.store -v
```

**Pass criteria:** `sample.nfc.unit.nfc_desfire.{model,poller,listener}` green; loopback compares app/file payloads; `test_read_golden` asserts `app_count >= 1` when golden has app.

---

## Caller sweep checklist

- [ ] `src/nfc/reader/nfc_reader_poller_registry.c` — clone path `desfire_poller_read` → listener init
- [ ] `tests/unit/nfc_desfire/src/test_listener.c` — SELECT → READ_DATA
- [ ] `tests/unit/nfc_desfire/src/test_model.c` — serialize roundtrip with app/file
- [ ] `tests/unit/nfc_reader/src/test_store_desfire_roundtrip.c` — assert file payloads not just `app_count`
- [ ] `scripts/nfc/protocol_to_card_bin.py --desfire` — regenerate store envelope
- [ ] `src/nfc/protocols/desfire/desfire.c` — model equality / serialize layout
- [ ] Chunk size vs `NFC_TRANSPORT_MAX_RESPONSE_LEN` (255 B) — Flipper uses 512 B buffer

---

## Deferred scope

- Flipper `.nfc` converter in `flipper_nfc_to_fixture.py` (manual pipeline OK for v1)
- Full crypto auth / key diversification
- DESFire listener for all file types (VALUE, RECORD) — read standard/backup only first
- HIL DESFire tag on PN7160
- `nfc_applet_verify_compare_desfire()` at product layer if loopback compare sufficient
