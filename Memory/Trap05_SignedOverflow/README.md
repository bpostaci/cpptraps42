# Trap 05 — Signed overflow

**Rule:** Check signed arithmetic before evaluating it, because overflowing a signed operation is undefined behavior.

## The rule

Unsigned arithmetic is defined modulo one more than the maximum representable value. Signed arithmetic is different: if addition, multiplication, or negation cannot represent the mathematical result in the destination type, the behavior is undefined.

That distinction matters for optimization. After a signed overflow expression has been evaluated, a later check is too late; the compiler may already assume the overflow path never happens.

## In this code

`main.cpp` first calls `safe_add(19, 23)`, then demonstrates defined unsigned wrap by incrementing an `unsigned` maximum value. The safe target is `Trap05_SignedOverflow`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap05_SignedOverflow_unsafe`.

| Function | Unsafe form | Safe form |
|---|---|---|
| `additive_overflow` | `maximum + 1` where `maximum` is `INT_MAX` | reject with `safe_add` before addition |
| `multiplicative_overflow` | `factor * factor` in `int` | cast to `long long` before multiplying |
| `negation_overflow` | `-minimum` where `minimum` is `INT_MIN` | cast to `long long` before negating |

`ctest -R Safe_SignedOverflow` runs the safe target and checks the root CMake PASS_REGULAR_EXPRESSION for the safe sum and unsigned-wrap line.

## Why it fails

The three variant functions are undefined behavior in the unsafe target. The unsigned increment in `main` is not UB; it is defined modulo arithmetic and is included to make the contrast explicit.

## Correct direction

```cpp
if (auto sum = safe_add(a, b)) {
    use(*sum);
}
const long long product = static_cast<long long>(factor) * factor;
```

Validate before the operation or move the operation into a type that can represent the result. Casting after an overflowing `int` multiplication would still be too late.

## Detection

| Tool | Result |
|---|---|
| Clang/GCC UBSan | reports signed integer overflow at runtime |
| Compiler warnings | catch some constant or obviously bounded cases |
| MSVC AddressSanitizer | no — this is not a memory-addressing error |
| Debugger inspection | shows operands and observed machine result, but does not make UB defined |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session classifying addition, multiplication, negation, and unsigned wrap.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 05.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 5 and practical exercise 1.