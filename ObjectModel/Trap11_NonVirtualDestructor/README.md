# Trap 11 — Non-Virtual Destructor

**Rule:** a polymorphic base that is deleted through a base pointer needs a virtual destructor, or deletion through that base type has undefined behavior.

## The rule

Deleting an object through a pointer to base must use the same destruction contract that created the complete object. If the static type has a non-virtual destructor, the delete expression cannot dispatch to the derived destructor, so the standard makes that operation undefined behavior when the dynamic object is derived.

A base class can also forbid polymorphic deletion by making its destructor protected and non-virtual. What it must not do is offer public base-pointer deletion while omitting the virtual destructor.

## In this code

`main.cpp` demonstrates the correct form. `Base` declares `virtual ~Base() = default`, `Derived` owns a `std::unique_ptr<int> resource`, and `std::unique_ptr<Base> p` is initialized with `std::make_unique<Derived>()`.

- **Safe target** (`Trap11_NonVirtualDestructor`) — destruction through `std::unique_ptr<Base>` dispatches through `Base`'s virtual destructor.
- There is no `_unsafe` target. The source contains no `RUN_UNSAFE_EXAMPLE` because this folder compiles the supported virtual-destructor form rather than a deliberately broken base class.

## Why it fails

The broken version would be undefined behavior: `std::unique_ptr<Base>` would perform deletion through `Base*` while the complete object is `Derived`. Without a virtual destructor, the derived cleanup contract is missing, so `Derived::resource` cleanup is not something the program may rely on.

## Correct direction

```cpp
struct Base {
    virtual ~Base() = default;
};

struct Derived final : Base {
    std::unique_ptr<int> resource = std::make_unique<int>(42);
};
```

Use a public virtual destructor when clients may own derived objects through `Base*` or `std::unique_ptr<Base>`. If base-pointer deletion is not supported, make that impossible in the interface.

## Detection

| Tool | Result |
|---|---|
| Compiler warnings | often warn for deleting a polymorphic object through a base with a non-virtual destructor |
| Leak tools / destructor breakpoints | show skipped derived cleanup in broken variants |
| ASan | not a general detector for missing virtual destructors; the wrong delete may not immediately touch invalid memory |
| This repository target | safe by construction; `Base` has a virtual destructor |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing the destructor dispatch path.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 11.
