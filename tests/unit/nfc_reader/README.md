# NFC reader applet unit tests (Tier E + E+)

**Status:** Active — 13 ztests on QEMU (`testcase.yaml` has no `skip`)  
**Normative:** [NFC_PROTOCOLS_COOKBOOK.md](../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md) §14.6a, §14.11 · [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md)

## Scope

QEMU tests for **applet orchestration** — not protocol poller/listener logic (those are Tier A/B/C in `tests/unit/nfc_<proto>/`).

### Store (`test_store_roundtrip.c`) — 7 tests

| Test | Covers |
|------|--------|
| `test_store_save_envelope` | TLV header + CRC valid for mock `.card` |
| `test_store_load_roundtrip` | save → load → `memcmp` model |
| `test_store_load_golden_empty_card` | load committed `ndef_empty.card.bin` |
| `test_store_save_matches_golden_empty_card` | save output matches golden |
| `test_store_peek_golden_empty_card` | peek API on golden slot |
| `test_store_peek_missing_slot` | `-ENOENT` on missing slot |
| `test_store_peek_rejects_read_only_partial` | partial blob rejected |

### Verify (`test_verify_diff.c`) — 3 tests

| Test | Covers |
|------|--------|
| `test_verify_diff_pass` | mock read matches stored blob → PASS |
| `test_verify_diff_fail` | intentional model drift → FAIL |
| `test_verify_diff_uid_fail` | UID mismatch → FAIL |

### Virtual loopback E+ (`test_virtual_loopback.c`) — 3 tests

| Test | Golden |
|------|--------|
| `test_virtual_loopback_empty_card` | `ndef_empty.card.bin` |
| `test_virtual_loopback_uri_5byte` | `ndef_uri_5byte.card.bin` |
| `test_virtual_loopback_chunk_255` | `ndef_chunk_255.card.bin` |

### TODO

| Test | Notes |
|------|-------|
| `test_reader_clone_slot` | poller walk → `nfc_store_save` for named slot — not implemented yet |

## Twister

```bash
west twister -T "$REPO/tests/unit/nfc_reader" -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t writable_ndef.nfc.reader -v
```

HIL sign-off remains Tier D (`nfc loop` on PN7160) — not this suite.
