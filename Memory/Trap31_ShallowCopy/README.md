# Trap 31 — Shallow copy

**Rule:** A class that owns a raw resource must define or disable copying, or delegate ownership to a member that does.

## The rule

If a class declares a destructor but leaves the copy constructor and copy assignment operator implicit, the compiler still generates memberwise copy operations. For a raw owning pointer, memberwise copy duplicates only the pointer value, not the allocation it owns.

That gives two objects the same cleanup responsibility. The later double free is a symptom; the design error is the shallow copy of ownership.

## In this code

`main.cpp` runs three short variants. The safe target is `Trap31_ShallowCopy`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap31_ShallowCopy_unsafe`.

| Function | Class | Meaning |
|---|---|---|
| `shallow_copy_double_free` | `Broken` | unsafe build copies `Broken::data`; safe build skips the broken copy |
| `deep_copy_is_safe` | `RuleOfThree` | copy constructor duplicates the buffer and assignment uses copy-and-swap |
| `rule_of_zero` | `RuleOfZero` | `std::string data_` owns memory, so generated special members are correct |

`ctest -R Safe_DeepCopy` runs the safe target and checks the root CMake PASS_REGULAR_EXPRESSION for the deep-copy output.

## Why it fails

The unsafe `Broken` path has undefined behavior at destruction because both copied objects own the same `char[]` and both destructors call `delete[]`. The copy itself is well formed; the ownership semantics are wrong.

## Correct direction

```cpp
class RuleOfZero {
public:
    explicit RuleOfZero(std::string text) : data_(std::move(text)) {}
private:
    std::string data_;
};
```

Prefer Rule of Zero. If a raw resource is unavoidable, define destructor, copy constructor, and copy assignment together, and add move operations deliberately.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | reports the eventual double free in the unsafe build |
| Debugger pointer comparison | shows `a.data` and `b.data` are identical in the shallow copy |
| Compiler warnings | partial; generated copying is legal unless made unavailable or suspicious |
| Safe `ctest` | proves the repaired target prints the deep-copy line, not that every raw owner is safe |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session comparing shallow pointer identity with Rule-of-Three and Rule-of-Zero repairs.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 31.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 25 and practical exercise 8.