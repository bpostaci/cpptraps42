# Trap 40 — Float comparison

**Rule:** Compare floating-point values according to the numeric invariant, not by assuming decimal-looking values are exact.

**This trap is not undefined behavior.** The comparisons and NaN behavior are defined floating-point semantics, so sanitizer silence is the expected, correct outcome.

## The rule

Most decimal fractions, including `0.1`, `0.2`, and `0.3`, are not exactly representable as binary floating-point values. Arithmetic rounds to nearby representable values, so the stored result of `0.1 + 0.2` can differ from the stored literal `0.3`.

A tolerance must fit the scale of the values being compared. NaN is a separate rule: it is unordered and compares unequal to every value, including itself.

## In this code

`main.cpp` runs three short variants. There is no `_unsafe` target because the source does not contain `RUN_UNSAFE_EXAMPLE`; the trap shows correct rules and wrong equality assumptions directly in `Trap40_FloatComparison`.

| Function | Demonstration | Correct idea |
|---|---|---|
| `exact_equality_fails` | prints `sum` for `0.1 + 0.2` and compares it with `0.3` | inspect or tolerate representation error |
| `tolerant_comparison` | calls `nearly_equal` for small and large magnitudes | combine absolute and relative tolerance |
| `accumulation_and_nan` | compares repeated accumulation with `1.0`, then tests `nan_value` | use integer loop counters and `std::isnan` |

`ctest -R Safe_FloatTolerance` runs this target and checks the root CMake PASS_REGULAR_EXPRESSION for the NaN comparison line.

## Why it fails

The category is defined-but-wrong behavior. The program is not corrupt; the exact comparison encodes a false numeric invariant, and `nan_value == nan_value` is specified to be false.

## Correct direction

```cpp
if (nearly_equal(measured, expected)) {
    accept();
}
if (std::isnan(value)) {
    handle_missing_number();
}
```

Use a domain-appropriate tolerance, and treat non-finite values explicitly. For loop counts, keep the controlling variable integral when possible.

## Detection

| Tool | Result |
|---|---|
| High-precision printing / debugger bits | yes; shows adjacent or rounded representable values |
| Unit tests with boundary cases | yes; large magnitudes and NaN reveal bad equality assumptions |
| AddressSanitizer / UBSan | no — the arithmetic and comparisons are defined |
| Compiler warnings | usually no; exact comparison can be intentional in some domains |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session inspecting stored double bits, tolerance math, accumulation, and NaN.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 40.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 34.