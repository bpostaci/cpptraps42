# Trap 15 — Pointer Provenance

**Rule:** a numeric address is not enough to prove that a pointer may access a live object.

## The rule

C++ pointer validity carries more information than the integer value printed for the address. The access must still be within the correct object's lifetime, bounds, alignment, and permitted typed-access rules. Converting a pointer to an integer and back is a platform boundary operation; it does not preserve ownership or extend lifetime.

That distinction matters most when the original owner changes. A reconstructed pointer can compare equal to the old address while no live object remains there.

## In this code

`main.cpp` creates `auto owner = std::make_unique<int>(42)` and saves `int* original = owner.get()`.

| Target | Code path | Meaning |
|---|---|---|
| `Trap15_PointerProvenance` | `int* observer = original;` then reads while `owner` is alive | valid observation of a live allocation |
| `Trap15_PointerProvenance_unsafe` | stores `original` in `std::uintptr_t`, reconstructs `int*`, calls `owner.reset()`, then dereferences | numeric address survives, object lifetime does not |

The `_unsafe` target exists because the source contains `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The unsafe branch has undefined behavior. After `owner.reset()`, the `int` lifetime has ended and the allocation has been released. `reconstructed` may still hold the same numeric address, but it no longer denotes a live `int` that the program may read.

## Correct direction

```cpp
auto owner = std::make_unique<int>(42);
int* observer = owner.get();
std::cout << *observer << '\n';  // owner is still alive
```

Keep the owner alive for the full observation window. If access must outlive a scope, pass ownership or a validated weak/shared handle instead of an address-shaped integer.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | reports this particular unsafe branch as use-after-free |
| Ownership/lifetime review | required for the general provenance question |
| Debugger numeric address comparison | can show equality, but cannot prove validity |
| Compiler warnings | generally none; the casts are syntactically valid |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session comparing the live owner path with the reconstructed pointer path.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 15.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 1.
