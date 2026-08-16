# Trap 06 — Ended Lifetime

**Rule:** a pointer or iterator must not be used after the lifetime of the object it denotes has ended.

## The rule

Object lifetime ends at a specific event: leaving a block for an automatic object, evaluating `delete` for a dynamically allocated object, or invalidating a container element by moving the container's storage. Existing observers are not rewritten when that happens. They may still compare non-null and may still contain the old address.

Dereferencing such an observer attempts to access an object that no longer exists at that location. That is undefined behavior even when the old bytes are still visible, because the language rule is about the live object, not the numeric address.

## In this code

`main.cpp` runs three variants from `main()`:

| Function | Lifetime-ending event | Safe form |
|---|---|---|
| `scope_exit_dangle` | `local` dies at the closing brace | do not dereference `observer` after the block |
| `deleted_heap_dangle` | `delete observer` ends the dynamic `int` lifetime | set `observer = nullptr` and stop using it |
| `reallocation_dangle` | `values.reserve(values.capacity() + 1)` reallocates the vector buffer | re-read through `values.front()` or reacquire a pointer |

- **Safe target** (`Trap06_EndedLifetime`) — avoids every stale dereference.
- **Unsafe target** (`Trap06_EndedLifetime_unsafe`, `RUN_UNSAFE_EXAMPLE`) — dereferences `observer` after scope exit, after `delete`, or after vector reallocation.

## Why it fails

All three unsafe reads are undefined behavior. The stack address, heap address, or old vector buffer address can still be mapped, but no live `int` object is available through that observer. A plausible printed value is only an accident of one run.

## Correct direction

```cpp
std::vector<int> values{1, 2, 3};
values.reserve(values.capacity() + 1);

int current = values.front();       // reacquire through the owner
```

Keep use inside the owner's lifetime. After any lifetime-ending or invalidating operation, discard old observers and reacquire them from the owner.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | reports the heap/vector use-after-free cases, and may report stack-use-after-scope when supported |
| Debug CRT fill bytes | often shows released heap/vector storage as `0xFEEEFEEE` in Debug builds |
| Compiler warnings | generally none — the stale pointer value is well formed |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session comparing old observers with the current owner state.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 06.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 1.
