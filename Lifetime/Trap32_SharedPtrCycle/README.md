# Trap 32 — `shared_ptr` Cycle

**Rule:** `std::shared_ptr` ownership is reference-counted; a cycle of owning edges keeps every object in the cycle alive forever.

**This trap is not undefined behavior.** The cyclic case is a leak, not an invalid access, so no sanitizer memory error is the expected outcome unless a leak checker is enabled.

## The rule

A `shared_ptr` contributes to a control block's strong count. The managed object is destroyed only when that strong count reaches zero. If object `A` owns object `B` and object `B` owns object `A`, dropping the local variables removes only the outside owners; the internal owners keep the counts above zero.

`std::weak_ptr` is the non-owning companion. It observes a control block without increasing the strong count, and `lock()` is the explicit check that temporarily creates a strong owner only when the object is still alive.

## In this code

`main.cpp` calls three variants from `main()`:

| Function | Ownership shape | Result |
|---|---|---|
| `shared_ptr_cycle_leaks` | `CyclicNode::peer` is a `shared_ptr` in both directions | destructors for the cyclic nodes never run |
| `weak_ptr_breaks_cycle` | `WeakNode::next` owns forward and `WeakNode::prev` observes backward | scope exit destroys both nodes |
| `locking_a_weak_ptr` | `observer` watches one `WeakNode` and calls `lock()` before use | after the owner scope, `observer.expired()` is true |

- **Safe target** (`Trap32_SharedPtrCycle`) — contains the leaking cycle and the corrected weak-edge forms directly.
- There is no `_unsafe` target — the source contains no `RUN_UNSAFE_EXAMPLE` branch.

## Why it fails

The cyclic form is defined-but-wrong. Each node owns the other, so both reference counts remain nonzero after the local `shared_ptr`s are destroyed. No destructor runs for the cycle, and the objects leak.

## Correct direction

```cpp
struct WeakNode {
    std::shared_ptr<WeakNode> next;
    std::weak_ptr<WeakNode> prev;
};
```

Make at least one edge in every ownership cycle non-owning. Lock a `weak_ptr` only for the short section that needs a live object.

## Detection

| Tool | Result |
|---|---|
| `use_count()` / destructor logging | yes — counts stay above zero and cyclic destructors do not run |
| `ctest -R Safe_SharedPtrCycle` | asserts `expired: true` for the weak observer variant |
| AddressSanitizer | no memory error — the objects are still owned by the cycle |
| Leak checker | reports the leaked cyclic nodes when leak detection is enabled |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session following strong counts through the cycle and weak edge.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 32.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 26 and practical exercise 9.
