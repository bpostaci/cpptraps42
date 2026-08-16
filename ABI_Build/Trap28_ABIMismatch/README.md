# Trap 28 — ABI Mismatch

**Rule:** a binary boundary must agree on layout, alignment, packing, calling convention, ownership, and runtime rules, not just field names.

**This trap is IFNDR-style ABI breakage.** Real ABI mismatches are often ill-formed, no diagnostic required; sanitizer silence is expected because this target is a single-executable simulation.

## The rule

An ABI is the compiled contract between separately built code. It includes `sizeof`, `alignof`, member offsets, packing pragmas, architecture, runtime library, exception policy, allocator ownership, and calling convention. Source code that appears to describe the same fields can still produce different binary layouts.

When producer and consumer decode the same bytes with different offsets or alignment expectations, each side is internally consistent and the boundary is wrong. The compiler and linker are often not in a position to diagnose that contract mismatch.

## In this code

The trap is split across three files.

| File | Contribution |
|---|---|
| `layout.hpp` | defines the reporting struct `Layout` and declares `producer_layout()` / `consumer_layout()` |
| `producer.cpp` | defines packed `ProducerMessage` under `#pragma pack(push, 1)` and reports size, alignment, and `request_id` offset |
| `main.cpp` | defines naturally aligned `ConsumerMessage`, computes `consumer_layout()`, calls `producer_layout()`, and prints both reports |

There is no `Trap28_ABIMismatch_unsafe` target because no source file contains `RUN_UNSAFE_EXAMPLE`. The safe target is `Trap28_ABIMismatch`; the normal target demonstrates the mismatch directly.

## Why it fails

`ProducerMessage` places `request_id` at offset 4 with alignment 1; `ConsumerMessage` places it at offset 8 with natural 8-byte alignment. If bytes produced by one side are consumed as the other layout, the `request_id` field is read from the wrong bytes. In real separate builds this is an ABI contract failure that may be IFNDR or simply outside the C++ type system's ability to diagnose.

## Correct direction

```cpp
struct WireMessage {
    std::uint32_t version;
    std::uint64_t request_id;
};

static_assert(offsetof(WireMessage, request_id) == 8);
static_assert(alignof(WireMessage) == 8);
```

For stable boundaries, prefer opaque handles or serialized wire formats with explicit versioning and validation.

## Detection

| Tool | Result |
|---|---|
| Layout assertions | yes — compare `sizeof`, `alignof`, and every boundary offset |
| ABI/symbol inspection | yes — confirms which module and packing policy produced each side |
| Sanitizers | no — a layout contract mismatch can be entirely in-bounds |
| Linker | no — `producer_layout()` and `consumer_layout()` are valid functions with incompatible assumptions |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that measures the producer and consumer layout facts.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 28.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 24.
