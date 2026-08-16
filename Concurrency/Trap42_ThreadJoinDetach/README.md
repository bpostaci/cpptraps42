# Trap 42 — Thread Join or Detach

**Rule:** a `std::thread` object must be non-joinable before its destructor runs.

**This trap is not undefined behavior.** Destroying a joinable `std::thread` is defined to call `std::terminate`, so sanitizer silence is expected; the program aborts by mandate.

## The rule

`std::thread` is a handle to an executing thread. If the handle is still joinable at destruction, the standard does not guess whether you meant to join, detach, or abandon shared state. It requires `std::terminate`.

Joining is also an ownership and lifetime rule. A detached thread must not refer to stack data that can disappear before the thread finishes. `std::jthread` is the safer default for scope-bound work because it joins in its destructor and participates in RAII.

## In this code

`main.cpp` runs three named variants.

| Function | Demonstration | Safe behavior |
|---|---|---|
| `destroyed_while_joinable` | unsafe branch lets `std::thread t{worker, 1}` reach destruction joinable | safe branch prints the skip note and calls `t.join()` |
| `exception_skips_manual_join` | an exception after starting work would skip manual cleanup | `std::jthread guarded{worker, 2}` joins during unwinding |
| `detach_outlives_its_data` | reference capture would be dangerous if detached | joins before `local` dies, then uses `std::vector<std::jthread>` with value capture |

- **Safe target** (`Trap42_ThreadJoinDetach`) — joins or uses `std::jthread`.
- **Unsafe target** (`Trap42_ThreadJoinDetach_unsafe`, `RUN_UNSAFE_EXAMPLE`) — aborts in the first variant.

The safe target has a CTest assertion: `ctest -R Safe_ThreadJoin` expects `pool size=3`.

## Why it fails

This is defined-but-dangerous behavior: `~std::thread` sees `joinable() == true` and calls `std::terminate`. The detached-lifetime case is a separate lifetime hazard: if a detached thread reads a reference after the owning scope ends, that later access can become undefined behavior.

## Correct direction

```cpp
std::jthread t{worker, id};        // joins at scope exit

std::thread manual{worker, id};
manual.join();                     // or detach only with an explicit lifetime contract
```

Prefer `std::jthread` for scoped worker ownership. Detach only when the data is owned independently of the launching scope.

## Detection

| Tool | Result |
|---|---|
| Runtime / debugger | yes — the unsafe target aborts through `std::terminate` |
| `ctest -R Safe_ThreadJoin` | asserts the RAII pool path reaches `pool size=3` |
| Sanitizers | no — mandated termination is not a memory or race violation |
| Code review | checks every control path for join, detach, or `std::jthread` ownership |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that follows the destructor and unwinding paths.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 42.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 36.
