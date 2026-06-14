# Protocol Ship Status

Track sequential Flipper parity ship. **Only one protocol in PROGRESS at a time.**

| P | Protocol | Status | Flipper fixtures | Tier B TX | Tier C listener | E+ deep compare | Last commit |
|---|----------|--------|------------------|-----------|-----------------|-----------------|-------------|
| P1 | Ultralight | **SHIPPED** | 6 `.nfc` + `.card.bin` | full TX all fixtures | native T2 + T4 adapter | yes (`ultralight_compare`) | 6b3b804 |
| P2 | FeliCa | **SHIPPED** | Felica.nfc + `.card.bin` | 29-TX full assert | virtual listener | yes (`felica_compare`) | 0d4db7b |
| P3 | SLIX / ISO15693 | **SHIPPED** | 3× Slix_cap + `.card.bin` | 85/87-TX full assert | virtual listener | yes (`slix_compare`) | 13b9d86 |
| P4a | EMV | **SHIPPED** | synthetic + MC `.card.bin` | PPSE/AFL/READ RECORD | listener Tier C | yes (`emv_compare`) | f585ac1 |
| P4b | Aliro | **SHIPPED** | wave5 + `Aliro.card.bin` | SELECT+AUTH+EXCHANGE | AUTH chain + EXCHANGE | yes (`aliro_compare_auth_chain`) | 9ade00f |
| P5 | DESFire | **SHIPPED** | MfDesfire_EV1 + `.card.bin` | 13-TX app/file SM | listener file types | yes (`desfire_compare`) | c0fc501 |
| P6 | Classic | **SHIPPED** | MfClassic_1K_4b + `.card.bin` | 98-TX full assert | virtual listener + Crypto1 | yes (`classic_compare`) | 38eb14b |
| P8 | NDEF T4 | **SHIPPED** (reference) | NXP goldens | full | yes | yes | 7d469ec |

**Program gate:** all rows SHIPPED + full twister green on `77ef29a` baseline.
