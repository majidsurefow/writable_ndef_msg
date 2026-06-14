# P4 — EMV Stronger Loopback Compare + Multi-Step Poller — Agent Dispatch Brief

**Priority:** #4 (EMV half)  
**Goal:** Deepen `emv_compare`, PPSE-driven AID selection, AFL multi-record READ RECORD loop; expand poller/listener tests.  
**Flipper provides no EMV wire goldens — work is entirely synthetic/HIL.**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper EMV read (PPSE/GPO/records)?** | **NO** — no `emv/` protocol in `flipperzero/lib/nfc/` |
| **Flipper EMV assets?** | Lookup only: `nfc_emv_parser.c` + `resources/nfc/assets/aid.nfc` (AID name catalog, **zero call sites**) |
| **Local EMV `.nfc`?** | **NO** — goldens from `emv_card_image_load_default()` + `protocol_to_card_bin.py` |
| **Loopback fidelity?** | **LOW** — compare checks `record_count` only; poller hardcodes Visa AID + single READ RECORD |
| **Blocked on Flipper?** | **NO** — blocked on our poller/compare depth |

---

## Exact files and functions to edit

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Refactor read flow** | `src/nfc/protocols/emv/emv_poller.c` | `emv_poller_read()` L60–133 |
| **Add PPSE parse** | `src/nfc/protocols/emv/emv_poller.c` or `emv.c` | Parse FCI tag `BF0C`/`61`/`4F` for candidate AIDs |
| **Relax AID lock** | `src/nfc/protocols/emv/emv.c` | `emv_deserialize()` L146–148, L229–231; `emv_card_image_load_default()` L130 |
| **Deepen compare** | `tests/unit/nfc_reader/src/test_virtual_loopback.c` | `emv_compare()` L621–636, `test_virtual_loopback_emv()` L659–687 |
| **Optional shared compare** | `src/nfc/applets/nfc_applet_verify_compare.c` | Add EMV branch (currently NDEF-only) |
| **Expand poller tests** | `tests/unit/nfc_emv/src/test_poller.c` | Multi-record AFL mock cases |
| **Extend mocks** | `tests/fixtures/emv/Emv_mock.h` | 2+ READ RECORD steps for multi-record AFL |
| **Second golden** | `scripts/nfc/protocol_to_card_bin.py`, `tests/fixtures/store/Emv_card.inc` | Mastercard/Amex variant (AID hex from Flipper `aid.nfc` read-only) |
| **Spec reference** | `docs/nfc/archive/waves/wave5-emv.md` §8 APDU catalog | PPSE/GPO/READ RECORD shapes |

### Flipper reference (read-only, AID names only)

- `flipperzero/applications/main/nfc/resources/nfc/assets/aid.nfc` — e.g. `A0000000041010` MC, `A000000025010104` Amex

---

## Wire format bytes / parity targets

### Current poller APDU sequence (`emv_poller.c`)

| Step | TX bytes | Notes |
|------|----------|-------|
| PPSE SELECT | `00 A4 04 00 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31` | L60–65, L83 |
| App SELECT (hardcoded Visa) | `00 A4 04 00 07 A0 00 00 00 03 10 10` | L66–67, L92 — **must become PPSE-derived** |
| GPO | `80 A8 00 00 02 83 00 00` | L68, L101 |
| READ RECORD (single) | `00 B2 01 0C 00` | L69, L115 — **must loop per AFL** |

### Default golden model fields (`emv.h` / `emv.c`)

```
PAN:     BCD 9999999999999999
AID:     A0 00 00 00 03 10 10 (Visa test RID)
AFL:     08 01 01 00  (SFI=1, records 1–1)
AIP:     00 00 (zeroed after read today — bug)
Records: 1 × tag-70 blob with track2 tag 57
```

### Target poller behavior (cookbook §5.5)

1. SELECT PPSE → parse FCI → enumerate AIDs  
2. SELECT first/priority AID (not hardcoded)  
3. GPO → parse AIP/AFL from response  
4. READ RECORD per AFL `(SFI, first_rec..last_rec)` — up to `CONFIG_NFC_EMV_MAX_RECORDS` (2)  
5. Extract PAN (`5A`), track2 (`57`) from record TLVs  
6. **Do not** zero AIP post-read  

### Mock format (`Emv_mock.h`)

4-step RX script today: PPSE FCI → app FCI → GPO → single record — extend for multi-record AFL.

---

## Current bugs (file:line)

| Bug | Location | Impact |
|-----|----------|--------|
| Compare checks `record_count` only | `test_virtual_loopback.c` L632–634 | PAN/track2/AID/AFL not verified in loopback |
| Expected model stub `{record_count=1}` | `test_virtual_loopback.c` L667 | No full struct compare |
| Hardcoded Visa AID after PPSE | `emv_poller.c` L66–67, L80–81, L92 | Ignores PPSE FCI directory |
| Single fixed READ RECORD | `emv_poller.c` L69, L115 | AFL loop unused |
| AIP zeroed after successful read | `emv_poller.c` L131–132 | Overwrites GPO-derived AIP |
| Deserialize rejects non-Visa AID | `emv.c` L146–148, L229–231 | Blocks second golden variant |
| No EMV path in verify applet | `nfc_applet_verify_compare.c` | NDEF-only compare helper |
| Store roundtrip stronger than loopback | `test_store_emv_roundtrip.c` | Inconsistent test depth |

---

## DO NOT touch

- `src/nfc/protocols/ultralight/**` — P1
- Porting Flipper `nfc_emv_parser.c` (FuriString/Storage — non-portable)
- CDOL/ARQC/GENERATE AC, live payment crypto
- `flipper_nfc_to_fixture.py` EMV path (no Flipper `.nfc` source)
- Aliro files — separate P4 Aliro brief
- EMV listener crypto beyond existing test scope unless needed for new poller steps

---

## Staged commits (exact message templates)

### Commit 1 — Loopback compare depth

```
tests/nfc_reader: deepen emv_compare for loopback golden parity

Compare app_aid, pan, track2, afl, and record bytes; deserialize golden
into expected struct in test_virtual_loopback_emv.
```

### Commit 2 — AFL multi-record poller

```
nfc/emv: parse PPSE FCI and loop READ RECORD per AFL

Select first AID from PPSE response; stop zeroing AIP after read;
populate PAN/track2 from record TLV parse.
```

### Commit 3 — Poller tests + mocks

```
tests/emv: add multi-record AFL mock and poller unit cases

Extend Emv_mock.h with second READ RECORD step; assert record_count > 1 path.
```

### Commit 4 — Second AID golden (optional)

```
tests/emv: add Mastercard synthetic golden via protocol_to_card_bin

Relax emv_deserialize AID lock or Kconfig-select default AID for test builds.
```

### Commit 5 — Docs

```
docs/nfc: note EMV goldens are synthetic not Flipper-derived

Add tests/fixtures/emv/README if created; cite wave5-emv §8.
```

---

## Twister verify commands

```bash
REPO=/Users/majidfaroud/writable_ndef_msg

# EMV unit suites
west twister -T "$REPO/tests/unit/nfc_emv" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Loopback (requires CONFIG_NFC_TEST_LOOPBACK in nfc_reader)
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.shell_off -v

# EMV-specific loopback filter (if tagged)
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v \
  --testcase-filter "*loopback_emv*"
```

**Pass criteria:** `sample.nfc.unit.nfc_emv.{model,poller,listener}` green; `test_virtual_loopback_emv` passes with deep compare; no store roundtrip regression.

---

## Caller sweep checklist

- [ ] `src/nfc/reader/nfc_reader_poller_registry.c` — EMV poller registration
- [ ] `src/nfc/protocols/emv/emv_listener.c` — listener responses match new poller steps
- [ ] `tests/unit/nfc_emv/src/test_listener.c` — wrong SFI / record range cases
- [ ] `tests/unit/nfc_emv/src/test_model.c` — serialize roundtrip after AID relax
- [ ] `tests/unit/nfc_reader/src/test_store_emv_roundtrip.c` — envelope flags
- [ ] `scripts/nfc/protocol_to_card_bin.py --emv` — regenerate `Emv_card.inc`
- [ ] `src/nfc/applets/nfc_applet_verify_compare.c` — if EMV branch added
- [ ] Any shell `nfc read` path using `emv_poller_read()`

---

## Deferred scope

- HIL reader-captured `.card.bin` from real PN7160 tag
- Full EMVCo CDOL/ARQC path
- Multi-AID priority selection policy (Visa vs MC preference rules)
- Flipper `country_code.nfc` / `currency_code.nfc` field labeling in firmware UI
- EMV applet verify at product layer (optional vs loopback-only compare)
