# Trap 12 — Object Slicing

**Rule:** copying a derived object into a base object creates a new standalone base object; the derived part is not copied into the destination.

**This trap is not undefined behavior.** Slicing is a well-defined copy of the base subobject, so sanitizer silence is the expected result.

## The rule

A `Base` object has exactly the state and dynamic type of `Base`. When a `Derived` is used to initialize a `Base` by value, only the `Base` subobject participates in that copy. Any derived state, override identity, or invariant outside the base subobject is not part of the destination.

Polymorphism requires indirection: a reference, pointer, or owning handle to a base subobject. Value semantics of the base type deliberately produce base values.

## In this code

`main.cpp` runs three variants:

| Function | Slicing form | Correct form |
|---|---|---|
| `copy_slicing` | `Base sliced = d;` then `sliced.type()` | `const Base& polymorphic = d;` |
| `container_slicing` | `_unsafe`: `std::vector<Base>` and `push_back(Derived{})` | `std::vector<std::unique_ptr<Base>>` |
| `parameter_slicing` | `_unsafe`: `print_by_value(Base b)` | `print_by_reference(const Base& b)` |

- **Safe target** (`Trap12_ObjectSlicing`) — keeps polymorphic objects behind references or owning pointers where needed.
- **Unsafe target** (`Trap12_ObjectSlicing_unsafe`, `RUN_UNSAFE_EXAMPLE`) — shows the same defined slicing in container and parameter forms.

## Why it fails

The category is defined-but-wrong behavior. The program is not corrupt; it simply asked for a `Base` value and got one. Dynamic dispatch then calls `Base::type()` because the destination object's dynamic type is `Base`.

## Correct direction

```cpp
void print_by_reference(const Base& b) {
    b.type();
}

std::vector<std::unique_ptr<Base>> values;
values.push_back(std::make_unique<Derived>());
```

Pass polymorphic objects by reference or pointer. Store polymorphic objects through owning handles rather than by value in a base-typed container.

## Detection

| Tool | Result |
|---|---|
| Debugger dynamic-type / vftable inspection | shows the copied object has `Base` dynamic type |
| Code review | effective when looking for base-by-value parameters and containers |
| ASan / UBSan | no report, because slicing is a valid copy |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session comparing the sliced object and the referenced derived object.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 12.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 13 and practical exercise 4.
