# Visual Studio Debugging Guide

Open `C:\src\cpptraps42` with **File > Open > Folder...**, select the `msvc-debug` preset, and choose a trap target. Search the selected source for `BP:` and set breakpoints on those lines. Build the normal target first. If an `_unsafe` sibling exists, use it only after you understand the invariant being demonstrated.

An expanded target runs three named variant functions in sequence. You do not need to step through all of them: place a function breakpoint such as `mixed_sign_comparison` or a line breakpoint inside the desired function, then press `F5`. Use **Step Over** (`F10`) for state changes and **Step Into** (`F11`) when ownership, construction, or dispatch is the subject.

`_unsafe` means “contrast/anti-pattern teaching branch.” It does not guarantee a crash or sanitizer report. Some variants are undefined behavior; others are defined but surprising, valid with an unspecified state, or unsafe only when another thread/process changes the state between operations.

## WinDbg/CDB notebooks beside every trap

All 42 trap folders contain `debug_analysis.md` and `debug_analysis.txt`. The two files describe the same trap-specific session in different formats: Markdown for study and review, plain text for keeping beside a CDB prompt. Each notebook includes the relevant source excerpt, normal and unsafe build commands where applicable, symbol/source-path setup, exact breakpoint commands, inspection commands, representative evidence, and the invariant that explains that evidence.

Use the notebook in the selected trap folder rather than copying commands from another trap. Target names, source paths, line numbers, useful symbols, and debugger limitations differ. A typical workflow is:

1. Open `TrapXX_Name/debug_analysis.md` and read the question and short answer.
2. Build the exact target listed in its **Build without a sanitizer** section.
3. Start CDB with that notebook's executable, symbol path, and source path.
4. Apply its initial debugger settings (for example `.symopt-100` and `.lines -e` when specified).
5. Run the safe session first, then the `_unsafe` session only when the notebook provides one.
6. Compare relationships and invariants, not ASLR-dependent addresses or allocator-specific byte values.

`DEBUGGING.md` is the cross-project map; the two files beside `main.cpp` are the authoritative step-by-step session for that individual trap.

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
| `Trap31` implicit copy/Rule of Three/Rule of Zero | Watch the raw pointer member in both objects after the copy. Identical addresses mean two owners and two `delete[]` calls. Compare against the deep-copy and smart-member versions. |
| `Trap32` cycle/`weak_ptr`/lock | Watch `use_count()` before and after the back edge is assigned, then at scope exit. Missing destructor output is the leak evidence; no sanitizer report is expected. |
| `Trap33` bracket/find/histogram | Inspect `size()` immediately before and after the `operator[]` lookup. The container grew during what looked like a read. |
| `Trap34` proxy/storage/alternatives | Inspect the type of `bits[0]` in the debugger. It is `std::vector<bool>::reference`, not `bool`, so the local copy still writes through to the container. |
| `Trap35` reference/range-for/`decltype(auto)` | Compare the address of the deduced variable with the address of the source object. Different addresses prove a copy was made. |
| `Trap36` hiding/`using`/`override` | Step into each call and read the resolved function name in the call stack. The chosen overload, not a crash, is the evidence. |
| `Trap37` default argument/NVI/overloads | Call through both the derived and the base static type. The body comes from the dynamic type and the default argument from the static type. |
| `Trap38` empty parens/arguments/braces | This trap is resolved at compile time. Read the MSVC C4930 warning and confirm the declared entity is a function, then compare `vector<int> v(3,0)` with `vector<int> v{3,0}` in the watch window. |
| `Trap39` reorder/safe/base ordering | Break in the constructor and step over each member initialization. The execution order follows the declarations, not the order written in the init-list. |
| `Trap40` equality/tolerance/accumulation | Print with `setprecision(20)` to see the stored value rather than the rounded display. Compare the absolute difference against a magnitude-scaled tolerance. |
| `Trap41` `size()-1`/reverse loop/mixed sign | Inspect the unsigned result of the subtraction. A value near `SIZE_MAX` is the wrap, and it is defined behavior, not corruption. |
| `Trap42` joinable/exception/detach | Break in the thread destructor path. A joinable thread reaching its destructor calls `std::terminate`, so use the `_unsafe` target deliberately and expect the process to abort. |

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
| name hiding, default arguments, vexing parse | compiler warnings + call stack | resolved at compile time; no runtime tool applies |
| inserting lookup, unsigned wrap, float equality | debugger + invariant assertions | fully defined behavior, so no sanitizer will report it |
| leaked `shared_ptr` cycle | `use_count` inspection / leak report | ASan reports the leak, not the cycle that caused it |

## A repeatable debugging loop

1. State the invariant: who owns the object, whether its lifetime has begun, and what bounds/type/thread contract applies.
2. Break immediately before the event that can invalidate that invariant.
3. Record addresses, sizes, capacity, dynamic type, and ownership state in Watch.
4. Step over exactly one operation and compare the state.
5. Run the matching sanitizer build when the defect class is supported.
6. Re-run the normal target and confirm that the safe pattern prevents the invalid state rather than merely hiding the symptom.
