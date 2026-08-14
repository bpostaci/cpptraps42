# C++ Traps - Visual Studio CMake Training Project

Each of the 30 traps is a separate executable target and lives in its own clearly named folder. The repository is a native CMake project; generated solution and project files are build artifacts and are intentionally ignored by Git.

**Document version:** 1.0.0  
**Publication date:** 14 August 2026  
**Author:** Buğra POSTACI · Software Engineer · Debugging Specialist

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

Projects that contain an intentionally broken operation have a separately generated target ending in `_unsafe`. For example, use `Trap01_UseAfterFree` to study the safe path and `Trap01_UseAfterFree_unsafe` to enter the guarded invalid access. Do not add `RUN_UNSAFE_EXAMPLE` manually; CMake defines it only for these explicit unsafe targets. Prefer an AddressSanitizer configuration for memory-unsafe targets.

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

The trap number is the position in the canonical 30-trap book list. Categories therefore contain non-contiguous numbers by design.

The presets intentionally omit a fixed generator so they can use the Visual Studio version installed on the current machine. Run them from a Visual Studio Developer PowerShell or another shell where MSVC and CMake are available.

## Safety

Never enable unsafe branches in production. Undefined behavior may crash, appear to work, or change with optimization. The purpose is to stop immediately before the invalid operation, inspect the lifetime/bounds/ownership invariant, and then let the relevant sanitizer capture evidence.

## License

This project is released under the [MIT License](LICENSE). Copyright (c) 2026 Bugra Postaci.
