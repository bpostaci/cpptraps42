# C++ Traps 42 - Visual Studio CMake Training Project

Each of the 42 canonical traps is a separate executable target and lives in its own clearly named folder. Selected labs contain several closely related real-world variants without changing the book's canonical trap structure. The repository is a native CMake project; generated solution and project files are build artifacts and are intentionally ignored by Git.

**Document version:** 2.0.0  
**Publication date:** 15 August 2026
**Author:** Buğra POSTACI · Software Engineer · Debugging Specialist
**Repository:** https://github.com/bpostaci/cpptraps42

Traps 01-30 form the original canonical set. Traps 31-42 extend it with the everyday standard-library, class-design, and parsing mistakes that practising C++ engineers report most often.

## Human-AI collaboration

This project was created through collaboration between **Bugra Postaci** and multiple AI systems, including **OpenAI Codex, Claude, Fable, and MAI**. Bugra defined the goals, selected the scope, directed the iterations, reviewed the outputs, and retains responsibility for the final publication. The AI systems assisted across drafting, code scaffolding, documentation, build automation, consistency checks, review suggestions, and PDF layout. AI assistance does not replace independent technical review; corrections and reproducible issue reports are welcome.

## Visual Studio

1. Select **File > Open > Folder...**.
2. Open the repository root: `C:\src\cpptraps42`.
3. Visual Studio detects `CMakeLists.txt` and `CMakePresets.json` automatically.
4. Select `msvc-debug` or `msvc-asan-unsafe` from the configuration preset menu.
5. In Solution Explorer, expand `Memory`, `Lifetime`, `ObjectModel`, `Concurrency`, or `ABI_Build`.
6. Select the trap target you want to run, search its sources for `BP:`, and set the suggested breakpoints.
7. Build and start debugging with `F5`.

Opening the repository folder keeps Solution Explorer attached to the tracked source tree, so Visual Studio's Git status decorations remain meaningful. It also exposes CMake presets directly and connects the CTest tests to Test Explorer. Do not open a generated solution under `VisualStudio/` or another build directory as the primary workspace.

Projects that contain an anti-pattern demonstration have a separately generated target ending in `_unsafe`. For example, use `Trap01_UseAfterFree` to study the safe path and `Trap01_UseAfterFree_unsafe` to enter the guarded invalid access. Do not add `RUN_UNSAFE_EXAMPLE` manually; CMake defines it only for these explicit targets. Depending on the variant, an `_unsafe` branch may demonstrate undefined behavior, a lifetime error, an unspecified-but-valid state, or defined behavior that is surprising and error-prone. Prefer AddressSanitizer for memory-unsafe targets; do not expect a sanitizer to diagnose every logic or API-contract mistake.

## Variant model

The executable target remains the unit you select in Visual Studio. In the following expanded labs, one run executes three short, named examples in sequence; set a breakpoint inside the function for the variant you want to study.

| Target | Included variants |
|---|---|
| `Trap03_UninitializedMemory` | uninitialized scalar, default-initialized dynamic object, partially initialized aggregate |
| `Trap04_OutOfBounds` | unchecked subscript, off-by-one loop, lost array extent |
| `Trap05_SignedOverflow` | signed overflow, mixed signed/unsigned comparison, narrowing conversion |
| `Trap06_EndedLifetime` | escaped stack address, deleted object, invalidation after container growth |
| `Trap07_IteratorInvalidation` | vector reallocation, erase invalidation, container-specific assumptions |
| `Trap08_StringView` | local owner, temporary owner, owner mutation/reallocation |
| `Trap09_LambdaCapture` | reference capture, implicit `this`, explicit shared ownership |
| `Trap12_ObjectSlicing` | copy initialization, base container, pass-by-value |
| `Trap17_NewDeleteMismatch` | scalar/array mismatch, wrong deallocator family, placement-new lifetime |
| `Trap19_DoubleFree` | duplicated raw ownership, shallow copy, manual cleanup paths |
| `Trap21_CheckThenAct` | split locking, queue check/pop, filesystem TOCTOU |
| `Trap23_MovedFrom` | valid but unspecified state, moved `unique_ptr`, self-move assumptions |
| `Trap31_ShallowCopy` | implicit copy of a raw owner, Rule of Three, Rule of Zero |
| `Trap32_SharedPtrCycle` | owning cycle, `weak_ptr` back edge, locking an expired observer |
| `Trap33_MapBracketInsert` | inserting lookup, non-mutating lookup, intentional insert-or-update |
| `Trap34_VectorBool` | proxy reference, missing contiguous storage, better alternatives |
| `Trap35_AutoDropsRef` | copied reference, copying range-for, `decltype(auto)` |
| `Trap36_HiddenOverload` | name hiding, `using` re-exposure, `override` signature mismatch |
| `Trap37_VirtualDefaultArg` | static default argument, non-virtual interface, real overloads |
| `Trap38_MostVexingParse` | empty parentheses, constructor-shaped arguments, brace pitfalls |
| `Trap39_MemberInitOrder` | declaration-order initialization, safe dependent members, base ordering |
| `Trap40_FloatComparison` | exact equality, scaled tolerance, accumulation and NaN |
| `Trap41_UnsignedUnderflow` | `size() - 1` wrap, reverse loop, mixed-sign comparison |
| `Trap42_ThreadJoinDetach` | joinable destruction, exception path, detached data lifetime |

The safe target presents the corrective pattern. The `_unsafe` target activates the contrasting branch; its suffix is a teaching label, not a claim that every line necessarily triggers undefined behavior.

## CMake

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Visual Studio's CMake target view groups targets by the same topic folders.

### Repeatable presets

```powershell
cmake --preset msvc-debug
cmake --build --preset build-safe
ctest --preset test-safe
```

To create safe/unsafe project pairs with MSVC AddressSanitizer:

```powershell
cmake --preset msvc-asan-unsafe
cmake --build --preset build-asan-unsafe
```

An unsafe target is suffixed `_unsafe`, for example `Trap01_UseAfterFree_unsafe`. CMake creates this sibling only when the trap source contains a `RUN_UNSAFE_EXAMPLE` branch; conceptual or already-safe demonstrations do not receive a duplicate unsafe target.
MSVC supports AddressSanitizer but not ThreadSanitizer or MemorySanitizer. Use Clang/GCC in Linux or WSL for TSan; MemorySanitizer is primarily supported by Clang on suitable platforms.

The trap number is the position in the canonical 42-trap book list. Categories therefore contain non-contiguous numbers by design.

The presets intentionally omit a fixed generator so they can use the Visual Studio version installed on the current machine. Run them from a Visual Studio Developer PowerShell or another shell where MSVC and CMake are available.

## Safety

Never enable unsafe branches in production. Undefined behavior may crash, appear to work, or change with optimization. Other labs deliberately show defined but misleading conversions, unspecified states, and broken higher-level invariants; these may not trigger a sanitizer at all. Stop before the questionable operation, inspect the relevant lifetime/bounds/ownership contract, and use the appropriate diagnostic tool where one applies.

## License

This project is released under the [MIT License](LICENSE). Copyright (c) 2026 Bugra Postaci.
