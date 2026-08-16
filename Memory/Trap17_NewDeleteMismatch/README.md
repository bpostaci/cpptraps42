# Trap 17 — New/delete mismatch

**Rule:** The deallocation and destruction protocol must match the way storage and object lifetime were created.

## The rule

C++ has several different allocation and lifetime protocols. `new[]` must be matched with `delete[]`; scalar `new` must be matched with scalar `delete`; `malloc` storage must be released with `free`; placement `new` constructs an object in caller-owned storage and does not allocate anything to delete.

The pointer value alone does not record enough information to repair a mismatch later. The program must preserve the correct ownership protocol from the creation site.

## In this code

`main.cpp` runs three short variants around `Widget`. The safe target is `Trap17_NewDeleteMismatch`. Because the source contains `RUN_UNSAFE_EXAMPLE`, enabling `TRAPS_BUILD_UNSAFE` also creates `Trap17_NewDeleteMismatch_unsafe`.

| Function | Unsafe form | Safe form |
|---|---|---|
| `array_form_mismatch` | `new Widget[3]` followed by scalar `delete` | `std::make_unique<Widget[]>(3)` |
| `allocator_family_mismatch` | `std::malloc(sizeof(Widget))` followed by `delete` | `std::make_unique<Widget>()` |
| `placement_new_mismatch` | placement `new` into `buffer`, then `delete p` | call `p->~Widget()`; `buffer` owns the storage |

## Why it fails

The defect is undefined behavior: each unsafe variant invokes the wrong destruction or deallocation protocol. The `malloc` case also has no constructed `Widget` before `delete`; the placement case tries to free stack storage that no allocation function returned.

## Correct direction

```cpp
auto objects = std::make_unique<Widget[]>(3);
auto one = std::make_unique<Widget>();
Widget* placed = new (buffer) Widget{};
placed->~Widget();
```

Prefer RAII owners that encode the deallocator. When using placement `new`, write the explicit destructor call at the same abstraction level as the construction.

## Detection

| Tool | Result |
|---|---|
| AddressSanitizer | often reports alloc/dealloc mismatch or invalid free |
| Debug CRT heap checks | can stop on mismatched heap operations in Debug builds |
| Compiler warnings | partial; runtime allocation families are generally hard to prove statically |
| Debugger address inspection | useful for placement `new`, but it does not enforce the contract |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB session showing array, allocator-family, and placement-new evidence.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 17.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 14.
