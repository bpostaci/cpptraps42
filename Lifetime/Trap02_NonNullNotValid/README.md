# Trap 02 — Non-null Is Not Valid

**Rule:** a stored address is not proof that an object is still alive; liveness must come from an owner or a checked weak observation.

**This trap is not undefined behavior.** The program uses `std::weak_ptr::lock()` and never dereferences a dead object, so sanitizer silence is expected.

## The rule

Pointer-like values and object lifetime are separate facts. A raw pointer, and even the implementation fields inside a `weak_ptr`, can retain a non-null numeric address after the last owning reference has released the object. That address is only historical evidence.

For `std::shared_ptr`, the strong count controls the owned object's lifetime. A `std::weak_ptr` observes the same control block, but it does not keep the object alive. The only valid way to convert that observation into a usable object is `lock()`, which either creates a new `shared_ptr` or returns empty.

## In this code

`main.cpp` creates `owner`, copies it into `observer`, then calls `owner.reset()` before asking the observer for a `snapshot`.

| Name | Type | Meaning |
|---|---|---|
| `owner` | `std::shared_ptr<int>` | the only strong owner of `42` |
| `observer` | `std::weak_ptr<int>` | a non-owning observation of the control block |
| `snapshot` | `std::shared_ptr<int>` | the result of `observer.lock()` |

- **Safe target** (`Trap02_NonNullNotValid`) — checks `observer.lock()` and prints the no-live-object branch when the snapshot is empty.
- There is no `_unsafe` target — the source contains no `RUN_UNSAFE_EXAMPLE` branch, and this trap shows the correct rule directly.

## Why it fails

The wrong reasoning is defined-but-wrong: treating a non-null stored address as a lifetime proof. After `owner.reset()`, the `int` lifetime has ended even if `observer` still contains implementation pointers that look meaningful in a debugger.

## Correct direction

```cpp
std::weak_ptr<int> observer = owner;

if (auto snapshot = observer.lock()) {
    std::cout << *snapshot << '\n';
}
```

Use the snapshot, not the observer's stored address, as the proof. If `lock()` fails, there is no live object to read.

## Detection

| Tool | Result |
|---|---|
| Ownership trace / debugger | yes — compare `owner` becoming empty with `observer.lock()` returning an empty `snapshot` |
| AddressSanitizer | no — the safe target performs no invalid access |
| Compiler warnings | none — weak observation and failed lock are valid C++ |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing the observer's address fields are not liveness evidence.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 02.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 1.
