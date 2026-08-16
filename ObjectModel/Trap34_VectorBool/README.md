# Trap 34 — `vector<bool>` Proxy

**Rule:** `std::vector<bool>` is a packed specialization whose element access returns proxy objects, not real `bool&` references.

**This trap is not undefined behavior.** The proxy operations are defined library behavior; compile errors and surprising aliasing are the expected symptoms, not sanitizer reports.

## The rule

Unlike `std::vector<T>` for ordinary `T`, `std::vector<bool>` is permitted to pack bits instead of storing addressable `bool` objects. Because a single bit has no `bool*` address, `operator[]` returns a proxy reference object that knows how to read or write the selected bit.

That proxy can be useful for compact flags, but it breaks assumptions about references, contiguous `bool` storage, and `auto` copies.

## In this code

`main.cpp` runs three variants:

| Function | Demonstrated issue | Correct direction shown |
|---|---|---|
| `proxy_instead_of_reference` | `auto proxy = bits[0]` still aliases the container; `bool& ref = bits[0]` would not compile | write `bool copy = bits[2]` for a detached value |
| `no_contiguous_storage` | no `const bool* raw = bits.data()` and no `for (bool& b : bits)` | use `auto&& b` for proxies or `std::vector<char>` for bytes |
| `better_alternatives` | packed bits are the wrong abstraction for some jobs | `std::bitset`, `std::array<bool, N>`, or `std::vector<char>` |

- **Safe target** (`Trap34_VectorBool`) — contains all three demonstrations directly.
- There is no `_unsafe` target because the source contains no `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The category is defined-but-wrong design, with some wrong forms rejected at compile time. Code that expects an addressable `bool` element or detached `auto` value is using the wrong container contract.

## Correct direction

```cpp
bool copy = bits[2];              // detached value
for (auto&& bit : bits) bit = false;  // proxy-aware mutation
std::vector<char> flags(8, 1);    // contiguous addressable storage
```

Choose the container that matches the requirement: packed flags, fixed-size bits, real bool references, or byte-addressable interop storage.

## Detection

| Tool | Result |
|---|---|
| Compiler errors | catch attempts to bind `bool&` or use `bool*` storage |
| `ctest -R Safe_VectorBoolProxy` | checks that an explicit `bool` copy is detached from the proxy |
| Debugger type inspection | shows `auto proxy` is a proxy type, not `bool` |
| ASan / UBSan | no report, because the proxy behavior is valid library behavior |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing the proxy object and packed storage.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 34.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 28.
