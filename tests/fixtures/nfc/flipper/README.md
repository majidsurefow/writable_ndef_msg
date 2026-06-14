# Flipper NFC unit-test fixtures

Golden tag dumps copied from Flipper Zero’s NFC unit-test resources. They are **reference inputs only** — CI and ztest use derived `.inc` / `.bin` artifacts, not FlipperFormat parsing at runtime.

## Manifest (13 files)

| File | Device / role |
|------|----------------|
| `MfClassic_1K_4b.nfc` | MIFARE Classic 1K (4-byte UID) — **generator golden** |
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

Convert all Flipper inputs (Tier A/B + store `.card.bin` where applicable):

```bash
python3 scripts/nfc/flipper_nfc_to_fixture.py --all --card-bin
```

Single file:

```bash
python3 scripts/nfc/flipper_nfc_to_fixture.py \
  --input tests/fixtures/nfc/flipper/<name>.nfc \
  --out-dir tests/fixtures/<protocol>/ \
  --card-bin
```

Outputs: `<stem>_model.bin`, `<stem>_read.inc`, and (except HAL signal files) `tests/fixtures/store/<stem>.card.bin`.

NDEF store goldens (no Flipper `.nfc`): `python3 scripts/nfc/ndef_to_fixture.py --variant all`.

See `scripts/nfc/flipper_nfc_to_fixture.py --help`. After upstream Flipper updates, re-copy the 12 basenames from the upstream directory above, then re-run `--all --card-bin`.

## Per-card parity tracker

| Flipper `.nfc` | Tier A/B fixtures | Tier A/B/C ztest | Tier E+ | `.card.bin` |
|----------------|-------------------|------------------|---------|-------------|
| `Ultralight_11.nfc` | `tests/fixtures/ultralight/` | pending F1 | pending F1 | yes |
| `Ultralight_21.nfc` | yes | pending F1 | pending F1 | yes |
| `Ultralight_C.nfc` | yes | pending F1 | pending F1 | yes |
| `Ntag213_locked.nfc` | yes | pending F1 | pending F1 | yes |
| `Ntag215.nfc` | yes | pending F1 | pending F1 | yes |
| `Ntag216.nfc` | yes | pending F1 | pending F1 | yes |
| `Felica.nfc` | `tests/fixtures/felica/` | pending F4 | **SKIP** | yes |
| `Slix_cap_default.nfc` | `tests/fixtures/slix/` | pending F4 | **SKIP** | yes |
| `Slix_cap_missed.nfc` | yes | pending F4 | **SKIP** | yes |
| `Slix_cap_accept_all_pass.nfc` | yes | pending F4 | **SKIP** | yes |
| `nfc_nfca_signal_short.nfc` | `tests/fixtures/hal/` | pending (HAL) | n/a | n/a |
| `nfc_nfca_signal_long.nfc` | NFC-A signal capture (long) |
| `MfClassic_1K_4b.nfc` | `tests/fixtures/classic/` | F2 Tier A/B | **SKIP** | yes |

Full policy: [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md) — per-card Flipper parity.

Full registration checklist: [NFC_PROTOCOLS_COOKBOOK.md](../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md) §14.11 · [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md).
