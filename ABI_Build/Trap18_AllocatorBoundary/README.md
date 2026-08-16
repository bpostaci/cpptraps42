# Trap 18 — Allocator Boundary

**Rule:** memory must be released by the same allocator family, module contract, or exported destroy function that owns it.

**This trap is not an active undefined-behavior demo.** The repository target uses the correct `malloc`/`free` pairing; sanitizer silence is expected unless a real mismatched release is introduced.

## The rule

Allocation is a protocol, not just a pointer value. A pointer returned by `std::malloc` must be released with `std::free`; memory returned by `new` must be released with the matching `delete`; memory owned by a DLL or plugin must be returned through that boundary's documented destroy API unless the contract says the caller owns the buffer.

Across binary boundaries, "same type" is not enough. Different CRTs, heaps, allocators, and ownership rules can make a release operation invalid even when the address looks ordinary in the debugger.

## In this code

`main.cpp` contains one safe target, `Trap18_AllocatorBoundary`, and no guarded unsafe branch.

| Entity | Role |
|---|---|
| `std::malloc(256)` | allocates from the C heap |
| `FreeDeleter::operator()` | calls `std::free(p)` |
| `std::unique_ptr<void, FreeDeleter> buffer` | stores the pointer and its matching release policy |

There is no `Trap18_AllocatorBoundary_unsafe` target because no source file contains `RUN_UNSAFE_EXAMPLE`. The trap demonstrates the correct boundary pattern directly.

## Why it fails

The failure category in real code is an allocator-contract violation, often undefined behavior at the deallocation call. The pointer value does not encode which allocator owns it. If the caller guesses the wrong release family or releases memory owned by another module, heap metadata can be corrupted far from the original allocation.

## Correct direction

```cpp
struct FreeDeleter {
    void operator()(void* p) const { std::free(p); }
};

std::unique_ptr<void, FreeDeleter> buffer(std::malloc(256));
```

For DLL APIs, export `destroy_widget(Widget*)` or require caller-provided storage so ownership never crosses the boundary ambiguously.

## Detection

| Tool | Result |
|---|---|
| Heap diagnostics / debug CRT | may report a mismatched or invalid heap release in a real bad case |
| AddressSanitizer | sometimes catches allocator-family mismatches, but not every DLL/CRT contract problem |
| Linker | no — the pointer type does not carry allocator ownership |
| CDB / WinDbg | can prove the safe target releases through `FreeDeleter` to `std::free` |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that follows the owner and deleter path.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 18.
