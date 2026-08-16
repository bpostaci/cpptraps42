# Trap 35 — `auto` Drops the Reference

**Rule:** `auto` deduction strips references and top-level `const`. `auto x = f();` copies, even when `f` returns `T&`.

**This trap is not undefined behavior.** Every line is well defined; the program is simply wrong. No sanitizer will report it, which is exactly why it survives code review.

## The rule

`auto` follows template argument deduction: the reference is removed, top-level `const` and `volatile` are removed, and what remains is the deduced type. So when `f()` returns `Heavy&`, `auto x = f();` deduces `Heavy` and constructs a full copy. Writes to `x` then modify the copy, and the original never changes.

The same rule drives range-for: `for (auto item : items)` copies every element, mutates the copy, and discards it at the end of the iteration.

## In this code

`main.cpp` runs three variants against one `static Heavy` returned by reference. There is no `_unsafe` target — each variant shows the wrong form and its correction side by side.

| Function | Wrong form | Correct form |
|---|---|---|
| `auto_copies_a_reference` | `auto copy = shared_instance();` | `auto& reference = shared_instance();` |
| `range_for_copies_elements` | `for (auto item : items)` | `for (auto& item : items)` / `for (const auto& item : items)` |
| `keeping_the_exact_type` | — | `decltype(auto) exact = shared_instance();` |

The output is the assertion: after the copy, `shared hits=0`; after `auto&`, `shared hits=42`.

## Why it fails

Nothing is invalid — the copy is a legitimate `Heavy` object with a legitimate lifetime. The bug is that the programmer intended an alias and received a value. The two costs are silent: the mutation goes to the wrong object, and every copy carries the `std::string` payload.

## Correct direction

| Intent | Write |
|---|---|
| Observe without copying | `const auto&` |
| Mutate the original | `auto&` |
| Preserve exactly what the expression returned | `decltype(auto)` |
| Deliberately take an independent copy | `auto` — and say so in a comment |

`decltype(auto)` is the right tool when forwarding a return type you do not control, such as a proxy from `std::vector<bool>` (see Trap 34).

## Detection

| Tool | Result |
|---|---|
| Debugger type inspection | yes — compare the address of the source and of the deduced variable |
| `ctest -R Safe_AutoReference` | asserts `decltype(auto) shared hits=7` |
| Compiler warnings | generally none; the code is valid |
| ASan / UBSan / TSan | no — there is no memory, lifetime, or race error to find |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that classifies each deduction as copy or alias.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 35.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 29 and practical exercise 10.
