# Trap 33 — Map bracket insert

**Rule:** `std::map::operator[]` is lookup-or-insert, not a read-only lookup.

**This trap is not undefined behavior.** The insertion on a missing key is defined library behavior, so sanitizer silence is the expected, correct outcome.

## The rule

For associative containers such as `std::map`, `operator[]` must return a mutable reference to a mapped value. If the key is absent, the only way to return such a reference is to create an element with that key and a value-initialized mapped value.

Read-only lookup uses different APIs: `find`, `contains`, and `at`. A `const std::map` deliberately has no `operator[]`, because the operation may mutate the container.

## In this code

`main.cpp` runs three short variants. There is no `_unsafe` target because the source does not contain `RUN_UNSAFE_EXAMPLE`; the trap shows wrong and correct forms directly in the normal target `Trap33_MapBracketInsert`.

| Function | Form | Effect |
|---|---|---|
| `bracket_inserts_silently` | `scores["bob"] == 0` | inserts `bob` with value `0` while looking like a read |
| `non_mutating_lookups` | `find`, `contains`, and `at` | reads `scores` without inserting `bob` |
| `const_map_forbids_bracket` | `const std::map` and `at` | makes accidental bracket lookup a compile-time error; uses `histogram[c]` when insertion is intended |

`ctest -R Safe_MapLookup` runs this target and checks the root CMake PASS_REGULAR_EXPRESSION for the non-mutating lookup size.

## Why it fails

The category is defined-but-wrong behavior. The library does exactly what `operator[]` specifies, but the program violates its higher-level invariant that a lookup should not change the map.

## Correct direction

```cpp
if (auto it = scores.find("bob"); it != scores.end()) {
    use(it->second);
}
if (scores.contains("ada")) { use(scores.at("ada")); }
```

Use `operator[]` only when insert-or-update is the intended operation, as in the `histogram` loop.

## Detection

| Tool | Result |
|---|---|
| Assertions on `size()` | yes; they expose mutation across a supposed lookup |
| Debugger/container inspection | yes; `bob` appears after the bracket expression |
| AddressSanitizer / UBSan | no — there is no memory error or undefined operation |
| Compiler | helps if the map is `const`; otherwise the mutating call is valid |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session measuring map size before and after bracket lookup.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 33.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 27.