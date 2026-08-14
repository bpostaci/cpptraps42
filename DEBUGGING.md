# Visual Studio Debugging Guide

Open `C:\src\cpptraps42` with **File > Open > Folder...**, select the `msvc-debug` preset, and choose a trap target. Search the selected source for `BP:` and set breakpoints on those lines. Build the normal target first. If an `_unsafe` sibling exists, use it only after you understand the invariant being demonstrated.

An expanded target runs three named variant functions in sequence. You do not need to step through all of them: place a function breakpoint such as `mixed_sign_comparison` or a line breakpoint inside the desired function, then press `F5`. Use **Step Over** (`F10`) for state changes and **Step Into** (`F11`) when ownership, construction, or dispatch is the subject.

`_unsafe` means “contrast/anti-pattern teaching branch.” It does not guarantee a crash or sanitizer report. Some variants are undefined behavior; others are defined but surprising, valid with an unspecified state, or unsafe only when another thread/process changes the state between operations.

## Inspecting the expanded labs

| Target and variant | Breakpoint and observations |
|---|---|
| `Trap03` scalar | Stop at the first read. Inspect the variable without assuming Debug fill bytes are values. MSVC warning C4700 is primary evidence. |
| `Trap03` dynamic object | Stop after default initialization. Compare default initialization with value initialization (`new T{}`). |
| `Trap03` aggregate | Stop before the field read and identify which members were initialized. |
| `Trap04` subscript | Inspect `index`, `size()`, `data()`, and `data()+index`. One-past may be formed, never dereferenced. |
| `Trap04` off-by-one | Break on the loop condition and watch the final legal index and the first illegal one. |
| `Trap04` lost extent | Step into the pointer-taking function; note that the callee cannot recover the array bound. |
| `Trap05` signed overflow | Stop before `maximum + 1`. The operation itself is UB; a check performed afterward is too late. Use UBSan with Clang. |
| `Trap05` mixed sign | Inspect the converted operands. `-1 < size_t(3)` is **false** because `-1` converts to a large unsigned value; this is defined but often defeats intended range logic. |
| `Trap05` narrowing | Watch `300` become the `unsigned char` result. This unsigned conversion is defined modulo the destination range, but it loses information. |
| `Trap06` scope/delete/reallocation | Record the address and the owner state before the lifetime-ending event, step over it, then observe that a numerically unchanged address does not prove a live object. |
| `Trap07` vector/erase/container rules | Compare `data()` and `capacity()` across growth; inspect which iterators erase invalidates; do not transfer one container's invalidation rules to another. |
| `Trap08` local/temporary/mutation | Watch both `string_view::data()` and the owning string's `data()`. Scope exit, temporary destruction, or reallocation can end the view's validity. |
| `Trap09` reference/`this`/shared owner | Break where the callback is created and invoked. Identify exactly what the closure stores and whether that object still lives. |
| `Trap12` copy/container/parameter | Inspect static and dynamic types before and after copying into `Base`. Slicing is defined behavior; the derived portion is simply not copied. |
| `Trap17` array/family/placement new | Match `new[]` with `delete[]`, allocation families with their deallocators, and every placement construction with one explicit destruction. ASan catches many mismatches, not all lifetime protocol errors. |
| `Trap19` duplicate owner/shallow copy/cleanup path | Track every pointer value and list who believes it owns the allocation. Set a data breakpoint or ASan breakpoint at the first release, not only the later failure. |
| `Trap21` split lock/queue/filesystem | Mark the check and the use as separate events. Ask what another thread or process can change between them. A debugger may hide the race; these examples illustrate the TOCTOU window rather than forcing a deterministic failure. |
| `Trap23` string/`unique_ptr`/self-move | Inspect the object after the move. It remains valid where the type contract says so, but its state may be unspecified; `unique_ptr` specifically becomes null after a successful move. Do not assume a particular self-move result unless the contract guarantees it. |

## Other high-value stops

- `Trap01_UseAfterFree`: record `owner.get()` and the observer before `owner.reset()`. The address can remain while ownership and lifetime disappear.
- `Trap10_VirtualInConstructor`: break in both `speak()` implementations and step through `Base::Base()`. Base dispatch is defined during base construction.
- `Trap14_Alignment`: inspect `reinterpret_cast<uintptr_t>(p) % alignof(T)`. Prefer UBSan/alignment checks over conclusions based on tolerant x64 hardware.
- `Trap16_MemsetObject`: inspect the class before the byte write. A raw overwrite does not perform class assignment and can destroy invariants.
- `Trap20_DataRace`: use ThreadSanitizer on Clang/Linux or WSL. Ordinary breakpoints serialize execution and can conceal the schedule.
- `Trap22_VolatileIsNotSync`: break on release and acquire. The successful acquire publishes the payload; `volatile` alone would not.
- `Trap28_ABIMismatch`: compare `sizeof`, `alignof`, member offsets, architecture, runtime library, packing, calling convention, ownership, and byte order across modules.
- `Trap29_RawBytes`: inspect alignment and bytes before `construct_at`, then the live object afterward. After `destroy_at`, storage remains but the `T` lifetime does not.

## Diagnostic tool map

| Symptom | First tool | Important limitation |
|---|---|---|
| use-after-free, bounds, double free | AddressSanitizer | does not prove all ownership protocols correct |
| signed overflow, misalignment | UndefinedBehaviorSanitizer (Clang) | MSVC does not provide full UBSan |
| data race | ThreadSanitizer (Clang/Linux or WSL) | not available in MSVC; debugger timing can hide races |
| uninitialized value | compiler warnings / MemorySanitizer | MSan is primarily a Clang tool on supported platforms |
| slicing, moved-from assumptions, narrowing | debugger + contract review | often valid/defined, so sanitizer silence is expected |

## A repeatable debugging loop

1. State the invariant: who owns the object, whether its lifetime has begun, and what bounds/type/thread contract applies.
2. Break immediately before the event that can invalidate that invariant.
3. Record addresses, sizes, capacity, dynamic type, and ownership state in Watch.
4. Step over exactly one operation and compare the state.
5. Run the matching sanitizer build when the defect class is supported.
6. Re-run the normal target and confirm that the safe pattern prevents the invalid state rather than merely hiding the symptom.
