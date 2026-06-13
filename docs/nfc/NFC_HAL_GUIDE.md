# NFC HAL — Backend Authoring Guide

**Status:** Authoring guide (non-normative).  
**Authority:** [`NFC_STACK_CONVENTIONS.md`](NFC_STACK_CONVENTIONS.md) + [`NFC_STACK_PLAN.md`](NFC_STACK_PLAN.md).  
**Historical detail:** optional deep dive in [`archive/waves/wave1-hal.md`](archive/waves/wave1-hal.md).

---

## 1. What HAL is

`hal/nfc_transport` is the **vendor-clean boundary** between controller hardware and the rest of `src/nfc/`. Everything above HAL — `reader/`, `nfc_stack/`, framing, router, `protocols/` — is backend-neutral.

- Public header `nfc_transport.h` has **no vendor includes**, types, or constants.
- Each backend is one `.c` file (e.g. `nfc_transport_pn7160.c`, `nfc_transport_nrfx.c`) selected by Kconfig.
- Transport-owned limits are `#define`s; backends `BUILD_ASSERT` equivalence to vendor limits.

---

## 2. Reader sub-API (poll)

Used by `reader/` for tag detect, clone, and verify. Blocking calls run on **`nfc_stack_wq`**, not the caller thread.

| HAL function | Role | PN7160 driver mapping |
|---|---|---|
| `nfc_transport_discover_start(tech_mask)` | Begin RF discovery | `pn7160_nci_discovery_start()` |
| `nfc_transport_discover_wait(info, timeout)` | Block until tag or timeout | `pn7160_nci_discovery_wait()` |
| `nfc_transport_tag_transceive(tx, rx, …)` | Exchange with active tag | `pn7160_nci_reader_tag_cmd()` |
| `nfc_transport_discover_stop()` | End discovery | `pn7160_nci_discovery_stop()` |

NFCT has **no reader sub-API** — on-die NFCT is listen-only. Reader paths compile only when a poll-capable backend is selected.

---

## 3. Listen sub-API (card emulation)

Used by `nfc_stack/` for Type-4 CE and protocol listeners. Upward events use **Pattern A** ops struct (`nfc_transport_register_callbacks`); responses go down via `nfc_transport_send_response()`.

| HAL surface | Role | PN7160 mapping | NFCT (nrfx) mapping |
|---|---|---|---|
| `nfc_transport_start(uid)` / `stop()` | Field listen lifecycle | `pn7160_nci_listen_start()` / `listen_stop()` + card-mode loop | `nfc_t4t_emulation_start()` / `stop()` |
| Ops: `on_field_on/off`, `on_apdu` | Upward events (WQ thread) | `pn7160_nci_card_mode_recv()` → assemble → callback | `NFC_T4T_EVENT_*` ISR → WQ re-dispatch |
| `nfc_transport_send_response(buf, len)` | Reply PDU | `pn7160_nci_card_mode_send()` | `nfc_t4t_response_pdu_send()` |
| `nfc_transport_set_uid(uid)` | Live NFCID1 rotation | PN7160 CE config | `NFC_T4T_PARAM_NFCID1` |

Inbound APDUs: FIXED `net_buf` pool, ISR allocates with `K_NO_WAIT`, WQ owns after fifo handoff (CONVENTIONS §5). Outbound: static service-owned buffer borrowed by HAL until next event.

---

## 4. Capabilities + BUILD_ASSERT

Each backend exports a static `nfc_transport_caps_t` via `nfc_transport_get_capabilities()`:

```c
typedef struct {
    uint8_t roles;          /* NFC_ROLE_READER | NFC_ROLE_LISTEN */
    uint32_t technologies;  /* NFC_TECH_* bitmask */
    nfc_hal_tier_t tier;
} nfc_transport_caps_t;
```

Backend file asserts at compile time, e.g.:

```c
BUILD_ASSERT(NFC_TRANSPORT_MAX_RESPONSE_LEN == NFC_T4T_MAX_PAYLOAD_SIZE);
```

Mismatch → fix the transport constant or open a spec change; do not `#ifdef` around limits in public code.

---

## 5. Work queues

| Queue | Owner | Runs |
|---|---|---|
| `pn7160_wq` | PN7160 Zephyr driver | IRQ → NCI RX drain, `pn7160_nci_process()` |
| `nfc_stack_wq` | `nfc_stack` / HAL backends | Discovery wait, card-mode dispatch, fifo drain, protocol response path |

**Single-flight:** one active poll session or one listen session at a time. `reader/` and `nfc_stack/` serialize access; HAL returns `-EBUSY` if re-entered while STARTED.

Never call scheduler APIs while holding a spinlock (CONVENTIONS §7).

---

## 6. What protocols must NOT do

Protocol modules under `protocols/` **must not**:

- `#include` vendor HAL or driver headers (`pn7160.h`, `nfc_t4t_lib.h`, …).
- Call `nfc_transport_*` poll/listen functions — they receive APDUs via the router and reply through session APIs.
- Register callbacks or wire cross-layer ops — wiring lives in `nfc_stack.c` only (CONVENTIONS §3).
- Allocate dynamically on hot paths; use static buffers and FIXED pools.
- Spawn threads or use the system work queue.

---

## 7. Backend checklist — PN7160

- [ ] Kconfig: `NFC_HAL_BACKEND_PN7160`; requires `CONFIG_PN7160`.
- [ ] Caps: `NFC_ROLE_READER | NFC_ROLE_LISTEN`; technologies per capability matrix.
- [ ] Poll: map discover/transceive/stop to `pn7160_nci_*` on `nfc_stack_wq`.
- [ ] Listen: card-mode recv/send loop; ops callbacks on `nfc_stack_wq`.
- [ ] `BUILD_ASSERT` response length vs NCI payload limit.
- [ ] Full lifecycle: init/start/stop/shutdown; Pattern B `atomic_t` state.
- [ ] Shell: `nfc_transport` subcmds in `nfc_transport_shell_cmds.c` (not `pn7160` — that stays driver-level).
- [ ] Stats/getters per CONVENTIONS §2 + §6.

---

## 8. Backend checklist — NFCT (stub)

- [ ] Kconfig: `NFC_HAL_BACKEND_NRFX`; mutually exclusive with PN7160 backend.
- [ ] Caps: listen only (`NFC_ROLE_LISTEN`); no reader symbols exported.
- [ ] Stub: init/shutdown + `get_capabilities()` + empty listen start/stop returning `-ENOTSUP` until gate 3.
- [ ] `BUILD_ASSERT(NFC_TRANSPORT_MAX_RESPONSE_LEN == NFC_T4T_MAX_PAYLOAD_SIZE)`.
- [ ] ISR → `nfc_stack_wq` bridge skeleton (field on/off, fragment fifo) — no full T4T CE until gate 3.
- [ ] Document NFCT peripheral exclusivity (one of T2T/T4T libs at a time).

---

## 9. Module contract pointer

Every HAL module follows CONVENTIONS §2:

- `<module>_config_t`, `<module>_stats_t`, `<module>_state_t`
- `get_config()` (never NULL), `get_stats()` (copy-out), `get_state()`
- `s_stats` + `s_stats_lock`; hot-path stats via private helper (§6)
- Shell: `config` / `stats` / `state` in `<module>_shell_cmds.c` (§10)
- Thread-safety annotations on every public function (§7)

For coupling, buffers, callbacks, and errno rules, read CONVENTIONS §3–§9 before writing backend code.
