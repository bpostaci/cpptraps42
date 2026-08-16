# Trap 07 — Iterator invalidation

**Rule:** After a container operation that invalidates an iterator, pointer, or reference, reacquire it before use.

## The rule

Iterator validity is a container contract. A stored address may still contain bytes after a mutation, but the program no longer has permission to use it if the operation invalidated that handle.

The invalidation rule depends on both the container and the operation. `std::vector` reallocation invalidates all iterators, pointers, and references into the old buffer. `std::vector::erase` invalidates the erased position and following positions. `std::map::erase` destroys only the erased node; other iterators remain valid.

## In this code

`main.cpp` runs three short variants. The safe target is `Trap07_IteratorInvalidation`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap07_IteratorInvalidation_unsafe`.

| Function | Unsafe form | Safe form |
|---|---|---|
| `reallocation_invalidation` | dereferences `old` and `old_reference` after `v.push_back(4)` may reallocate | uses `v.begin()` and `v.front()` after the mutation |
| `erase_invalidation` | dereferences `it` after `v.erase(it)` | continues with the returned `next` iterator |
| `node_invalidation` | dereferences `erased` after `m.erase(erased)` | uses `survivor`, which still refers to a live map node |

## Why it fails

The unsafe dereferences are undefined behavior. The object or node may have moved, been destroyed, or simply no longer be reachable through that handle under the standard container contract.

## Correct direction

```cpp
auto next = v.erase(it);
for (auto current = next; current != v.end(); ++current) {
    use(*current);
}
```

Use returned iterators, reacquire references after mutation, and reserve capacity only when that is genuinely part of the invariant you need.

## Detection

| Tool | Result |
|---|---|
| Debug iterator checks | often report invalidated iterator use in Debug STL builds |
| AddressSanitizer | catches stale storage reads when invalidation also reaches poisoned storage |
| Compiler warnings | generally no; the operations are well formed and validity is path-dependent |
| Release build | often silent; stale storage may still hold the old-looking value |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session comparing vector buffers, erased iterators, and surviving map nodes.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 07.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — questions 8 and 9 and practical exercise 2.
