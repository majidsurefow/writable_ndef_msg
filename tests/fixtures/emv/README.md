# EMV test fixtures

EMV store goldens and poller mocks are **synthetic** — not derived from Flipper `.nfc`
files (Flipper has no EMV protocol implementation; `aid.nfc` is a read-only AID name catalog).

## Sources

| Asset | Generator |
|-------|-----------|
| `tests/fixtures/store/Emv.card.bin` | `python3 scripts/nfc/protocol_to_card_bin.py --emv` |
| `tests/fixtures/store/Emv_mc.card.bin` | `python3 scripts/nfc/protocol_to_card_bin.py --emv-mc` |
| `tests/fixtures/emv/Emv_mock.h` | Hand-authored PPSE/GPO/READ RECORD RX scripts |

Default Visa test AID: `A0 00 00 00 03 10 10`. Mastercard variant: `A0 00 00 00 04 10 10`
(from Flipper `aid.nfc` catalog, names only).

APDU shapes and listener FSM: [`docs/nfc/archive/waves/wave5-emv.md`](../../docs/nfc/archive/waves/wave5-emv.md) §8.
