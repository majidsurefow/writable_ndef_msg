# P6 — MIFARE Classic TX Asserts — Agent Dispatch Brief

**Priority:** #6  
**Goal:** Add cookbook §14.3 TX asserts to Tier B Classic poller tests; optional converter TX sidecar. **No poller rewrite** — 98-step RX replay already Flipper-aligned.  
**Optional tag sim deferred.**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper Classic/Crypto1?** | **YES** — `mf_classic_poller.c`, `mf_classic_poller_i.c`, `crypto1.c` |
| **Local `.nfc` golden?** | **YES** — `tests/fixtures/nfc/flipper/MfClassic_1K_4b.nfc` (repo-only; not in Flipper upstream unit-test dir) |
| **98-step RX replay aligned?** | **YES** — 2 detect + 16×6 sector ops; Crypto1 determinism via `CONFIG_NFC_CLASSIC_TEST_DETERMINISTIC_NR` |
| **TX asserts?** | **NO** — missing `test_read_tx_sequence`; only `nfc_session_mock_tx_count()==1` on detect |
| **Blocker for full 98-TX?** | `NFC_SESSION_MOCK_MAX_TX = 16` — use **spot-check** pattern (3–4 TX like NDEF's 6) |

---

## Exact files and functions to edit

| Action | Path | Functions / symbols |
|--------|------|---------------------|
| **Add TX test** | `tests/unit/nfc_classic/src/test_poller.c` | New `test_read_tx_sequence`; copy `assert_tx_equals()` pattern from NDEF |
| **Add §14.3 gaps** | `tests/unit/nfc_classic/src/test_poller.c` | `test_read_truncated`, `test_read_no_session_end`, inactive session, EIO/timeout |
| **Extend converter** | `scripts/nfc/flipper_nfc_to_fixture.py` | Optional `classic_*_tx.inc` emission from `classic_read_steps()` |
| **Optional cap raise** | `tests/common/include/nfc_session_mock.h` L20 | Only if pursuing full 98-TX (not recommended v1) |
| **Update CONTEXT** | `src/nfc/protocols/classic/CONTEXT.md` | Name §14.3 case list (model: NDEF CONTEXT) |
| **Doc drift** | `tests/fixtures/nfc/flipper/README.md`, `docs/nfc/NFC_APPLETS_AND_TESTING.md` | Reconcile 12 vs 13 file manifest |

### Reference implementations (copy pattern)

- `tests/unit/nfc_ndef/src/test_poller.c` — `assert_tx_equals()` L51–58, `test_read_tx_sequence` L189–202
- `src/nfc/protocols/classic/classic_poller.c` — auth + encrypted read (already cites Flipper)

### Flipper reference (read-only)

- `flipperzero/lib/nfc/protocols/mf_classic/mf_classic_poller_i.c` — `mf_classic_poller_send_encrypted_frame()`
- `flipperzero/applications/debug/unit_tests/tests/nfc/nfc_test.c` — `mf_classic_send_frame_test`

---

## Wire format bytes / parity targets

### 98-step math (1K, default key `FF…FF`)

```
2 detect (4K probe block 254 fail + 1K probe block 62 NT)
+ 16 sectors × (2 auth + 4 block reads) = 96
= 98 total transceives (steps 0–97 in MfClassic_1K_4b_mock.h)
```

### Sector 0 TX spot-check targets (recommended assert set)

| Index | Plaintext TX (before Crypto1) | Encrypted TX |
|-------|------------------------------|--------------|
| Detect | `[0x60, 0x3E]` or `[0x60, 0xFE]` probes | Plain |
| Sector 0 auth | `[0x60, 0x00]` | Plain |
| NR\|AR | 8 B NR + 4 B AR | Encrypted (`NR={01,02,03,04}` deterministic) |
| Block 0 READ | `[0x30, 0x00]` + CRC-A (4 B) | Encrypted 4 B ciphertext |

**Golden ciphertext RX** already in `MfClassic_1K_4b_mock.h` — e.g. `classic_MfClassic_1K_4b_step4_rx` used in `test_crypto_decrypt_block0_golden()` L94–96.

### Intentional divergences (do not "fix" in P6)

- Flipper custom parity in HAL — mock sees ciphertext bytes only
- No Classic listener / E+ loopback — clone-only policy
- Fixed default key only — no dict attack

---

## Current bugs (file:line)

| Bug | Location | Impact |
|-----|----------|--------|
| No `test_read_tx_sequence` | `tests/unit/nfc_classic/src/test_poller.c` | Cookbook §14.3 gap |
| Detect asserts TX count only | `test_poller.c` L49 | No byte-level TX check |
| `NFC_SESSION_MOCK_MAX_TX=16` | `nfc_session_mock.h` L20 | Blocks full 98-TX log |
| Missing truncated/no_session_end tests | `test_poller.c` | §14.3 incomplete vs NDEF |
| Flipper manifest 12 vs 13 files | `NFC_APPLETS_AND_TESTING.md` vs `flipper/README.md` | Doc drift |
| §14.3 case list under-documented | `classic/CONTEXT.md` | vs NDEF CONTEXT |

**Not bugs:** RX replay and Crypto1 decrypt goldens (`test_crypto_decrypt_block0_golden` L64–97, `test_read_1k_golden` L134–146) — **keep green**.

---

## DO NOT touch

- `src/nfc/protocols/classic/classic_poller.c` **logic** — unless TX test reveals real bug (unlikely)
- `src/nfc/protocols/ultralight/**` — P1
- Classic listener / virtual RF tag sim — optional later
- Regenerate 98-step RX fixtures unless `.nfc` or key material changes
- Flipper submodule

---

## Staged commits (exact message templates)

### Commit 1 — TX assert helper + spot-check test

```
tests/classic: add test_read_tx_sequence with sector-0 TX goldens

Spot-check plain auth [60,00], NR|AR ciphertext, first encrypted READ;
mirror NDEF assert_tx_equals pattern.
```

### Commit 2 — Converter TX sidecar (optional)

```
scripts/nfc: emit classic TX golden arrays from flipper_nfc_to_fixture.py

Optional *_tx.inc alongside existing RX steps for maintainability.
```

### Commit 3 — Remaining §14.3 poller cases

```
tests/classic: add truncated RX and no_session_end poller cases

Fill cookbook §14.3 gaps: inactive session, EIO, timeout on detect/read.
```

### Commit 4 — Docs

```
docs/nfc: reconcile Flipper fixture manifest and classic CONTEXT §14.3

Document 13 flipper files including MfClassic_1K_4b.nfc; list named ztests.
```

---

## Twister verify commands

```bash
REPO=/Users/majidfaroud/writable_ndef_msg

west twister -T "$REPO/tests/unit/nfc_classic" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

# Store roundtrip (Tier E)
west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.store -v
```

**Pass criteria:** `sample.nfc.unit.nfc_classic.{model,poller}` — 17 cases remain green + new TX tests pass.

---

## Caller sweep checklist

- [ ] `tests/unit/nfc_classic/src/test_poller.c` — all ztests
- [ ] `tests/unit/nfc_classic/src/test_model.c` — unchanged
- [ ] `scripts/nfc/flipper_nfc_to_fixture.py` — `classic_read_steps()`, `mf_classic_crypto1.py`
- [ ] `tests/fixtures/classic/MfClassic_1K_4b_mock.h` — 98 steps unchanged unless regenerating TX sidecar
- [ ] `src/nfc/protocols/classic/crypto1.c` — spot tests still decrypt golden RX
- [ ] `tests/unit/nfc_reader/src/test_store_classic_roundtrip.c` — Tier E envelope

---

## Deferred scope

- Full 98-TX assert (raise `NFC_SESSION_MOCK_MAX_TX` to 128)
- Classic virtual tag sim via `mf_classic_listener` behavioral reference
- Additional Flipper generator variants (mini/4K/7b UID)
- E+ virtual loopback for Classic (explicitly SKIP)
- Custom parity modeling in session mock
