# Trap 30 — Unsequenced Modification

**Rule:** do not modify the same scalar object more than once in one full-expression unless the modifications are sequenced.

## The rule

C++ specifies sequencing relationships between evaluations, not a universal left-to-right evaluation order. If two side effects on the same scalar object are unsequenced relative to each other, or one side effect is unsequenced relative to a value computation of that same object, the behavior is undefined.

Post-increment and pre-increment each modify their operand. Combining them in a larger expression does not automatically choose a portable order.

## In this code

`main.cpp` starts with `int value = 1`.

| Target | Code path | Meaning |
|---|---|---|
| `Trap30_UnsequencedModification` | stores `old`, increments `value`, stores `after_first_increment`, increments again, then assigns `old + after_first_increment` | every state transition is sequenced |
| `Trap30_UnsequencedModification_unsafe` | `value = value++ + ++value;` | multiple unsequenced modifications of `value` |

The `_unsafe` target exists because the source contains `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The unsafe branch has undefined behavior. There is no standard value for the expression, even if one compiler and build prints a stable number. The emitted instruction order is an implementation artifact, not a meaning assigned by C++.

## Correct direction

```cpp
const int old = value;
++value;
const int after_first_increment = value;
++value;
value = old + after_first_increment;
```

Give each mutation its own statement when the intermediate states matter. This makes the sequencing relationship explicit and reviewable.

## Detection

| Tool | Result |
|---|---|
| Clang/GCC warnings such as `-Wunsequenced` | often diagnose the compact expression |
| Code review | reliable when looking for repeated `++`, `--`, or assignments to the same scalar in one expression |
| UBSan | not a dependable detector for all unsequenced side effects |
| MSVC `/W4` in the debug analysis | did not warn for this source |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session separating the defined statements from one emitted unsafe order.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 30.
