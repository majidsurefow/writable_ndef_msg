# Protocol Ship Status

Track sequential Flipper parity ship. **Only one protocol in PROGRESS at a time.**

| P | Protocol | Status | Flipper fixtures | Tier B TX | Tier C listener | E+ deep compare | Last commit |
|---|----------|--------|------------------|-----------|-----------------|-----------------|-------------|
| P1 | Ultralight | **SHIPPED** | 6 `.nfc` + `.card.bin` | full TX all fixtures | native T2 + T4 adapter | yes (`ultralight_compare`) | 6b3b804 |
| P2 | FeliCa | WIP (poller Lite) | Felica.nfc | 29-TX | none | SKIP | 1c60749 |
| P3 | SLIX / ISO15693 | WIP | 3× Slix_cap | addressed | none | SKIP | 903845b |
| P4a | EMV | WIP | synthetic | yes | yes | **envelope only** | b80d127 |
| P4b | Aliro | WIP | wave5 | SELECT+AUTH | AUTH chain | yes | e259343 |
| P5 | DESFire | WIP | MfDesfire_EV1 | app/file SM | partial | yes | 906c68e |
| P6 | Classic | WIP | MfClassic_1K_4b | spot TX | none | SKIP | 35838a4 |
| P8 | NDEF T4 | **SHIPPED** (reference) | NXP goldens | full | yes | yes | 7d469ec |

**Program gate:** all rows SHIPPED + full twister green on `77ef29a` baseline.
