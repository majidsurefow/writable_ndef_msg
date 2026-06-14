# NFC reader applet unit tests (Tier E + E+)

**Status:** Active — **59 ztests** across 12 source files on QEMU (3 scenarios: `store`, `store_ram`, `shell_off`)  
**Normative:** [NFC_PROTOCOLS_COOKBOOK.md](../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md) §14.6a, §14.11 · [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md)

## Scope

QEMU tests for **applet orchestration** — not protocol poller/listener logic (those are Tier A/B/C in `tests/unit/nfc_<proto>/`).

### Store roundtrip (`test_store_roundtrip.c`) — 12 tests

| Test | Covers |
|------|--------|
| `test_store_save_envelope` | TLV header + CRC valid for mock `.card` |
| `test_store_load_roundtrip` | save → load → `memcmp` model |
| `test_store_load_golden_empty_card` | load committed `ndef_empty.card.bin` |
| `test_store_save_matches_golden_empty_card` | save output matches golden |
| `test_store_peek_golden_empty_card` | peek API on golden slot |
| `test_store_peek_missing_slot` | `-ENOENT` on missing slot |
| `test_store_peek_rejects_read_only_partial` | partial blob rejected |
| + 5 additional roundtrip variants | multi-format store tests |

### Protocol-specific roundtrip — 23 tests

| File | Tests | Covers |
|------|-------|--------|
| `test_store_classic_roundtrip.c` | 5 | Classic key + sector |
| `test_store_felica_roundtrip.c` | 4 | FeliCa block |
| `test_store_slix_roundtrip.c` | 5 | SLIX CAP variants |
| `test_store_desfire_roundtrip.c` | 2 | DESFire app |
| `test_store_emv_roundtrip.c` | 3 | EMV record |
| `test_store_aliro_roundtrip.c` | 2 | Aliro credential |
| `test_store_import.c` | 2 | Golden import |

### Verify (`test_verify_diff.c`) — 3 tests

| Test | Covers |
|------|--------|
| `test_verify_diff_pass` | mock read matches stored blob → PASS |
| `test_verify_diff_fail` | intentional model drift → FAIL |
| `test_verify_diff_uid_fail` | UID mismatch → FAIL |

### Virtual loopback E+ (`test_virtual_loopback.c`) — 12 tests

| Test | Golden |
|------|--------|
| `test_virtual_loopback_empty_card` | `ndef_empty.card.bin` |
| `test_virtual_loopback_uri_5byte` | `ndef_uri_5byte.card.bin` |
| `test_virtual_loopback_chunk_255` | `ndef_chunk_255.card.bin` |
| + 9 protocol loopback variants | DESFire, EMV, Aliro, etc. |

### Headless applet tests (`test_applet_headless.c`) — 7 tests

| Test | L1 Function |
|------|-------------|
| `test_scan_get_result_no_session` | `nfc_applet_scan_get_result()` |
| `test_scan_get_result_null_out` | `nfc_applet_scan_get_result()` |
| `test_scan_get_result_populated` | `nfc_applet_scan_get_result()` |
| `test_get_card_meta_missing_slot` | `nfc_applet_get_card_meta()` |
| `test_get_card_meta_from_golden` | `nfc_applet_get_card_meta()` |
| `test_verify_compare_headless` | `nfc_applet_verify_compare()` |
| `test_verify_compare_headless_mismatch` | `nfc_applet_verify_compare()` |

### Poller registry (`test_poller_registry.c`) — 2 tests

| Test | Covers |
|------|--------|
| `test_poller_registry_ndef` | NDEF poller dispatch |
| `test_poller_registry_unknown` | Unknown tech fallback |

### TODO

| Test | Notes |
|------|-------|
| `test_reader_clone_slot` | poller walk → `nfc_store_save` for named slot — not implemented yet |

---

## Memory Pattern for Test Files

### Problem
Large NFC protocol data structures can cause **stack overflow** when allocated as local variables in test functions. This happens when build configuration changes memory layout (e.g., `CONFIG_SHELL=n` in the `shell_off` scenario).

### Solution
**For separate test binaries** (e.g., `nfc_classic`, `nfc_ultralight`), use static storage for structs > 256 bytes.

**For nfc_reader tests**, all source files compile into a single binary. Adding static storage to multiple files accumulates RAM and causes overflow. Keep local stack allocation unless a specific test proves problematic.

```c
// In SEPARATE test binaries (nfc_classic, nfc_ultralight, etc.):
static classic_data_t s_copy;  /* Static to avoid ~4KB stack allocation */

ZTEST(suite, test_foo)
{
    (void)memset(&s_copy, 0, sizeof(s_copy));
    ...
}

// In nfc_reader (shared binary) — use stack, be careful with multiple large locals:
ZTEST(suite, test_foo)
{
    classic_data_t expected;  // OK if only one large struct
    ...
}
```

### Affected Data Types
| Type | Approx Size | Notes |
|------|-------------|-------|
| `classic_data_t` | ~4KB | `blocks[256][16]` |
| `ultralight_data_t` | ~1KB | `pages[256][4]` |
| `desfire_data_t` | ~450 bytes | DESFire EV1/2/3 |
| `felica_data_t` | ~500+ bytes | FeliCa blocks |
| `aliro_data_t` | ~300+ bytes | Aliro credential |
| `emv_card_image_t` | ~300+ bytes | EMV records |

### Reference
P5b fixed `test_virtual_loopback_desfire` stack overflow by moving `desfire_data_t` to static (`s_desfire_loopback`).

---

## Twister

```bash
west twister -T "$REPO/tests/unit/nfc_reader" -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t writable_ndef.nfc.reader -v
```

HIL sign-off remains Tier D (`nfc loop` on PN7160) — not this suite.
