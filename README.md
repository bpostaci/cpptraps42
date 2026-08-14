# C++ Traps - Visual Studio Training Solution

Each applicable trap is a separate executable project and lives in its own clearly named folder. Open `VisualStudio/CppTrapsCode.sln` for the normal Visual Studio experience, or open this root folder as a CMake project. Both use the same source files.

## Visual Studio

1. Open `VisualStudio/CppTrapsCode.sln`.
2. In Solution Explorer, expand `Memory`, `Lifetime`, `ObjectModel`, `Concurrency`, or `ABI_Build`.
3. Right-click one trap project and choose **Set as Startup Project**.
4. Search its `main.cpp` for `BP:` and set the suggested breakpoints.
5. Build/run with `F5`.

Broken operations are wrapped in `RUN_UNSAFE_EXAMPLE`. To activate one intentionally unsafe branch, open that project's properties and add `RUN_UNSAFE_EXAMPLE` under **C/C++ > Preprocessor > Preprocessor Definitions** for Debug only. Prefer enabling AddressSanitizer as well.

## CMake

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

The CMake-generated solution also groups targets by the same topic folders.

## Safety

Never enable unsafe branches in production. Undefined behavior may crash, appear to work, or change with optimization. The purpose is to stop immediately before the invalid operation, inspect the lifetime/bounds/ownership invariant, and then let the relevant sanitizer capture evidence.

