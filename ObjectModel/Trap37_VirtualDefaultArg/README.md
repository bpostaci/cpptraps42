# Trap 37 — Virtual Default Argument

**Rule:** virtual dispatch selects the function body dynamically, but default arguments are substituted from the static type at the call site.

**This trap is not undefined behavior.** The calls are fully defined; the surprising result is the specified split between static defaults and dynamic dispatch.

## The rule

Default arguments are not virtual. They are compile-time substitutions made where the call is written. After that substitution, the virtual call mechanism chooses the final overrider using the object's dynamic type.

Putting different defaults on overrides therefore creates one function body that can receive different implicit argument values depending on the expression's static type.

## In this code

`main.cpp` runs three variants:

| Function | Demonstrated issue | Correct direction shown |
|---|---|---|
| `default_argument_comes_from_static_type` | `d.render()` uses `Derived`'s default `100`; `as_base.render()` calls `Derived::render` with `Base`'s default `1` | avoid different defaults on virtual functions |
| `non_virtual_interface_has_one_default` | — | `Interface::render(int scale = 1)` is non-virtual and calls virtual `do_render` |
| `overloads_instead_of_defaults` | — | `Explicit::render()` forwards to `render(1)` |

- **Safe target** (`Trap37_VirtualDefaultArg`) — contains the wrong and corrected designs directly.
- There is no `_unsafe` target because the source contains no `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The category is defined-but-wrong behavior. The programmer expects the override's default to travel with the override body, but the default was already chosen from the static type before virtual dispatch happened.

## Correct direction

```cpp
struct Interface {
    void render(int scale = 1) const { do_render(scale); }
private:
    virtual void do_render(int scale) const = 0;
};
```

Keep defaults on a non-virtual wrapper, or use overloads instead of defaults. The virtual function itself should receive explicit arguments.

## Detection

| Tool | Result |
|---|---|
| `ctest -R Safe_VirtualDefault` | checks that the base-reference call reaches the derived override with scale 1 |
| Code review | effective when looking for default arguments repeated on overrides |
| Debugger argument inspection | shows `Derived::render` reached with both `100` and `1` |
| ASan / UBSan | no report, because the behavior is defined |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing dynamic body selection with static default values.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 37.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 31.
