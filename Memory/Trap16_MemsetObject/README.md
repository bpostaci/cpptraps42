# Trap 16 — `memset` object

**Rule:** Byte writes are not class operations; use the type's constructor, assignment, or member functions to change object state.

## The rule

A live class object has invariants that are maintained by its constructors, destructors, assignment operators, and member functions. Writing raw zero bytes across the complete object representation bypasses those operations and overwrites private representation fields the program is not allowed to manage by convention.

For trivially copyable byte-oriented data this can be intentional. For `std::string`, zeroing the object is not the same operation as assigning an empty string or calling `clear()`.

## In this code

`main.cpp` creates `std::string text = "owned characters"`, then either clears it correctly or overwrites its representation. The safe target is `Trap16_MemsetObject`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap16_MemsetObject_unsafe`.

| Target | Operation | Meaning |
|---|---|---|
| `Trap16_MemsetObject` | `text.clear()` | type-aware operation preserves the string invariant |
| `Trap16_MemsetObject_unsafe` | `std::memset(&text, 0, sizeof text)` | raw byte write overwrites every implementation field |

## Why it fails

The unsafe branch has undefined behavior because it corrupts the representation of a live `std::string` object outside the class contract. The program may still print a size that looks reasonable, but a plausible observation is not a valid invariant.

## Correct direction

```cpp
std::string text = "owned characters";
text.clear();
text = {};
```

Let the class perform the state transition. Reserve `std::memset` for raw storage or trivial byte buffers where the representation operation is the actual intent.

## Detection

| Tool | Result |
|---|---|
| Debugger byte inspection | shows that `memset` changed fields `clear()` preserved |
| AddressSanitizer | usually no; the write is in bounds and the object storage is alive |
| Compiler warnings | usually no; `std::memset` accepts `void*` and the call is syntactically valid |
| Runtime output | not reliable; the object can look empty while its invariant is broken |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session comparing `std::string` representation before and after `clear()` and `memset`.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 16.
