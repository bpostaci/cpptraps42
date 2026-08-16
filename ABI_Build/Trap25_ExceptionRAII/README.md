# Trap 25 — Exception RAII

**Rule:** cleanup that must run on every exit path belongs in a destructor, not after the operation that may throw.

**This trap is not undefined behavior.** A skipped manual cleanup is a resource defect; sanitizer silence is expected because the language unwinding rules are working as designed.

## The rule

When an exception is thrown, ordinary statements after the throw are skipped until a matching handler is found. During that unwinding, destructors for fully constructed automatic objects are called in reverse construction order. RAII uses that rule by putting resource release in the destructor of an object whose lifetime is tied to the scope.

Manual `lock(); throw; unlock();` is wrong because the unlock statement is just another statement that can be bypassed. `std::scoped_lock` is right because unlocking is part of object destruction.

## In this code

`main.cpp` creates the safe target, `Trap25_ExceptionRAII`. It creates `std::mutex m`, then enters a `try` block with `std::scoped_lock lock(m)`. It throws `std::runtime_error("failure")`, unwinds the lock, and the `catch(...)` prints `recovered and unlocked`.

| Entity | Role |
|---|---|
| `std::mutex m` | resource being protected |
| `std::scoped_lock lock(m)` | RAII owner of the lock |
| `throw std::runtime_error("failure")` | non-local exit |
| `catch(...)` | resumes after the lock destructor has run |

There is no `Trap25_ExceptionRAII_unsafe` target because no source file contains `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The anti-pattern fails when cleanup is manual and appears after a throwing operation. That is a resource leak or stuck-lock defect, not undefined behavior by itself. The real source demonstrates the corrected form directly: the destructor runs during unwinding, so the mutex is unlocked before control reaches the handler.

## Correct direction

```cpp
std::mutex m;
try {
    std::scoped_lock lock(m);
    may_throw();
} catch (...) {
    recover();
}
```

Use RAII wrappers for every resource: locks, files, allocations, handles, transactions, and temporary state changes.

## Detection

| Tool | Result |
|---|---|
| Exception breakpoints / debugger stack | yes — shows unwinding enters `std::scoped_lock` destructor |
| Static review | yes — flags cleanup statements placed after throwing work |
| Sanitizers | no — a skipped unlock is not a memory, race, or undefined-behavior report |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that follows construction, throw, destructor, and unlock.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 25.
