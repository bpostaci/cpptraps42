# Trap 20 — Data Race

**Rule:** every shared object that can be accessed by more than one thread must have all conflicting accesses ordered by atomics or synchronization.

## The rule

A data race occurs when two threads access the same memory location concurrently, at least one access modifies it, and there is no happens-before relationship between the accesses. In C++, a data race is undefined behavior. The problem is not only a lost update; once the program has a data race, the optimizer may assume the racy execution does not exist.

`std::atomic` makes the counter operation indivisible. `memory_order_relaxed` is enough for this specific counter because no other payload is being published through the counter; the only invariant is that every increment contributes to one final numeric total.

## In this code

`main.cpp` starts two `std::jthread` workers and joins both before printing `counter`.

| Target | Counter type | Worker operation |
|---|---|---|
| `Trap20_DataRace` | `std::atomic<int> counter{0}` | `counter.fetch_add(1, std::memory_order_relaxed)` |
| `Trap20_DataRace_unsafe` (`RUN_UNSAFE_EXAMPLE`) | `int counter = 0` | `++counter` |

The safe target has a CTest assertion: `ctest -R Safe_AtomicCounter` expects the final count to contain `200000`.

## Why it fails

`++counter` on a plain `int` is a read-modify-write sequence. When both worker threads execute it without synchronization, their reads and writes conflict. That is undefined behavior, even if a particular run merely looks like a smaller final count.

## Correct direction

```cpp
std::atomic<int> counter{0};
auto work = [&] {
    for (int i = 0; i < 100000; ++i)
        counter.fetch_add(1, std::memory_order_relaxed);
};
```

Use a mutex instead when the operation protects a larger invariant than one atomic integer.

## Detection

| Tool | Result |
|---|---|
| ThreadSanitizer | yes — reports the conflicting unsynchronized accesses in the unsafe target |
| MSVC AddressSanitizer | no — MSVC supports ASan, not TSan or MSan, and this is not an address error |
| CDB / WinDbg | shows both threads writing the same address, but breakpoints can serialize the race |
| `ctest -R Safe_AtomicCounter` | asserts the corrected atomic path prints `200000` |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that contrasts atomic writes with plain racy writes.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 20.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 20.
