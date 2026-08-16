# Trap 13 — Strict Aliasing

**Rule:** a cast changes the type of the *pointer*, not the type of the object stored at that address.

## The rule

An object may be read only through a type compatible with its dynamic type (plus a few explicitly permitted forms, such as `char`, `unsigned char`, and `std::byte`). Reading a `float` object through a `std::uint32_t*` is not one of them, so the access is undefined behavior even though both types are four bytes wide and the address is perfectly aligned.

Compilers rely on this rule to prove that a `float*` and a `std::uint32_t*` cannot refer to the same object, which lets them reorder, cache, or eliminate loads and stores. That is why the defect is optimizer-dependent: `/Od` often produces the expected number and `/O2` does not.

## In this code

`main.cpp` extracts the bit pattern of `1.0F` two ways:

- **Safe target** (`Trap13_StrictAliasing`) — `std::bit_cast<std::uint32_t>(value)`, a defined value-representation copy between equal-sized trivially copyable types.
- **Unsafe target** (`Trap13_StrictAliasing_unsafe`, `RUN_UNSAFE_EXAMPLE`) — `*reinterpret_cast<std::uint32_t*>(&value)`, a typed access the object model does not permit.

Both print `3f800000` in a typical Debug build. That agreement is the trap: it is evidence about one build, not about the language rule.

## Why it fails

`reinterpret_cast` produces a pointer of the requested type; it does not create a `std::uint32_t` object at that address and does not end the `float`'s lifetime. Dereferencing it reads an object through an incompatible type. Because the behavior is undefined, the compiler may assume the two pointers never alias and keep `value` in a register while the read observes stale memory — or vice versa.

## Correct direction

```cpp
auto bits = std::bit_cast<std::uint32_t>(value);   // C++20, constexpr-friendly
std::memcpy(&bits, &value, sizeof bits);           // pre-C++20 equivalent
```

Both copy the value representation without claiming that one object is another. Reading through `std::byte`/`unsigned char` is also permitted when inspecting raw bytes is the actual intent.

## Detection

| Tool | Result |
|---|---|
| Comparing `/Od` and `/O2` output | the most reliable signal; a value that changes with optimization |
| GCC/Clang `-Wstrict-aliasing` | sometimes warns, and misses many real cases |
| UndefinedBehaviorSanitizer | may flag related type-confusion, but does not diagnose aliasing generally |
| AddressSanitizer | no — memory is in bounds and alive; the violation is in the type system |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session comparing the generated loads.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 13.
