# C++ Traps - Visual Studio Training Solution

Each of the 30 traps is a separate executable project and lives in its own clearly named folder. Open `VisualStudio/CppTrapsCode.slnx` for the normal Visual Studio experience, or open this root folder as a CMake project. Both use the same source files.

## Visual Studio

1. Open `VisualStudio/CppTrapsCode.slnx`.
2. In Solution Explorer, expand `Memory`, `Lifetime`, `ObjectModel`, `Concurrency`, or `ABI_Build`.
3. Right-click one trap project and choose **Set as Startup Project**.
4. Search its `main.cpp` for `BP:` and set the suggested breakpoints.
5. Build/run with `F5`.

Projects that contain an intentionally broken operation have a separately generated target ending in `_unsafe`. For example, use `Trap01_UseAfterFree` to study the safe path and `Trap01_UseAfterFree_unsafe` to enter the guarded invalid access. Do not add `RUN_UNSAFE_EXAMPLE` manually; CMake defines it only for these explicit unsafe targets. Prefer an AddressSanitizer configuration for memory-unsafe targets.

## CMake

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

The CMake-generated solution also groups targets by the same topic folders.

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
