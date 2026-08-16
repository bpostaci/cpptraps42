# Trap 24 — Temporary Lifetime

**Rule:** borrowing a pointer from a temporary object does not extend the temporary's lifetime.

## The rule

Temporary objects usually live until the end of the full expression that created them. Binding a temporary directly to a reference can extend that lifetime in specific cases, but extracting a pointer from the temporary is not one of those cases.

`std::string::c_str()` returns a borrowed pointer into the string's character storage. If the string is a temporary, that storage becomes unavailable when the full expression ends. The pointer value may remain non-null, but it no longer points into a live `std::string`.

## In this code

`main.cpp` is deliberately small:

| Target | Code shape | Lifetime result |
|---|---|---|
| `Trap24_TemporaryLifetime` | `std::string owner = "hello"; std::cout << owner.c_str()` | `owner` stays alive through the print |
| `Trap24_TemporaryLifetime_unsafe` | `const char* p = std::string("hello").c_str();` then prints `p` | the temporary owner dies at the semicolon before the print |

- **Safe target** (`Trap24_TemporaryLifetime`) — names the owning `std::string`.
- **Unsafe target** (`Trap24_TemporaryLifetime_unsafe`, `RUN_UNSAFE_EXAMPLE`) — stores a `const char*` borrowed from a temporary string.

## Why it fails

The unsafe target has undefined behavior. `p` is a borrowed character pointer whose owner was the temporary `std::string`. That owner is destroyed before the next statement begins, so streaming `p` reads through a dangling pointer.

## Correct direction

```cpp
std::string owner = "hello";
const char* p = owner.c_str();
std::cout << p << '\n';
```

Keep the owner alive for at least as long as the borrowed pointer is used. Prefer passing the `std::string` or `std::string_view` with an explicit owner lifetime when possible.

## Detection

| Tool | Result |
|---|---|
| Debugger lifetime trace | yes — line up the full-expression boundary with the later pointer use |
| AddressSanitizer | may catch some dangling reads, but small-string storage and immediate reuse can be silent |
| Compiler warnings | generally none — `c_str()` and pointer assignment are individually valid |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session showing the borrowed pointer after the temporary string has died.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 24.
