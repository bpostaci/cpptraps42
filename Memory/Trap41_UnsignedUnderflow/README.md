# Trap 41 — Unsigned underflow

**Rule:** Do not express possibly negative counts or indices in an unsigned type and then expect them to become negative.

**This trap is not undefined behavior.** Unsigned wrap and signed-to-unsigned conversion are defined modular arithmetic, so sanitizer silence is the expected, correct outcome.

## The rule

Unsigned integer arithmetic is performed modulo one more than the maximum value of the type. For `std::size_t`, subtracting one from zero produces the maximum `std::size_t` value, not `-1`.

The usual arithmetic conversions also matter. When a signed negative value is compared with an unsigned value of at least the same rank, the signed value is converted to unsigned first, which can turn `-1` into a huge value.

## In this code

`main.cpp` runs three short variants. There is no `_unsafe` target because the source does not contain `RUN_UNSAFE_EXAMPLE`; the trap shows safe forms and wrong commented forms directly in `Trap41_UnsignedUnderflow`.

| Function | Wrong invariant | Safe form |
|---|---|---|
| `size_minus_one_on_empty` | `empty.size() - 1` should mean no last element | guard with `empty.empty()` before indexing |
| `reverse_loop_never_ends` | `i >= 0` can stop an unsigned reverse loop | use `for (std::size_t i = v.size(); i-- > 0;)` or reverse iterators |
| `signed_unsigned_comparison` | `-1 < 3u` behaves like signed comparison | compare in a chosen signed domain or use `std::ssize` |

`ctest -R Safe_UnsignedGuard` runs this target and checks the root CMake PASS_REGULAR_EXPRESSION for the empty-container guard.

## Why it fails

The category is defined-but-wrong behavior. The arithmetic and conversions are specified, but the resulting huge value or false comparison violates the range invariant the code intended.

## Correct direction

```cpp
if (!v.empty()) {
    use(v.back());
}
for (auto it = v.rbegin(); it != v.rend(); ++it) {
    use(*it);
}
```

Guard the container state before subtracting, use reverse iterators when possible, and use `std::ssize` when signed indexing is the clearest expression of the invariant.

## Detection

| Tool | Result |
|---|---|
| Assertions on invariants | yes; check `!empty()` before computing a last index |
| Compiler warnings | can report always-true unsigned comparisons or signed/unsigned mixes |
| AddressSanitizer / UBSan | no — unsigned wrap and conversion are defined; no bad access occurs in the safe code |
| Debugger/watch window | useful for seeing the maximum `std::size_t` value, but it is not a sanitizer failure |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session inspecting wrap, reverse-loop control, and signed/unsigned conversion.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 41.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — questions 6 and 35 and practical exercise 12.