# Trap 21 — Check Then Act

**Rule:** the check and the action that relies on it must be protected as one invariant, not as two separate observations.

**This trap is not automatically undefined behavior.** It is an atomicity and TOCTOU defect; sanitizer silence is expected unless the unprotected interval also permits a concrete data race or lifetime error.

## The rule

A successful check describes only the state at the instant of the check. If another thread or process can change that state before the action, the later action is using a stale fact. Locking only around the check is therefore not enough: the lock must cover the check and the use, or the check must produce an owning snapshot that remains valid after the lock is released.

The same idea applies outside memory. A filesystem `exists()` check followed by a later `open()` is two observations of a pathname, and another process can change the path between them.

## In this code

`main.cpp` runs three named variants.

| Function | Demonstration | Correct form in the safe target |
|---|---|---|
| `check_then_act_lock` | copies `shared` while holding `m` | uses `snapshot = shared` as the lifetime handoff |
| `empty_then_pop` | `empty()` and `back()`/`pop_back()` can be split | one `std::scoped_lock` covers the check and pop |
| `exists_then_open` | `exists(path)` can go stale before reading | opens first, then tests the resulting stream |

- **Safe target** (`Trap21_CheckThenAct`) — keeps each checked invariant inside one protocol.
- **Unsafe target** (`Trap21_CheckThenAct_unsafe`, `RUN_UNSAFE_EXAMPLE`) — splits the queue check/use and the filesystem check/use.

## Why it fails

The defect is the unprotected interval. In `empty_then_pop`, `has_item` is only a stale-capable boolean after the lock is released. In `exists_then_open`, the path may be replaced or removed after `exists()` succeeds. This is a logic/atomicity defect; it becomes undefined behavior only if the interval allows an invalid memory access or data race.

## Correct direction

```cpp
std::scoped_lock lock(m);
if (!queue.empty()) {
    int value = queue.back();
    queue.pop_back();
    use(value);
}
```

For external resources, prefer acquire-and-validate APIs: open the file, then validate the handle you actually acquired.

## Detection

| Tool | Result |
|---|---|
| Schedule/timeline review | yes — shows the competing action between check and use |
| ThreadSanitizer | only if the bad window produces an actual data race; it does not prove TOCTOU absence |
| MSVC AddressSanitizer | no — the demonstrated bug is not an address error |
| CDB / WinDbg | useful for explaining the two stops, but breakpoints can hide the interleaving |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that makes the stale interval visible.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 21.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — questions 18 and 19, and practical exercise 6.
