# Trap 10 — Virtual in Constructor

**Rule:** virtual dispatch during construction or destruction is limited to the class whose constructor or destructor is currently running.

**This trap is not undefined behavior.** `Base::Base()` calling `Base::speak()` is defined dispatch to the base implementation, so sanitizer silence is the expected result.

## The rule

While a base subobject is being constructed, the most-derived object is not yet active as a complete `Derived`. The virtual-call rule reflects that phase: a virtual call made from `Base::Base()` resolves as if the dynamic type were `Base`, not the final type that will exist after all constructors finish.

This protects the program from calling an override that expects derived members to be initialized. It also means virtual functions are not an initialization customization point.

## In this code

`main.cpp` constructs a `Derived d`. The `Base` constructor calls `speak()`, and that call prints `Base phase` because the `Derived` part is not active yet. After construction, `main` calls `d.speak()`, which dispatches to `Derived::speak()` and uses `ready`.

- **Safe target** (`Trap10_VirtualInConstructor`) — shows the defined constructor-phase dispatch rule directly.
- There is no `_unsafe` target. The source contains no `RUN_UNSAFE_EXAMPLE` because this virtual call is not UB; it is defined dispatch with often-surprising semantics.

## Why it fails

The defect is defined-but-dangerous design. Code that expects `Base::Base()` to call `Derived::speak()` is relying on a dynamic type that does not exist yet. The derived member `ready` is not a valid dependency of base construction.

## Correct direction

```cpp
struct Base {
    Base() = default;
    virtual ~Base() = default;
    virtual void speak() {}
};

struct Derived : Base {
    int ready{99};
    void initialize() { speak(); }
};
```

Finish construction first, then call virtual customization from a separate step or factory. Keep base constructors responsible only for base invariants.

## Detection

| Tool | Result |
|---|---|
| Debugger call stack / vftable inspection | shows `Base::speak()` during `Base::Base()` and `Derived::speak()` after construction |
| Compiler warnings | generally no; the call is well formed and defined |
| ASan / UBSan | no report, because there is no memory or lifetime violation in this source |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing the constructor-phase vftable change.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 10.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 22.
