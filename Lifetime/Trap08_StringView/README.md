# Trap 08 — `string_view` Does Not Own

**Rule:** `std::string_view` is only a pointer and a length; it never extends the lifetime of the characters it views.

## The rule

A `std::string_view` is a borrowed range. Constructing one from a `std::string` records where the string's characters currently live and how many characters are visible. It does not keep the string alive, prevent mutation, or subscribe to buffer changes.

The view becomes dangling when the owner dies, when a temporary owner is destroyed at the end of the full expression, or when the owner mutates in a way that changes the character buffer. Reading through a dangling view is undefined behavior.

## In this code

`main.cpp` has one helper, `make_owner()`, and three variants called from `main()`:

| Function | Wrong form | Correct form |
|---|---|---|
| `dangling_return` | `returned_view()` returns a view to local `s` | keep a named `std::string owner` and view that |
| `temporary_binding` | `std::string_view view = make_owner();` | store `make_owner()` in `owner` first |
| `mutated_owner` | use `view` after `owner` is assigned a longer string | set `view = owner` again after mutation |

- **Safe target** (`Trap08_StringView`) — keeps the owner alive or re-seats the view after mutation.
- **Unsafe target** (`Trap08_StringView_unsafe`, `RUN_UNSAFE_EXAMPLE`) — reads views after the owner has gone away or moved its buffer.

## Why it fails

The unsafe target has undefined behavior. The view's pointer and size can still look consistent, but they no longer describe a live character range owned by an active `std::string`.

## Correct direction

```cpp
std::string owner = make_owner();
std::string_view view = owner;

owner = "replacement text";
view = owner;                       // re-seat after mutation
```

Return owning strings from factories. Use `string_view` only when the caller can prove the viewed storage outlives every use.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | can report heap-backed dangling reads; small-string and stack cases may be silent |
| Debugger owner/view comparison | yes — compare `view.data()` with the current live owner's `data()` |
| Compiler warnings | incomplete — some tools warn on returning a view to a local, but not on all owner mutations |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session tracking the view pointer against the owning string buffer.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 08.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — questions 10 and 11, and practical exercise 3.
