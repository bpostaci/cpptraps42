# Trap 36 — Hidden Overload

**Rule:** declaring a member with a name in a derived class hides every base-class overload with that same name.

**This trap is not undefined behavior.** Name hiding is a compile-time overload-resolution rule, so sanitizer silence is the expected result.

## The rule

Unqualified lookup finds declarations by name before overload resolution chooses among signatures. When `Hiding` declares `log(double)`, lookup for `h.log(...)` stops in `Hiding`; the `Base::log(int)` and `Base::log(const std::string&)` overloads are not candidates unless they are explicitly qualified or reintroduced.

The same idea protects virtual overrides: a signature mismatch is not an override. The `override` keyword turns that mistake into a compile error.

## In this code

`main.cpp` runs three variants:

| Function | Wrong form | Correct form |
|---|---|---|
| `name_hiding_changes_overload_resolution` | `h.log(1)` converts to `Hiding::log(double)`; `h.log("text")` would not compile | `h.Base::log(1)` reaches the base explicitly |
| `using_declaration_restores_the_set` | — | `using Base::log;` makes all base overloads visible in `Exposing` |
| `override_keyword_catches_mismatch` | commented `draw(long) const override` would fail | `Circle::draw(int) const override` matches `Shape::draw` |

- **Safe target** (`Trap36_HiddenOverload`) — demonstrates hiding and the repairs directly.
- There is no `_unsafe` target because the source contains no `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The category is a compile-time lookup and design error. The base overloads exist, but they are not in the overload set selected for `Hiding`. Overload resolution cannot choose a function that name lookup never found.

## Correct direction

```cpp
struct Exposing : Base {
    using Base::log;
    void log(double value);
};
```

Add a `using` declaration when a derived class intentionally extends a base overload set. Use `override` on virtual functions so signature drift is diagnosed immediately.

## Detection

| Tool | Result |
|---|---|
| `ctest -R Safe_HiddenOverload` | checks that the restored overload set selects `Base::log(int)` for the `int` argument |
| Compiler | rejects calls such as the commented hidden string overload and catches bad `override` signatures |
| Debugger call stack | shows `h.log(1)` resolved to `Hiding::log(double)` |
| ASan / UBSan | no report, because no runtime memory rule is violated |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing which overload each call resolved to.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 36.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 30.
