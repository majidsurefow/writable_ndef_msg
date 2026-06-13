# Store fixture goldens (Tier E)

Binary `.card.bin` envelopes match the layout produced by `nfc_store_save()` in
`src/nfc/store/nfc_store.c` (magic `NF`, version `0x02`, TLV entries, CRC16-CCITT).

CI links committed binaries and companion `*_card.inc` arrays only — no host parser at runtime.

## Files (13 `.card.bin` goldens)

| File | Protocol | Source |
|------|----------|--------|
| `ndef_empty.card.bin` | NDEF (empty T4T) | `ndef_to_fixture.py` / cookbook §5.1 |
| `ndef_uri_5byte.card.bin` | NDEF (URI, 5 B) | same |
| `ndef_chunk_255.card.bin` | NDEF (NLEN=300 transport cap) | same |
| `Ultralight_11.card.bin` | MIFARE Ultralight EV1 (11 pages) | `flipper_nfc_to_fixture.py --card-bin` |
| `Ultralight_21.card.bin` | MIFARE Ultralight EV1 (21 pages) | same |
| `Ultralight_C.card.bin` | MIFARE Ultralight C | same |
| `Ntag213_locked.card.bin` | NTAG213 (locked) | same |
| `Ntag215.card.bin` | NTAG215 | same |
| `Ntag216.card.bin` | NTAG216 | same |
| `Felica.card.bin` | FeliCa | same |
| `Slix_cap_default.card.bin` | ISO15693 SLIX — default CAP | same |
| `Slix_cap_missed.card.bin` | ISO15693 SLIX — missed CAP | same |
| `Slix_cap_accept_all_pass.card.bin` | ISO15693 SLIX — accept-all CAP | same |

Companion C includes (where present): `ndef_empty_card.inc`, `ndef_uri_5byte_card.inc`, `ndef_chunk_255_card.inc`, `store_fixture.h`.

HAL `nfc_nfca_signal_*` fixtures do not produce `.card.bin` (framing-only).

## Regenerate

From repo root:

```bash
# NDEF (3 goldens + Tier A/B fixtures)
python3 scripts/nfc/ndef_to_fixture.py --variant all

# All Flipper-derived model/read fixtures + 10 store goldens
python3 scripts/nfc/flipper_nfc_to_fixture.py --all --card-bin
```

Per-variant NDEF wrap only:

```bash
python3 scripts/nfc/ndef_to_card_bin.py \
  --model tests/fixtures/ndef/empty.bin \
  --output tests/fixtures/store/ndef_empty.card.bin
```

Commit updated `.bin`, `.inc`, and any test expectations in the same change.

See cookbook §14.12 and [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md).
