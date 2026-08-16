# Trap 04 — Out-of-bounds access

**Rule:** A one-past address may be formed as a sentinel, but it must not be dereferenced.

## The rule

Array and container subscripting is governed by the valid element range, not by whether address arithmetic can produce a numeric pointer. For an array of four elements, indices `0` through `3` name elements; index `4` is one past the end and is only usable for comparison or as an iterator sentinel.

Passing an array to a function as `const int*` loses the extent. Once the parameter is only a pointer, `sizeof(data)` measures the pointer object, not the original array.

## In this code

`main.cpp` runs three variants. The safe target is `Trap04_OutOfBounds`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap04_OutOfBounds_unsafe`.

| Function | Unsafe form | Safe form |
|---|---|---|
| `unchecked_subscript` | reads `values[index]` with `index == 4` | checks `index < values.size()` |
| `off_by_one_loop` | loops with `i <= values.size()` | loops with `i < values.size()` |
| `decayed_array_size` | computes a count from `sizeof(data)` | refuses to infer the bound; use `std::span` or pass the size |

## Why it fails

The first two variants are undefined behavior because they read outside the `std::array` elements. The third variant demonstrates a defined calculation that can become an out-of-bounds bug when the recovered count is trusted.

## Correct direction

```cpp
void sum_values(std::span<const int> values) {
    int sum = 0;
    for (int value : values) { sum += value; }
}
```

Carry the extent with the data. `std::array`, `std::vector`, and `std::span` make the valid range explicit; `operator[]` still requires you to respect it.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | usually reports the illegal read when the access crosses poisoned bounds |
| MSVC debug STL | reports `std::array` subscript violations in Debug builds |
| Compiler warnings | sometimes catch constant off-by-one cases, but not general runtime indices |
| Release build | often silent; the read may return adjacent storage and appear plausible |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session separating one-past address formation from legal dereference.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 04.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — questions 3 and 4.
