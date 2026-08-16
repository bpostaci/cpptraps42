# Trap 23 — Moved-From State

**Rule:** after a successful move, the source object remains valid, but its value is unspecified unless that type documents a stronger postcondition.

**The moved-from-state lesson in this trap is not undefined behavior.** Reading the moved-from `std::string` state and the self-moved `std::vector` size relies on unspecified values, so sanitizer silence is expected; the unsafe target also includes a separate null `unique_ptr` dereference that is undefined behavior.

## The rule

`std::move` does not move anything by itself. It casts an expression so a move constructor or move assignment may steal resources. For standard library types, the moved-from source remains a valid object: it can be destroyed, assigned to, or used by operations with no precondition on its value.

Validity is not the same as a known value. A moved-from `std::string` may be empty in one implementation and not in another. `std::unique_ptr` is different because its contract says the source becomes null after a successful move.

## In this code

`main.cpp` runs three variants from `main()`:

| Function | Unsafe assumption | Safe form |
|---|---|---|
| `moved_from_container` | prints `source` as if it still has the old text | calls `source.clear()` before semantic reuse |
| `moved_from_owner` | dereferences moved-from `source` | uses `destination` and checks `source == nullptr` |
| `self_move` | assigns `values = std::move(values)` and relies on its size | avoids self-move |

- **Safe target** (`Trap23_MovedFrom`) — establishes known state or uses the destination object.
- **Unsafe target** (`Trap23_MovedFrom_unsafe`, `RUN_UNSAFE_EXAMPLE`) — demonstrates unspecified moved-from values and one null owner dereference.

## Why it fails

The moved-from `std::string` and self-moved `std::vector` cases are valid-but-unspecified, not undefined behavior. The defect is making a semantic decision from a value the standard does not promise. The moved-from `std::unique_ptr` case is different: dereferencing the guaranteed-null source is undefined behavior.

## Correct direction

```cpp
std::string source = "payload";
std::string destination = std::move(source);

source.clear();                     // known state before reuse
```

After a move, either stop using the source for its old meaning, assign it a new value, or call an operation that establishes a documented state.

## Detection

| Tool | Result |
|---|---|
| Contract review | required — the key distinction is valid-but-unspecified versus documented-null |
| AddressSanitizer / UndefinedBehaviorSanitizer | no report for unspecified moved-from values; a null dereference may be reported separately |
| Compiler warnings | generally none — moving and then using a valid object is often well formed |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session inspecting the source and destination after each move.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 23.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — questions 16 and 17.
