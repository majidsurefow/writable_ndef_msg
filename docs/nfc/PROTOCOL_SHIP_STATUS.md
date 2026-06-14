# Protocol Ship Status

Track sequential Flipper parity ship. **Only one protocol in PROGRESS at a time.**

| P | Protocol | Status | Door-lock check¹ | Flipper fixtures | Tier B TX | Tier C listener | E+ deep compare | Last commit |
|---|----------|--------|------------------|------------------|-----------|-----------------|-----------------|-------------|
| P1 | Ultralight | **SHIPPED** | N/A (Flipper golden) | 6 `.nfc` + `.card.bin` | full TX all fixtures | native T2 + T4 adapter | yes (`ultralight_compare`) | 6b3b804 |
| P2 | FeliCa | **SHIPPED** | N/A (Flipper golden) | Felica.nfc + `.card.bin` | 29-TX full assert | virtual listener | yes (`felica_compare`) | 0d4db7b |
| P3 | SLIX / ISO15693 | **SHIPPED** | N/A (Flipper golden) | 3× Slix_cap + `.card.bin` | 85/87-TX full assert | virtual listener | yes (`slix_compare`) | 13b9d86 |
| P4a | EMV | **SHIPPED** | N/A (Flipper golden) | synthetic + MC `.card.bin` | PPSE/AFL/READ RECORD | listener Tier C | yes (`emv_compare`) | f585ac1 |
| P4b | Aliro | **SHIPPED** | **PASS (CI tier)**² | wave5 + `Aliro.card.bin` | SELECT+AUTH+EXCHANGE | AUTH chain + EXCHANGE | yes (`aliro_compare_auth_chain`) | 9ade00f |
| P5 | DESFire | **SHIPPED** | N/A (Flipper golden) | MfDesfire_EV1 + `.card.bin` | 13-TX app/file SM | listener file types | yes (`desfire_compare`) | c0fc501 |
| P6 | Classic | **SHIPPED** | N/A (Flipper golden) | MfClassic_{1K_4b,1K_7b,4K_4b,Mini_4b} + `.card.bin` | full TX assert (98/337/32) | virtual listener + Crypto1 | yes (`classic_compare`, 4 variants) | b4bf320 |
| P8 | NDEF T4 | **SHIPPED** (reference) | N/A (Flipper golden) | NXP goldens | full | yes | yes (`nfc_applet_verify_compare`) | d62310e |

**Program gate:** PASS — all rows SHIPPED; twister 475/475 ci_unit + ci_build + pn7160 + 4 nrf54l15 profiles green on `b3258c6`.

¹ Door-lock reference applies to Aliro only — see [`ALIRO_DOOR_LOCK_SHIP_CHECK.md`](ALIRO_DOOR_LOCK_SHIP_CHECK.md).  
² CI tier PASS (31/31 vectors + store/loopback); door-lock interop VERIFIED (PSA + Test Harness) pending.
