# Trap 19 — Double free

**Rule:** Each dynamically allocated resource must have exactly one owning release path.

## The rule

A deallocation consumes the ownership right for that allocation. Raw pointers are only numeric values, so deleting through one pointer does not clear aliases, and constructing two owners from the same raw pointer does not create shared ownership.

The allocator often reports the second release, but the bug begins earlier: when the program duplicates ownership or keeps using a pointer after its ownership has been spent.

## In this code

`main.cpp` runs three short variants. The safe target is `Trap19_DoubleFree`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap19_DoubleFree_unsafe`.

| Function | Unsafe form | Safe form |
|---|---|---|
| `explicit_double_delete` | `delete p; delete p;` on the same raw pointer | `std::unique_ptr<int>` with one `reset()` |
| `duplicated_ownership` | constructs `first(raw)` and `second(raw)` | moves ownership from `first` to `second` |
| `shallow_copy_double_free` | unsafe `Buffer` copy duplicates `data` | safe build deep-copies `Buffer::data` |

## Why it fails

The unsafe variants are undefined behavior. The same allocation is released twice, either explicitly, through two `unique_ptr` destructors that both believe they are exclusive owners, or through two `Buffer` destructors after a shallow copy.

## Correct direction

```cpp
auto first = std::make_unique<int>(7);
auto second = std::move(first);
if (second) { use(*second); }
```

Make the ownership transfer visible in the type system. For classes, prefer Rule of Zero members or implement copying as a deep copy.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | reports many double frees with allocation and first-free context |
| Debug CRT heap checks | can show freed-memory patterns or stop at the second release |
| Compiler | prevents some cases when ownership is represented with non-copyable types |
| Release build | may be silent until allocator metadata is damaged later |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session locating duplicate owner addresses before the allocator failure.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 19.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 15 and practical exercise 5.
