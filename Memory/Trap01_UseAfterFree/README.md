# Trap 01 — Use After Free

**Rule:** a pointer value and an object lifetime are independent facts. Releasing the object does not change the pointers that still hold its address.

## The rule

An object's lifetime ends when its storage is deallocated. Every pointer that held its address becomes invalid at that moment, but nothing writes to those pointers — they keep the same numeric value they had before. Accessing an object through such a pointer is undefined behavior, so the compiler is free to assume it never happens, and the program may crash, may print plausible data, or may change behavior between Debug and Release.

The invalid pointer is a *consequence*. The defect is that a non-owning observer outlived the owner.

## In this code

`main.cpp` holds one `Widget` through two names:

| Name | Type | Owns the object |
|---|---|---|
| `owner` | `std::unique_ptr<Widget>` | yes |
| `observer` | raw `Widget*` | no |

- **Safe target** (`Trap01_UseAfterFree`) — reads through `observer` only while `owner` is alive, then calls `owner.reset()` and stops.
- **Unsafe target** (`Trap01_UseAfterFree_unsafe`, `RUN_UNSAFE_EXAMPLE`) — repeats the read *after* `reset()`.

Note that the safe target's memory is freed at the same point. It is not safe because the memory is in a better state; it is safe because nobody reads it.

## Why it fails

`reset()` runs `~Widget` and returns the storage to the allocator. `observer` is not notified and is not cleared. The subsequent read is a heap use-after-free: the storage may have been reused, may hold allocator bookkeeping, or may still contain the old bytes — which is the worst case, because the program then looks correct and the tests pass.

## Correct direction

Make ownership explicit instead of tracking it by convention:

```cpp
auto owner = std::make_shared<Widget>();
std::weak_ptr<Widget> observer = owner;      // observation that can be validated
if (auto locked = observer.lock()) { use(*locked); }
```

Or keep the raw observer, but bound its use to a scope where the owner is provably alive — and never store it.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | reports it precisely, with the allocation and free stacks |
| Debug CRT fill bytes (`0xFEEEFEEE`) | visible as evidence in a debugger, Debug builds only |
| Release build | typically silent; the read returns whatever was left behind |
| Compiler warnings | none — the code is well formed; the defect is in the lifetime |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that measures the two pointers diverging.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 01.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 1.
