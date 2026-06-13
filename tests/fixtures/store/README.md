# Store fixture goldens (Tier E)

Binary `.card.bin` envelopes match the layout produced by `nfc_store_save()` in
`src/nfc/store/nfc_store.c` (magic `NF`, version `0x02`, TLV entries, CRC16-CCITT).

CI links committed binaries and companion `*_card.inc` arrays only — no host parser at runtime.

## Files

| File | Protocol | Source |
|------|----------|--------|
| `ndef_empty.card.bin` | NDEF (empty T4T) | Cookbook §5.1 empty model |
| `ndef_empty_card.inc` | same | C include mirror of `.card.bin` |

## Regenerate (NDEF)

From repo root:

```bash
python3 scripts/nfc/ndef_to_fixture.py
```

Or wrap an existing Tier A model only:

```bash
python3 scripts/nfc/ndef_to_card_bin.py \
  --model tests/fixtures/ndef/empty.bin \
  --output tests/fixtures/store/ndef_empty.card.bin
```

Commit updated `.bin`, `.inc`, and any test expectations in the same change.

## Other protocols

Flipper-ported protocols: run `flipper_nfc_to_fixture.py` for `.inc`/`.bin`, then
`ndef_to_card_bin.py` (or a protocol-specific wrapper) once serialize layout is known.
See cookbook §14.12.
