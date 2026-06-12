# Network Buffer Guide

Reference for how `net_buf` is used in this firmware. Based on Zephyr's `net_buf`
implementation (`lib/net_buf/buf.c`, `include/zephyr/net/buf.h`).

---

## The One Rule

> **Every buffer has exactly one owner at any point in time.**
> The owner is responsible for calling `net_buf_unref()` when done.
> Ownership is transferred explicitly — never implicitly — at function call boundaries.

Violating this rule causes either a double-free (two unrefs) or a memory leak (no unref).
Both are silent in production and catastrophic under load.

---

## net_buf vs net_pkt — Which One to Use

**Use `net_buf`. Never use `net_pkt` in this codebase.**

`net_pkt` is Zephyr's IP networking packet abstraction. It adds:
- `struct net_context *context` — socket/connection binding
- `struct net_if *iface` — interface routing
- `struct net_pkt_cursor cursor` — multi-layer parse state
- `atomic_t atomic_ref` — thread-safe atomic reference counting
- Timestamps, VPN metadata, routing table entries

None of these are applicable here. We have no IP stack, no sockets, no interface objects.
Our frames are ≤254 B, parsed once, atomically owned. `net_buf` is correct.

`net_buf_simple` is a lighter variant with no pooling and no ref-counting. Use it only
for temporary, stack-allocated scratch buffers in a single function — never for data that
crosses a thread boundary.

---

## Pool Types

Zephyr provides four pool types. Choose one based on allocation pattern.

### `NET_BUF_POOL_FIXED_DEFINE` — use this by default

```c
NET_BUF_POOL_FIXED_DEFINE(my_pool, count, data_size, user_data_size, destroy_cb);
```

- Memory: static array, allocated at link time
- Allocation: O(1), deterministic, non-blocking
- Data ref-counting: none (single owner only)
- Failure mode: returns NULL immediately if pool exhausted
- **When to use:** Known buffer count, fixed data size, ISR/BT-RX-safe allocation

`NET_BUF_POOL_DEFINE` is an alias for this — they are identical.

### `NET_BUF_POOL_VAR_DEFINE` — variable-size payloads

```c
NET_BUF_POOL_VAR_DEFINE(my_pool, count, max_data_size, user_data_size, destroy_cb);
```

- Memory: `k_heap` shared across all buffers in the pool
- Allocation: can block (honors `K_FOREVER` timeout)
- Data ref-counting: yes — data payload has its own reference count, enabling
  `net_buf_clone()` to share data zero-copy
- **When to use:** When sizes vary significantly and memory efficiency matters more than
  allocation determinism. Not suitable for ISR context.

### `NET_BUF_POOL_HEAP_DEFINE` — system heap

```c
NET_BUF_POOL_HEAP_DEFINE(my_pool, count, user_data_size, destroy_cb);
```

- Memory: `k_malloc` from the system heap
- Allocation: always `K_NO_WAIT` for data regardless of timeout argument
- **When to use:** Rarely. Prefer FIXED for embedded firmware. Only use when pool count
  is unknown at compile time.

### Decision guide

```
Is the buffer size fixed and known at compile time?
  YES → NET_BUF_POOL_FIXED_DEFINE (or NET_BUF_POOL_DEFINE, same thing)
  NO  → Do you need zero-copy clone/share across threads?
          YES → NET_BUF_POOL_VAR_DEFINE
          NO  → Still prefer FIXED with the largest expected size

Can allocation happen from BT RX thread or ISR?
  YES → Must use FIXED (non-blocking, deterministic)
  NO  → Any pool is valid, but FIXED is still preferred
```

**In this firmware:** All pools are `NET_BUF_POOL_FIXED_DEFINE` (or the DEFINE alias).
This is correct. Do not change pool types without measuring the impact.

---

## Ownership Semantics

### After `net_buf_alloc()` — caller owns

```c
struct net_buf *buf = net_buf_alloc(&my_pool, K_NO_WAIT);
if (!buf) {
    /* Pool exhausted — increment dropped stat, return error */
    return -ENOMEM;
}
/* buf->ref == 1. We own it. We must eventually unref. */
```

Never assert on allocation failure. Increment a `dropped` stat and propagate the error.

### Passing without transfer — callee borrows

```c
/* Callee reads buf but does not take ownership */
void inspect(struct net_buf *buf)
{
    process(buf->data, buf->len);
    /* No unref here */
}

/* Caller unrefs when done */
inspect(buf);
net_buf_unref(buf);
```

Document this in the function's Doxygen: `@note Does not take ownership of @p buf.`

### Transfer — callee takes ownership

```c
/**
 * @brief Submit buffer for processing.
 *
 * Always consumes @p buf. Caller must NOT unref after this call.
 * On error, buf is freed internally.
 *
 * @return 0 on success, negative errno on failure.
 */
int module_submit(struct net_buf *buf);

/* Caller */
int rc = module_submit(buf);
/* buf is invalid from this point regardless of rc */
```

The callee unrefs on error — the caller never touches the buffer again after transfer.
This is the pattern used by `tlv_processor_submit()`, `tx_channel_submit()`, and
`ble_scan_buffer_enqueue()`.

### Sharing — explicit ref before passing

```c
/* Send same buffer to two consumers */
consumer_a(net_buf_ref(buf));   /* buf->ref == 2 */
consumer_b(net_buf_ref(buf));   /* buf->ref == 3 */
net_buf_unref(buf);             /* buf->ref == 2 — our ref released */
/* consumers_a and _b each own one ref; each will unref when done */
```

**This pattern is not used in our current codebase.** Every buffer has exactly one owner
at a time. Do not introduce ref-sharing without a documented reason.

### Error path discipline — unref before every return

```c
buf = net_buf_alloc(&pool, K_NO_WAIT);
if (!buf) { return -ENOMEM; }

rc = do_step_one(buf);           /* borrows */
if (rc != 0) {
    net_buf_unref(buf);          /* ← must unref on every early return */
    return rc;
}

rc = module_submit(buf);         /* transfers — buf is now gone */
if (rc != 0) {
    /* DO NOT unref here — module_submit already did */
    return rc;
}
return 0;
```

Add the unref on every early-return path before the ownership-transfer call.
After the transfer call, never touch the buffer pointer again.

---

## user_data — Per-Buffer Metadata

`user_data` is a fixed-size byte array allocated contiguously after the `net_buf` struct.
Its size is set at pool definition time and is the same for every buffer in that pool.
It is **zeroed on every allocation**.

```c
/* Define metadata struct */
struct scan_meta {
    uint64_t sender_uuid;
    int8_t   rssi_dbm;
    uint32_t rx_timestamp_ms;
};

/* Pool with user_data sized for this struct */
NET_BUF_POOL_FIXED_DEFINE(scan_pool, 16, 256,
                           sizeof(struct scan_meta), NULL);

/* Access via cast */
static inline struct scan_meta *scan_meta(struct net_buf *buf)
{
    return (struct scan_meta *)net_buf_user_data(buf);
}

/* Fill after alloc */
struct net_buf *buf = net_buf_alloc(&scan_pool, K_NO_WAIT);
scan_meta(buf)->sender_uuid    = uuid;
scan_meta(buf)->rssi_dbm       = rssi;
scan_meta(buf)->rx_timestamp_ms = k_uptime_get_32();
```

**Rules for user_data:**
- Define a named struct — never cast raw bytes directly at call sites
- Provide an inline accessor (like `scan_meta()` above) — never scatter casts
- user_data is owned by the buffer. It is not separately ref-counted. It is freed when
  the buffer's ref drops to zero.
- user_data is **copied, not shared** when `net_buf_clone()` is called

**In this firmware:** `ble_scan_metadata_t` is stored in `ble_scan_pool` user_data.
`buf_meta_t` (channel, alloc timestamp, correlation ID) is stored in tx pool user_data.
Both follow this pattern correctly.

---

## Headroom and Tailroom — Zero-Copy Header Manipulation

Headroom is empty space between `buf->__buf` and `buf->data`. It allows prepending a
header without copying the existing payload:

```
Before push:   [-- headroom --][  payload  ][-- tailroom --]
                               ^data

After push(4): [-- hdroom --][hdr][  payload  ][-- tailroom --]
                             ^data (moved back 4 bytes)
```

Tailroom is empty space after `buf->data + buf->len`. `net_buf_add()` appends there.

### When to use headroom

Use headroom when multiple protocol layers each prepend their own header to the same
buffer, and you want zero copies:

```c
/* L4 allocates with headroom for TLV + radio headers */
buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
net_buf_reserve(buf, TLV_HDR_SIZE + RADIO_HDR_SIZE);

/* L4 writes its payload */
uint8_t *p = net_buf_add(buf, payload_len);
memcpy(p, payload, payload_len);

/* TLV header prepended — no copy of payload */
struct tlv_hdr *h = net_buf_push(buf, sizeof(*h));
h->type = record_type;
h->len  = payload_len;

/* L1 prepends radio header — no copy */
struct radio_hdr *r = net_buf_push(buf, sizeof(*r));
r->type = FRAME_TYPE_DATA;
```

### When NOT to use headroom — our TLV model

Our TLV encode/decode does not use headroom or tailroom. This is intentional:

- TLV frames are encoded into **stack-allocated buffers** at the call site, then
  written into the BLE advertising payload via `tx_air`.
- Inbound TLV frames are parsed **in-place** from the scan buffer — the parser walks
  `buf->data` directly without modifying it.
- Frame size is bounded at 254 B. No fragmentation, no multi-layer header prepending.

If a future protocol layer needs multi-layer header prepending (e.g., adding an
encryption envelope), **use headroom**. Reserve it at allocation time with `net_buf_reserve()`.

---

## Fragment Chaining

Fragments are `net_buf` objects linked via `buf->frags`. Unreffing the head unrefs the
entire chain.

```c
/* Append a fragment */
net_buf_frag_add(head, frag);      /* head owns frag now; caller must not unref frag */

/* Remove a fragment */
struct net_buf *next = net_buf_frag_del(parent, frag);  /* unrefs frag */

/* Total length across all fragments */
size_t total = net_buf_frags_len(head);

/* Flatten to linear buffer */
net_buf_linearize(dst, dst_len, head, offset, len);

/* Unref chain — one call, all fragments freed */
net_buf_unref(head);
```

### When to use fragments — not in our current stack

Fragments are for reassembling multi-segment payloads (BLE ACL continuation packets,
IP fragmentation). Our stack does not use them:

- BLE extended advertising delivers complete payloads — no continuation.
- Max payload is 254 B — fits in one FIXED pool buffer.
- The TLV parser requires contiguous data (no boundary-crossing records).

**Do not add fragment chaining without a documented requirement.** The added complexity
(ownership across the chain, linearization cost) is not justified by our frame sizes.

---

## Destroy Callback

The optional `destroy_cb` fires when `buf->ref` reaches zero, before the buffer struct
is returned to the pool. Use it to release resources stored in `user_data`.

```c
static void my_pool_destroy(struct net_buf *buf)
{
    struct my_meta *m = (struct my_meta *)net_buf_user_data(buf);
    if (m->timer_active) {
        k_timer_stop(&m->timer);
    }
    /* MUST call net_buf_destroy() to complete return to pool */
    net_buf_destroy(buf);
}

NET_BUF_POOL_FIXED_DEFINE(my_pool, 16, 256, sizeof(struct my_meta), my_pool_destroy);
```

**If the destroy callback is NULL** (our current pools), `net_buf_destroy()` is called
directly by Zephyr — the buffer is returned to the pool's LIFO and its data is released.

The scan_buffer pool registers a custom wrapper via `ble_scan_buffer_unref()` — not as
a pool destroy callback but as a named function called by the pipeline. This is correct;
a pool destroy callback would also work but is less transparent.

---

## Our Pools at a Glance

| Pool | Definition | Count | Data | user_data | Owner |
|---|---|---|---|---|---|
| `ble_scan_pool` | `NET_BUF_POOL_DEFINE` | `BLE_SCAN_POOL_COUNT` | `BLE_SCAN_BUF_SIZE` (242 B) | `sizeof(ble_scan_metadata_t)` (56 B) | scanner → reception → tlv_processor |
| `ch_ack_pool` | `NET_BUF_POOL_DEFINE` | per Kconfig | `TX_CH_ACK_DATA_SIZE` | `sizeof(buf_meta_t)` (12 B) | tx_channel, CH_ACK |
| `ch_high_pool` | `NET_BUF_POOL_DEFINE` | per Kconfig | `TX_CH_HIGH_DATA_SIZE` | `sizeof(buf_meta_t)` | tx_channel, CH_HIGH |
| `ch_low_pool` | `NET_BUF_POOL_DEFINE` | per Kconfig | `TX_CH_LOW_DATA_SIZE` | `sizeof(buf_meta_t)` | tx_channel, CH_LOW via tx_batch |

All pools use FIXED (non-blocking, deterministic). This is correct.

---

## Checklist — Before Adding a New Buffer Use

- [ ] Did I check for NULL after alloc? (never assert on pool exhaustion)
- [ ] Does every early-return path call unref before the ownership-transfer call?
- [ ] After the ownership-transfer call, does any code path touch the buffer pointer?
      (It must not.)
- [ ] If passing across a thread boundary, is ownership transfer explicit?
- [ ] Is user_data accessed via a named struct and inline accessor?
- [ ] Is the buffer size the right pool size? (not silently truncating)
- [ ] If using fragments: is `net_buf_unref(head)` the only unref in the path?

---

## Anti-Patterns

### ❌ Touching buf after submit

```c
int rc = module_submit(buf);          /* ownership transferred */
if (rc == 0) {
    LOG_DBG("sent %d bytes", buf->len); /* BUG: buf may be freed */
}
```

### ❌ Missing unref on error path

```c
buf = net_buf_alloc(...);
if (step_one(buf) != 0) {
    return -EIO;                        /* BUG: buf leaked */
}
```

### ❌ Double unref

```c
net_buf_unref(buf);                     /* correct */
/* ... */
if (cleanup) {
    net_buf_unref(buf);                 /* BUG: double free */
}
```

### ❌ Stale pointer after unref

```c
net_buf_unref(buf);
do_something(buf->data);               /* BUG: buf is invalid */
```

### ✅ Correct ownership transfer

```c
buf = net_buf_alloc(&pool, K_NO_WAIT);
if (!buf) { s_stats.dropped++; return -ENOMEM; }

rc = encode_payload(buf);              /* borrows — does not unref */
if (rc != 0) {
    net_buf_unref(buf);                /* we still own it — unref before return */
    return rc;
}

rc = module_submit(buf);               /* transfers ownership */
if (rc != 0) {
    /* submit failed AND module already freed buf — do nothing */
    return rc;
}
return 0;
```
