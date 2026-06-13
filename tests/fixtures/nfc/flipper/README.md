# Flipper NFC unit-test fixtures

Golden tag dumps copied from Flipper Zero’s NFC unit-test resources. They are **reference inputs only** — CI and ztest use derived `.inc` / `.bin` artifacts, not FlipperFormat parsing at runtime.

## Manifest (12 files)

| File | Device / role |
|------|----------------|
| `Ultralight_11.nfc` | MIFARE Ultralight EV1 (11 pages) |
| `Ultralight_21.nfc` | MIFARE Ultralight EV1 (21 pages) |
| `Ultralight_C.nfc` | MIFARE Ultralight C |
| `Ntag213_locked.nfc` | NTAG213 (locked) |
| `Ntag215.nfc` | NTAG215 |
| `Ntag216.nfc` | NTAG216 |
| `Felica.nfc` | FeliCa |
| `Slix_cap_default.nfc` | ISO15693 SLIX — default CAP |
| `Slix_cap_missed.nfc` | ISO15693 SLIX — missed CAP |
| `Slix_cap_accept_all_pass.nfc` | ISO15693 SLIX — accept-all CAP |
| `nfc_nfca_signal_short.nfc` | NFC-A signal capture (short) |
| `nfc_nfca_signal_long.nfc` | NFC-A signal capture (long) |

## Provenance

- **Upstream path:** `flipperzero/applications/debug/unit_tests/resources/unit_tests/nfc/`
- **License:** Flipper Zero firmware is **GPL-3.0**. These files are kept in-tree as behavioral references; product protocol code is **reimplemented** under our license (see [NFC_PROTOCOLS_COOKBOOK.md](../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md) §13).

## Regenerating derived fixtures

Convert a `.nfc` file to Tier A/B inputs (`.inc`, `.bin`, mock scripts) with:

```bash
python3 scripts/nfc/flipper_nfc_to_fixture.py \
  --input tests/fixtures/nfc/flipper/<name>.nfc \
  --out-dir tests/fixtures/<protocol>/
```

See `scripts/nfc/flipper_nfc_to_fixture.py` for options (`python3 … --help` exits 0). After upstream Flipper updates, re-copy the 12 basenames from the upstream directory above, then re-run the script for affected protocols.

Full registration checklist: [NFC_PROTOCOLS_COOKBOOK.md](../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md) §14.11 · [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md).
