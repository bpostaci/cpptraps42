# Trap 39 — Member Init Order

**Rule:** bases are initialized first, then members in declaration order, regardless of the order written in the constructor's init-list.

## The rule

The member-initializer list does not define construction order. It supplies initializers that are applied in the fixed order determined by the class definition: virtual bases, direct bases, then non-static data members in declaration order.

That rule keeps destruction order well-defined, but it makes dependencies between members dangerous. A member initializer must not read a member declared later, because that later member has not been initialized yet.

## In this code

`main.cpp` runs three variants:

| Function | Demonstrated form | Meaning |
|---|---|---|
| `init_list_order_is_a_lie` | `_unsafe`: `Reordered` declares `doubled` before `count` but initializes `doubled(count * 2)` | reads an indeterminate `count` |
| `safe_dependent_members` | `Ordered` declares `count` before `doubled`; `BodyComputed` assigns in the body | dependencies are sequenced safely |
| `bases_precede_members` | `View : Buffer` initializes `Buffer(n)` before `size(storage.size())` | base subobject is alive before members |

- **Safe target** (`Trap39_MemberInitOrder`) — skips the UB `Reordered` construction and runs the safe forms.
- **Unsafe target** (`Trap39_MemberInitOrder_unsafe`, `RUN_UNSAFE_EXAMPLE`) — constructs `Reordered r{21}` and reads `count` before initialization.

## Why it fails

The unsafe branch has undefined behavior. `doubled` is initialized first because it is declared first, and its initializer reads `count` while `count` still has an indeterminate value.

## Correct direction

```cpp
struct Ordered {
    int count;
    int doubled;
    explicit Ordered(int value) : count(value), doubled(count * 2) {}
};
```

Declare members in dependency order, or compute dependent values in the constructor body after all members have been initialized.

## Detection

| Tool | Result |
|---|---|
| `ctest -R Safe_MemberInitOrder` | checks that `Ordered` has count 21 and doubled 42 |
| Clang/GCC `-Wreorder` and related warnings | can flag init-list order or dependency mistakes |
| MemorySanitizer | can detect the indeterminate read in suitable builds |
| ASan | no; the read is in bounds and from live storage, but the value is indeterminate |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing constructor order and the indeterminate read.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 39.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 33 and practical exercise 11.
