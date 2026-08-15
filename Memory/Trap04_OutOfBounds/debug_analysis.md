# Trap04_OutOfBounds - CDB debug analysis

**Question:** if `data()+index` can be formed, may the program read through it?

**Short answer:** no. One-past-the-end is a valid sentinel address, not an element. The debugger session separates address arithmetic from dereference permission.

---

## 1. Read the source first

```cpp
 5  void unchecked_subscript() {
 6      std::array<int, 4> values{10,20,30,40};
 7      const std::size_t index = 4;
 9      std::cout << "subscript: " << values[index] << '\n'; // unsafe BP
11      if (index < values.size()) std::cout << "subscript: " << values[index] << '\n';
12      else std::cout << "subscript: rejected index " << index << '\n';
17  void off_by_one_loop() {
18      std::array<int, 4> values{10,20,30,40};
21      for (std::size_t i = 0; i <= values.size(); ++i) sum += values[i]; // unsafe BP
23      for (std::size_t i = 0; i < values.size(); ++i) sum += values[i];
29  void decayed_array_size(const int* data) {
31      const std::size_t count = sizeof(data) / sizeof(data[0]); // unsafe BP
37      std::cout << "decay: pass the size explicitly or use std::span/std::array\n";
```

| Entity | Role |
|---|---|
| `values._Elems[0]` | debugger-visible start of the four-element array |
| `index` / `i` | candidate subscript |
| `data` | decayed pointer; the callee no longer knows the original bound |

---

## 2. Build without a sanitizer

```powershell
cmake -S . -B build\cdb -DTRAPS_BUILD_UNSAFE=ON -DTRAPS_SANITIZER=none
cmake --build build\cdb --config Debug --target Trap04_OutOfBounds Trap04_OutOfBounds_unsafe
```

AddressSanitizer is the better crash-class tool for real bounds bugs. This CDB lab is kept unsanitized so the standard library debug checks and addresses remain easy to inspect.

---

## 3. Start CDB

Use `-y` for the PDB directory and `-srcpath` for this trap folder, then run `.symopt-100` and `.lines -e` before setting source-line breakpoints.

---

## 4. The safe target

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:11`
0:000> bp `main.cpp:25`
0:000> bp `main.cpp:37`
0:000> g
Breakpoint 0 hit
Trap04_OutOfBounds!unchecked_subscript+0x52:
0:000> $$ ===== unchecked_subscript: safe branch rejects index == size =====
0:000> dv /t /v
000000ed`1acffb98 unsigned int64 index = 4
000000ed`1acffb78 class std::array<int,4> values = { size=4 }
0:000> ?? index
unsigned int64 4
0:000> ?? &values._Elems[0]
int * 0x000000ed`1acffb78
0:000> ?? &values._Elems[0]+index
int * 0x000000ed`1acffb88
0:000> g
Breakpoint 1 hit
Trap04_OutOfBounds!off_by_one_loop+0x96:
0:000> $$ ===== off_by_one_loop: loop stopped before one-past =====
0:000> dv /t /v
000000ed`1acffb88 class std::array<int,4> values = { size=4 }
000000ed`1acffba4 int sum = 0n100
0:000> ?? sum
int 0n100
0:000> g
Breakpoint 2 hit
Trap04_OutOfBounds!decayed_array_size+0xa:
0:000> $$ ===== decayed_array_size: safe branch refuses to infer a bound =====
0:000> dv /t /v
000000ed`1acffbe0 int * data = 0x000000ed`1acffc08
0:000> ?? data
int * 0x000000ed`1acffc08
0:000> g
subscript: rejected index 4
loop: sum=100
decay: pass the size explicitly or use std::span/std::array
```

---

## 5. The unsafe target

Unchecked subscript:

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:9`
0:000> g
Breakpoint 0 hit
Trap04_OutOfBounds_unsafe!unchecked_subscript+0x4f:
0:000> $$ ===== unchecked_subscript: index equals size =====
0:000> dv /t /v
0000008f`490ff688 unsigned int64 index = 4
0000008f`490ff668 class std::array<int,4> values = { size=4 }
0:000> ?? index
unsigned int64 4
0:000> ?? &values._Elems[0]
int * 0x0000008f`490ff668
0:000> ?? &values._Elems[0]+index
int * 0x0000008f`490ff678
```

Off-by-one loop, stopped by the MSVC debug library at the first illegal element:

```
0:000> bp Trap04_OutOfBounds_unsafe!main+0x26
0:000> bp `main.cpp:21`
0:000> g
Breakpoint 0 hit
Trap04_OutOfBounds_unsafe!main+0x26:
0:000> r rip = Trap04_OutOfBounds_unsafe!main+0x2b
0:000> g
Debug Assertion Failed!
File: C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\include\array
Line: 525
Expression: array subscript out of range
Trap04_OutOfBounds_unsafe!std::array<int,4>::operator[]+0x57:
0:000> dv /t /v
000000b7`90affb50 class std::array<int,4> * this = 0x000000b7`90affb78 { size=4 }
000000b7`90affb58 unsigned int64 _Pos = 4
```

Lost extent:

```
0:000> bp Trap04_OutOfBounds_unsafe!main+0x26
0:000> bp `main.cpp:31`
0:000> g
Breakpoint 0 hit
Trap04_OutOfBounds_unsafe!main+0x26:
0:000> r rip = Trap04_OutOfBounds_unsafe!main+0x30
0:000> g
Breakpoint 1 hit
Trap04_OutOfBounds_unsafe!decayed_array_size+0xa:
0:000> $$ ===== decayed_array_size: callee sees only a pointer =====
0:000> dv /t /v
0000008f`da6ff980 int * data = 0x0000008f`da6ff9a8
0000008f`da6ff940 unsigned int64 count = 0
0000008f`da6ff948 int sum = 0n1117989454
0:000> ?? sizeof(data)
unsigned int64 8
0:000> ?? sizeof(data[0])
unsigned int64 4
0:000> ?? sizeof(data) / sizeof(data[0])
unsigned int64 2
0:000> dd @@c++(data) L4
0000008f`da6ff9a8  0000000a 00000014 0000001e 00000028
0:000> g
decay: count=2 sum=30
```

---

## 6. What the measurements prove

| Variant | Measurement | Interpretation |
|---|---|---|
| unchecked subscript | `index == 4`, size is 4, address is `base + 4` | one-past can be formed but not read |
| off-by-one | library reports `_Pos = 4` for `{ size=4 }` | `<=` admits the first illegal iteration |
| lost extent | `sizeof(data) == 8`, `sizeof(data[0]) == 4`, count becomes 2 | the pointer-taking function cannot recover the original bound |

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `?? &values._Elems[0]` | debugger-visible address of the first `std::array` element |
| `?? &values._Elems[0]+index` | compute the one-past address without dereferencing it |
| `dv /t /v` | show the library's `_Pos` and size view |
| `dd addr L4` | show four `int` elements as raw DWORDs |
| `r rip = ...` | skip earlier unsafe variants so each demonstration can be reached |

---

## 8. Left to you

1. Replace `std::array` with a raw array in the unsafe target and compare the debug assertion evidence.
2. Break before the off-by-one loop body and record `i == 3` and then `i == 4` in one run.
3. Change the raw array to five elements and see whether `sizeof(data) / sizeof(data[0])` changes in the callee.
4. Rewrite the pointer-taking function to accept `std::span<const int>` and inspect the stored extent.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, with addresses and library checks | does not by itself prove all possible bounds |
| MSVC debug STL | yes for `std::array::operator[]` here | Debug library feature, not the C++ rule itself |
| AddressSanitizer | yes for many real out-of-bounds reads | changes code generation and allocator layout |
| Compiler warnings | sometimes | generally cannot know runtime indices |
