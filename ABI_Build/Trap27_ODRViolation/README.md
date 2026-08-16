# Trap 27 — ODR Violation

**Rule:** every odr-used class, inline function, and template entity must have the same definition in every translation unit.

**This trap is IFNDR.** The unsafe program is ill-formed, no diagnostic required, so linker and sanitizer silence is the expected outcome.

## The rule

The One Definition Rule is what lets separately compiled translation units agree about a type. If two translation units see different definitions of the same class, they may compile successfully but disagree about size, alignment, layout, calling convention, or generated code. The toolchain is not required to diagnose this category; it is commonly called IFNDR.

Preprocessor-controlled headers are a common source. A macro that is visible in only one `.cpp` can silently change a type that both sides believe has the same name.

## In this code

The trap is split across three files.

| File | Contribution |
|---|---|
| `packet.hpp` | defines `struct Packet { int id; ... }` and optionally adds `void* payload` when `EXTRA_FIELD` is defined |
| `provider.cpp` | defines `EXTRA_FIELD` only when `RUN_UNSAFE_EXAMPLE` is set, then returns `sizeof(Packet)` from `provider_packet_size()` |
| `main.cpp` | includes `packet.hpp` without `EXTRA_FIELD` and prints its `sizeof(Packet)` beside `provider_packet_size()` |

- **Safe target** (`Trap27_ODRViolation`) — both translation units see the one-member `Packet`.
- **Unsafe target** (`Trap27_ODRViolation_unsafe`, `RUN_UNSAFE_EXAMPLE`) — `provider.cpp` sees the extra `payload` member, while `main.cpp` does not.

## Why it fails

The unsafe target gives the same class name two different definitions. `main.cpp` compiles a four-byte `Packet`; `provider.cpp` compiles a larger `Packet` with a pointer member. This is IFNDR: the program is ill formed even if the linker accepts it and the executable merely prints two different size constants.

## Correct direction

```cpp
// packet.hpp
struct Packet {
    int id;
    void* payload;
};
static_assert(sizeof(Packet) == expected_packet_size);
```

Put the canonical definition in one configuration-controlled header, and make every target consume the same generated configuration.

## Detection

| Tool | Result |
|---|---|
| Preprocessed source comparison | yes — shows `packet.hpp` expands differently per translation unit |
| Layout/size assertions in every module | yes — catch disagreement close to the boundary |
| Linker | no — it can link one `provider_packet_size()` symbol while the type definitions disagree |
| Sanitizers | no — IFNDR is not generally diagnosed by ASan/UBSan/TSan |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that compares the consumer and provider size constants.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 27.
