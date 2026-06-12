# Sigmation Stack Specification

**Status: LOCKED**

This is the authoritative specification for the Sigmation networking stack.
Every layer contract, coupling pattern, thread assignment, and object-lifetime rule
defined here is locked. Deviations require a documented design decision in the
commit message that introduces them.

---

## Physical Medium and Constraints

**Medium:** BLE Extended Advertising — connectionless, non-connectable.

**Max frame size:** 254 bytes (BLE `ADV_EXT_IND` maximum payload). No fragmentation.
The TLV encoder aborts at buffer capacity; oversized frames are not sent.

**Addressing at L1:** None. Every advertisement reaches every scanning peer.
Target filtering (by UUID embedded in the TLV payload) is handled at the application layer.

**ACK at L1:** None. Reliable delivery at L4 is:
  advertiser sends frame → receiver sends ACK advertisement → sender receives it.
This is the QUIC-over-UDP model — reliability layered on unreliable broadcast — not
TCP windowing. There is no protocol-level retry; callers re-submit on `TIMED_OUT`.

**Radio concurrency:** The BLE controller interleaves scan windows and advertising
bursts automatically. Scanning never pauses for TX traffic. RSSI is measured
from every received advertisement regardless of payload type.

---

## Layer Map

```
┌────────────────────────────────────────────────────────────────┐
│  L6  src/apps/                                                 │
│      heartbeat  scan_recorder  test_harness                    │
│      — application logic; uses transmission APIs only          │
│  L6  src/labs/  Track C labs (pingpong, spam, …)               │
├────────────────────────────────────────────────────────────────┤
│  L5  src/comms/protocol/                                       │
│      record_frame  tlv_processor  tlv_parser  tlv_api           │
│      ZBus channels                                              │
│      — on-air delimiters; payload decode; ZBus publish to L6    │
├────────────────────────────────────────────────────────────────┤
│  L4  src/comms/transport/                                      │
│      transmission  tx_channel  tx_batch  pending               │
│      inflight  tx_admit  transmission_rx                       │
│      — reliable delivery; transmission_rx uses record_frame    │
│      — per-channel TX state, ACK correlation, admit gate       │
├────────────────────────────────────────────────────────────────┤
│  L3  src/comms/peer_tracker/state_table/                  ✦   │
│      ble_state_table  device_table  row_rssi  row_persist      │
│      row_allow  row_touch  ble_proximity_fsm                   │
│      — peer directory, RSSI history, proximity zones           │
├────────────────────────────────────────────────────────────────┤
│  L2  src/comms/peer_tracker/reception/              ✦          │
│      reception  (+ ble_scan_buffer)                            │
│      — FIFO drain, state-table update, rate limit, demux       │
│      — every frame → L3 (RSSI+presence); TLV frames → L4+     │
├────────────────────────────────────────────────────────────────┤
│  L1  src/comms/radio/                               ✦          │
│      ble_scanner  scan_buffer  tx_air                          │
│      — BLE scan/advertise; scan_buffer uses record_frame_peek  │
├────────────────────────────────────────────────────────────────┤
│  L0  BLE Controller (nRF54L15 HCI / SoftDevice)     ✦          │
│      — not our code; boundary is the Zephyr BT API            │
└────────────────────────────────────────────────────────────────┘
  ✦ = always on; independent of L4–L6
```

**The minimal path** — L0 → L1 → L2 → L3 — operates independently of L4–L6.
State table, RSSI tracking, and proximity FSM function with no transport or protocol layers present.
`main_rssi.c` and `main_proximity.c` are the reference entry points for this path.

---

## Inter-Layer Coupling: Four Patterns

Every layer boundary uses exactly one of these patterns. Using the wrong one is an
API design bug, not a style choice.

### Pattern A — Ops struct (vtable)

**Zephyr reference:** `struct net_l2` (`include/zephyr/net/net_l2.h`),
`struct bt_l2cap_chan_ops` (`include/zephyr/bluetooth/l2cap.h`).

**When:** Tight, per-object coupling. Multiple related callbacks all fire against the
same object. Registered once at init, not changed at runtime. Lower layer owns the
call site; upper layer provides the implementation.

```c
/* Defined in the lower-layer header */
struct radio_ops {
    void (*on_rx)(uint64_t sender_uuid, struct net_buf *buf, void *user_ctx);
    struct net_buf *(*alloc_rx_buf)(void *user_ctx);  /* called from BT RX thread */
};

int radio_register_ops(const struct radio_ops *ops, void *user_ctx);
```

**Use at:** L0↔L1 (HCI→scanner), L1↔L2 (scanner→reception pre-alloc/prefilter).

### Pattern B — Register function with slist node

**Zephyr reference:** `net_conn_register()` (`subsys/net/ip/connection.c`),
`bt_conn_cb_register()` (`include/zephyr/bluetooth/conn.h`).

**When:** Optional, independently removable. Multiple subscribers may co-exist.
One or two callback types. Subscriber identity matters (for unregister).

```c
struct my_event_cb {
    sys_snode_t    node;
    my_event_fn    fn;
    void          *user_ctx;
};

int  my_module_register_event_cb(struct my_event_cb *cb,
                                  my_event_fn fn, void *user_ctx);
void my_module_unregister_event_cb(struct my_event_cb *cb);
```

**Critical:** Registration AND invocation must hold the same spinlock. Snapshot the
callback pointer under lock; call it outside the lock.

**Use at:** L2→L5 (route callback), L3→L4 (admit gate), L4→L6 (delivery callback).

### Pattern C — Static linker section

**Zephyr reference:** `NET_L2_INIT`, `BT_CONN_CB_DEFINE`,
`STRUCT_SECTION_ITERABLE` (`include/zephyr/sys/iterable_sections.h`).

**When:** Fixed, always-present, zero runtime overhead. Set of subscribers is known
at compile time and does not change after linking.

```c
/* In include/: define the iterable struct */
struct tlv_record_handler {
    uint16_t          record_type;
    tlv_record_recv_fn recv;
};

#define TLV_RECORD_HANDLER_DEFINE(_name, _type, _fn)            \
    const STRUCT_SECTION_ITERABLE(tlv_record_handler, _name) = { \
        .record_type = (_type),                                  \
        .recv        = (_fn),                                    \
    }

/* In tlv_processor.c: iterate at runtime */
STRUCT_SECTION_FOREACH(tlv_record_handler, h) {
    if (h->record_type == type) { h->recv(sender_uuid, payload, len); }
}
```

**Use at:** TLV record type → handler dispatch (L5 → L6).

### Pattern D — ZBus pub/sub

**Zephyr reference:** ZBus (`include/zephyr/zbus/zbus.h`), our
`src/comms/protocol/tlv/processor/tlv_processor_channels.h`.

**When:** Loose coupling. Publisher does not know its consumers. Multiple independent
subscribers react independently. **L5→L6 only** — never used for L1–L4.

**Rules:**
- Never publish from BT RX thread or ISR — defer to a work queue first.
- Listener callbacks must not block.

---

## Boundary Coupling Map

| Boundary | Dir | Pattern | Zephyr reference |
|---|---|---|---|
| L0 ↔ L1 | both | Zephyr BT API | `bt_le_scan_cb`, `bt_le_ext_adv` |
| L1 → L2 (pre-alloc, prefilter) | up | **A** ops struct | `bt_le_scan_cb.recv` |
| L1 → L2 (scan buffer enqueue) | up | k_fifo | `net_if_recv_data` |
| L2 → L3 (state table update) | up | **A** direct call | `net_l2.recv → process_data` |
| L2 → L5 (TLV submit) | up | **B** registered route_cb | `net_conn_input` |
| L3 → L4 (admit gate) | — | **B** registered admit_cb | `net_conn_register` |
| L4 → L6 (delivery callbacks) | up | **B** per-slot delivery_fn | TCP → `net_context` recv_cb |
| L5 → L6 (TLV records to apps) | up | **D** ZBus | `zbus_chan_pub` |
| L6 → L4 (broadcast TX) | down | Direct API | `net_context_send` |

**Rule:** Downward data flow is always direct function calls.
Upward data flow (events, received frames) always uses A / B / C / D.

---

## Threading Model

| Thread | Priority | Runs |
|---|---|---|
| BT RX thread | Zephyr internal | `ble_scanner` scan_callback, pre_alloc hook, scan_prefilter |
| `rx_pipeline_wq` | 6 | reception drain, tlv_processor |
| `tx_pipeline_wq` | 4 | tx_channel, delivery callbacks (cb_work), cooldown timers |
| Caller thread | varies | `transmission_*()` |

`tx_pipeline_wq` (priority 4) preempts `rx_pipeline_wq` (priority 6).

**Delivery callbacks ALWAYS run on `tx_pipeline_wq`** — never on the caller's thread,
never on `rx_pipeline_wq`. Callbacks must not block or take a mutex with a non-zero
timeout.

**BT RX thread callbacks must not block and must not call back into the BT stack.**
Fast-path ACK handling (`scan_pre_alloc`) is exempt because it only writes atomics
and reads from a static pending table.

---

## L4 Reliability Model

Reliable delivery is retransmit-by-readvertise, not TCP windowing:

- One pending slot per outstanding reliable send.
- `pending` table tracks correlation ID, peer UUID, deadline, delivery callback.
- `transmission_rx` matches inbound ACK frames to pending slots.
- On match: fires delivery callback with `TRANSMISSION_DELIVERED`, frees slot.
- On deadline: fires delivery callback with `TRANSMISSION_TIMED_OUT`, frees slot.
- No automatic retry. Callers re-submit on TIMED_OUT.

The `tx_admit` gate enforces per-peer in-flight limits before a pending slot is
allocated. This is the L4 equivalent of TCP's congestion window.

---

## Callback Registration Rules

1. Register callbacks **after** `_init()` and **before** `_start()` of the emitting module.
   The emitting module must not fire events before the receiver is ready.

2. All wiring happens in `system_lifecycle.c`. No module includes another module's
   header to register cross-layer callbacks. The lifecycle file is the only place
   inter-module callbacks are set.

3. For Pattern B callbacks: the spinlock protecting the pointer must be held for
   both write (registration) and read (invocation snapshot). Call outside the lock.

4. At shutdown: clear callbacks before calling `_shutdown()` of the emitting module.
   See `reception_subsystem_shutdown()` in `system_lifecycle.c` for the reference.

---

## Buffer Contract

See `NETWORK_BUFFERS.md`. Summary:

- **One owner at all times.** Transfer is explicit. Never implicit.
- All pools are `NET_BUF_POOL_FIXED_DEFINE` — non-blocking, deterministic.
- Delivery callbacks receive payload as a transient pointer — not a `net_buf` reference.
  The pointer is invalid after the callback returns.
- Callee unrefs on error — caller never touches the pointer after transfer.

---

## What Does Not Exist Here (and Why)

| Absent | Reason |
|---|---|
| IP addresses | Peers identified by 64-bit device UUID from hardware identity |
| Routing | Broadcast medium; target filtering by UUID at the application layer |
| Fragmentation at L4 | 254 B fits one FIXED pool buffer; TLV aborts at capacity |
| BLE connections | Connectionless extended advertising only; no GATT, no ATT |
| Flow control at L1/L2 | Backpressure at L4 (tx_admit) |
| net_pkt | No IP stack, no sockets; `net_buf` is correct |
| Pattern D below L5 | ZBus is for L5→L6 loose coupling only |
