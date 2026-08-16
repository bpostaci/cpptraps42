# Trap 22 — Volatile Is Not Synchronization

**Rule:** `volatile` is not a thread-synchronization primitive; use atomics or locks to communicate between threads.

## The rule

In C++, `volatile` is about observable accesses to special memory, not inter-thread ordering. It does not make an operation atomic, does not create a happens-before edge, and does not publish ordinary payload writes to another thread. A `volatile bool` flag can still be read and written by different threads without synchronization.

The usual producer/consumer pattern needs a release operation in the writer and an acquire operation in the reader. The release makes earlier writes visible to a matching acquire; the flag is not merely a polling variable but the synchronization edge.

## In this code

`main.cpp` shares `payload` and a readiness flag between a writer `std::jthread` and a reader `std::jthread`.

| Target | Flag | Payload guarantee |
|---|---|---|
| `Trap22_VolatileIsNotSync` | `std::atomic<bool> ready` | `store(..., memory_order_release)` pairs with `load(..., memory_order_acquire)` |
| `Trap22_VolatileIsNotSync_unsafe` (`RUN_UNSAFE_EXAMPLE`) | `volatile bool ready` | no synchronization; `payload` is still racy |

The unsafe target may appear to print the intended value, but that is only one schedule and one implementation result.

## Why it fails

The writer stores `payload = 42` and then writes `ready`; the reader spins on `ready` and then reads `payload`. With `volatile`, those operations are not ordered across threads. The read and write of `payload` are conflicting unsynchronized accesses, so the unsafe target has a data race and therefore undefined behavior.

## Correct direction

```cpp
std::atomic<bool> ready{false};
writer: payload = 42;
ready.store(true, std::memory_order_release);

reader: while (!ready.load(std::memory_order_acquire)) { }
use(payload);
```

Use a mutex and condition variable when the payload is more than a simple one-shot publication.

## Detection

| Tool | Result |
|---|---|
| ThreadSanitizer | yes — reports the racy `payload` access in the unsafe target |
| MSVC AddressSanitizer | no — MSVC has ASan but not TSan or MSan, and the memory is in bounds |
| CDB / WinDbg | can show the missing acquire/release protocol, but it is not a race detector |
| Compiler warnings | generally none; `volatile` is legal syntax with the wrong contract |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that compares the release/acquire path with the volatile path.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 22.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 21.
