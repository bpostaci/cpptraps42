# Trap 03 — Uninitialized memory

**Rule:** An object must have a value established before any read observes it.

## The rule

Default-initialization does not mean zero-initialization for automatic scalar objects or for objects with scalar members and no user-provided constructor. Until an initialization or assignment establishes a value, reading such an object observes an indeterminate value and has undefined behavior for these `int` examples.

Value-initialization is different. Braces such as `int count{}`, `new Point{}`, and the omitted members in `Config c{3}` establish zero values for the scalar subobjects.

## In this code

`main.cpp` runs three short variants. The safe target is `Trap03_UninitializedMemory`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap03_UninitializedMemory_unsafe`.

| Function | Unsafe form | Safe form |
|---|---|---|
| `uninitialized_local` | reads `count` after `int count;` | `int count{};` |
| `default_vs_value_new` | reads `p->x` and `p->y` after `new Point` | `new Point{}` |
| `partial_aggregate` | reads every member after `Config c;` | `Config c{3}` value-initializes the rest |

## Why it fails

The defect is undefined behavior: the program reads indeterminate `int` values. Debug fill bytes such as `0xCC` or `0xCD` can make the run look repeatable, but those bytes are implementation diagnostics, not C++ values.

## Correct direction

```cpp
int count{};
auto p = std::make_unique<Point>();
Config c{3};
```

Initialize at the declaration point, and prefer brace initialization for aggregates and scalar members. If a value is intentionally unknown, model that state explicitly with `std::optional`.

## Detection

| Tool | Result |
|---|---|
| MSVC warning/runtime check | can report simple scalar use before initialization, including `count` |
| MemorySanitizer | yes, on supported Clang platforms; it tracks uninitialized reads |
| AddressSanitizer | no — the storage is allocated and in bounds; ASan is not MemorySanitizer |
| Debugger byte inspection | useful evidence, but fill bytes are Debug-build artifacts |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session comparing initialized values with Debug fill-byte evidence.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 03.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 2.
