# Trap 29 — Raw Bytes

**Rule:** suitably sized and aligned storage is not a live object until construction begins that object's lifetime.

## The rule

Raw storage and object lifetime are separate facts. `std::byte storage[sizeof(T)]` can reserve bytes with the right size, and `alignas(T)` can make the address suitable, but neither operation constructs a `T`. A typed pointer to that storage is only a candidate until lifetime is begun.

For non-implicit-lifetime class types such as `Record`, use construction and destruction APIs that express the lifetime boundary. After destruction, the bytes remain, but the object no longer exists.

## In this code

`main.cpp` defines `Record` with `std::string name` and `int number`. It creates aligned byte storage, forms `candidate`, constructs a `Record` with `std::construct_at(candidate, 7)`, prints the constructed fields, and then calls `std::destroy_at(object)`.

- **Safe target** (`Trap29_RawBytes`) — demonstrates the complete raw-storage protocol.
- There is no `_unsafe` target. The source contains no `RUN_UNSAFE_EXAMPLE`; it shows the correct rule rather than compiling a pre-lifetime typed access.

## Why it fails

The trap would be undefined behavior if code read `candidate->name` or `candidate->number` before `std::construct_at`, or after `std::destroy_at`. At those points the storage exists, but no live `Record` object exists there.

## Correct direction

```cpp
alignas(Record) std::byte storage[sizeof(Record)];
auto* candidate = reinterpret_cast<Record*>(storage);
Record* object = std::construct_at(candidate, 7);
std::destroy_at(object);
```

Treat `construct_at` and `destroy_at` as the lifetime markers. Do not confuse a byte address with a constructed class object.

## Detection

| Tool | Result |
|---|---|
| `ctest -R Safe_RawBytes` | checks that the constructed `Record` name and number appear |
| Debugger lifetime trace | shows the same storage before construction, during the live `Record`, and after destruction |
| ASan | usually no for pre-lifetime class access when storage is addressable |
| Compiler warnings | generally no; the pointer cast alone is not the lifetime violation |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session marking the raw-storage and live-object phases.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 29.
- [`../../SELF_TEST.md`](../../SELF_TEST.md) — question 23 and practical exercise 7.
