# Trap 38 — Most Vexing Parse

**Rule:** if a statement can be parsed as a declaration, C++ parses it as a declaration.

**This trap is not undefined behavior.** The wrong forms are parsing or compile-time issues, so sanitizer silence is the expected result.

## The rule

C++ grammar gives declarations priority in ambiguous-looking constructs. Inside a function, `Timer t();` is not a default-constructed local object; it declares a function named `t` that takes no parameters and returns `Timer`.

Braces often remove that declaration interpretation, but braces have their own overload rule: initializer-list constructors are preferred when available.

## In this code

`main.cpp` runs three variants:

| Function | Wrong or surprising form | Correct form |
|---|---|---|
| `empty_parens_declare_a_function` | `Timer t();` declares a function; `t.ticks` would not compile | `Timer braced{};` or `Timer plain;` |
| `named_argument_becomes_a_parameter` | commented iterator constructor shape can declare a function | named iterator values or `std::vector<char> braced_form{...}` |
| `braces_have_their_own_rule` | `std::vector<int> braces{3, 0}` creates two elements, not three zeros | `std::vector<int> parens(3, 0)` for the size constructor |

- **Safe target** (`Trap38_MostVexingParse`) — includes the parse trap and the safe alternatives directly.
- There is no `_unsafe` target because the source contains no `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The category is a parsing and compile-time error. No `Timer` object named `t` exists in the first variant, so member access would fail to compile. In the vector example, braces select a different valid constructor.

## Correct direction

```cpp
Timer braced{};
Timer plain;
std::vector<int> zeros(3, 0);
```

Use braces or no parentheses for default construction. Use parentheses when you specifically want a size/value constructor that would conflict with an initializer-list overload.

## Detection

| Tool | Result |
|---|---|
| MSVC C4930 / Clang and GCC `-Wvexing-parse` | warn that a declaration was probably intended as an object definition |
| `ctest -R Safe_VexingParse` | checks that the braced and plain `Timer` objects both have ticks 7 |
| Compiler errors | appear when trying to use `t` as an object |
| ASan / UBSan | no report, because the problem is not a runtime memory error |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session confirming no local `t` object exists.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 38.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 32.
