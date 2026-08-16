# Trap 09 — Lambda Capture Lifetime

**Rule:** a closure stores exactly what the capture list says; references and `this` do not become owned snapshots.

## The rule

Lambda capture is a storage decision. Capturing by value copies a value into the closure object. Capturing by reference stores a reference to something outside the closure. Capturing `this` stores the object pointer, not the data members themselves.

That distinction matters when the closure is returned, stored in `std::function`, or invoked later. The closure can outlive a stack local, an object, or an owner that was present when the lambda was created. Calling it after that referent dies is undefined behavior.

## In this code

`main.cpp` calls three variants from `main()`:

| Function | Unsafe capture | Safe capture |
|---|---|---|
| `escaping_reference_capture` | `bad_by_reference()` returns `[&local]` | `good_by_value()` returns `[local]` |
| `this_capture` | `Session::make_reader()` returns `[this]` | returns `[copy = id]` |
| `owned_member_capture` | stores `Session& ref = *owner` and captures `[&ref]` | captures `[owner]` to share ownership |

- **Safe target** (`Trap09_LambdaCapture`) — copies the needed integer or captures the `shared_ptr<Session>` owner.
- **Unsafe target** (`Trap09_LambdaCapture_unsafe`, `RUN_UNSAFE_EXAMPLE`) — invokes callbacks that kept raw references or pointers after the referent died.

## Why it fails

The unsafe callbacks have undefined behavior. The `std::function` object is still alive, but the local `int`, the `Session` object, or the `shared_ptr`-owned object behind the captured reference is not.

## Correct direction

```cpp
struct Session {
    int id{7};
    std::function<int()> make_reader() {
        return [copy = id] { return copy; };
    }
};
```

Capture values for deferred work. If the callback needs an object to remain alive, capture an owning handle such as `std::shared_ptr`, not a borrowed reference.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | can report stack-use-after-scope or heap-use-after-free when the bad callback reads dead storage |
| Debugger closure inspection | yes — shows whether the closure stores an integer, raw pointer, or `shared_ptr` pair |
| ThreadSanitizer | no — this is a lifetime bug, not a data race |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session classifying each closure's stored capture.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 09.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 12.
