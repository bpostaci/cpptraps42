# Trap 14 — Alignment

**Rule:** a typed access is valid only when the address satisfies that type's alignment requirement.

## The rule

Every object type has an alignment requirement, reported by `alignof(T)`. Creating or using a `T*` for storage that is not suitably aligned for `T` violates the object model, even if the address lies inside a live byte buffer and even if the hardware happens to tolerate the load or store.

Alignment is a language requirement, not just a performance hint. Some architectures fault on misaligned access, while others execute it more slowly; the C++ program is undefined either way.

## In this code

`main.cpp` allocates `alignas(std::uint64_t) std::byte storage[sizeof(std::uint64_t)+1]`.

| Target | Pointer | Behavior |
|---|---|---|
| `Trap14_Alignment` | `reinterpret_cast<std::uint64_t*>(storage)` | address is aligned for `std::uint64_t`; writes value 7 and prints the read value |
| `Trap14_Alignment_unsafe` | `reinterpret_cast<std::uint64_t*>(storage + 1)` | address is one byte past the aligned base; typed access is misaligned UB |

The `_unsafe` target exists because the source contains `RUN_UNSAFE_EXAMPLE`.

## Why it fails

The unsafe branch has undefined behavior. The `std::byte` array provides storage, but `storage + 1` does not satisfy `alignof(std::uint64_t)`. A successful x64 Debug run only shows that this hardware/build accepted one misaligned instruction.

## Correct direction

```cpp
alignas(std::uint64_t) std::byte storage[sizeof(std::uint64_t)];
auto* p = reinterpret_cast<std::uint64_t*>(storage);
std::construct_at(p, 7ULL);
std::destroy_at(p);
```

Use storage whose address is aligned for the destination type, and begin the object's lifetime before treating the bytes as a live object.

## Detection

| Tool | Result |
|---|---|
| Clang/GCC UBSan alignment checks | reports the misaligned typed access |
| Debugger address check | `address % alignof(std::uint64_t)` exposes the defect |
| MSVC AddressSanitizer | no; it checks addressability, not this alignment rule |
| x64 hardware behavior | may appear to work, which is not evidence of defined C++ |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session measuring the aligned and misaligned addresses.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 14.
