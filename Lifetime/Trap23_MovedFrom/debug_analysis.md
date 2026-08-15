# Trap23_MovedFrom - CDB debug analysis

**Question:** after `std::move`, can the source still be read as if it kept its old value?

**Short answer:** no. A moved-from object is still valid where the type contract says so, but its value is often unspecified. `std::unique_ptr` is the important exception here: after a successful move, the source is guaranteed to be null.

---

## 1. Read the source first

`main.cpp` (45 lines). The `BP:` comments mark the lines that matter.

```cpp
 1  #include <iostream>
 2  #include <memory>
 3  #include <string>
 4  #include <vector>
 5
 6  // Type 1: a moved-from standard container is valid but its value is unspecified.
 7  void moved_from_container() {
 8      std::string source = "payload";
 9      std::string destination = std::move(source); // BP: source valid, state unspecified.
10  #if defined(RUN_UNSAFE_EXAMPLE)
11      std::cout << "container: assuming old content: " << source << '\n'; // BP: unspecified value.
12  #else
13      source.clear(); // Establish a known state before semantic reuse.
14      std::cout << "container: " << destination << ", source.size=" << source.size() << '\n';
15  #endif
16  }
17
18  // Type 2: a moved-from unique_ptr is guaranteed null - dereferencing it is undefined.
19  void moved_from_owner() {
20      auto source = std::make_unique<int>(7);
21      auto destination = std::move(source); // BP: source is now null by contract.
22  #if defined(RUN_UNSAFE_EXAMPLE)
23      std::cout << "owner: " << *source << '\n'; // BP: null dereference.
24  #else
25      std::cout << "owner: " << *destination << ", source empty=" << (source == nullptr) << '\n';
26  #endif
27  }
28
29  // Type 3: self-move leaves the object in a valid but unspecified state.
30  void self_move() {
31      std::vector<int> values{1,2,3};
32  #if defined(RUN_UNSAFE_EXAMPLE)
33      values = std::move(values); // BP: self-move; contents become unspecified.
34      std::cout << "self: size=" << values.size() << '\n'; // BP: relies on an unspecified state.
35  #else
36      std::cout << "self: size=" << values.size() << " (self-move avoided)\n";
37  #endif
38  }
39
40  int main() {
41      moved_from_container();
42      moved_from_owner();
43      self_move();
44  }
```

| Variant | Entity to inspect | Contract being tested |
|---|---|---|
| moved string | `source`, `destination` | source is valid, value unspecified |
| moved `unique_ptr` | raw pointer inside `source` | source is guaranteed null |
| self-move | `values` | valid, but do not assume a result |

---

## 2. Build without a sanitizer

For this capture I did not build or clean anything. I used the already-built debugger targets in `build\cdb\Debug`:

```powershell
C:\src\cpptraps42\build\cdb\Debug\Trap23_MovedFrom.exe
C:\src\cpptraps42\build\cdb\Debug\Trap23_MovedFrom_unsafe.exe
```

AddressSanitizer is not the point of this trap. The moved string and self-move examples are not undefined behavior. The bad `unique_ptr` branch is undefined only when the null pointer is dereferenced; CDB lets us stop before that operation and inspect the ownership state.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
    -srcpath C:\src\cpptraps42\Lifetime\Trap23_MovedFrom ^
    C:\src\cpptraps42\build\cdb\Debug\Trap23_MovedFrom.exe
```

Then enable the two settings that are off by default:

```
.symopt-100     resolve local names such as source and destination
.lines -e       make source-line breakpoints such as bp `main.cpp:25` work
```

---

## 4. The safe target

The safe target stops after each move and reads state before the safe repair or print.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:13`
0:000> bp `main.cpp:25`
0:000> bp `main.cpp:36`
0:000> g
Breakpoint 0 hit
Trap23_MovedFrom!moved_from_container+0x4e:
00007ff6`42751cae 488d4c2428      lea     rcx,[rsp+28h]
0:000> $$ ===== variant 1 safe: moved string, before clear() =====
0:000> dv /t /v
00000098`4336fda8 class std::basic_string<char,std::char_traits<char>,std::allocator<char> > destination = "payload"
00000098`4336fd68 class std::basic_string<char,std::char_traits<char>,std::allocator<char> > source = ""
0:000> g
Breakpoint 1 hit
Trap23_MovedFrom!moved_from_owner+0x42:
00007ff6`42751db2 488d15d7e10000  lea     rdx,[Trap23_MovedFrom!std::_Digit_pairs<wchar_t>+0x2f0 (00007ff6`4275ff90)]
0:000> $$ ===== variant 2 safe: moved unique_ptr =====
0:000> dv /t /v
00000098`4336fdc8 class std::unique_ptr<int,std::default_delete<int> > destination = unique_ptr 7
00000098`4336fda8 class std::unique_ptr<int,std::default_delete<int> > source = empty
0:000> dqs @@c++(&source) L1
00000098`4336fda8  00000000`00000000
0:000> dqs @@c++(&destination) L1
00000098`4336fdc8  00000296`90420150
0:000> g
Breakpoint 2 hit
Trap23_MovedFrom!self_move+0xbb:
00007ff6`42751f1b 488d158ee00000  lea     rdx,[Trap23_MovedFrom!std::_Digit_pairs<wchar_t>+0x310 (00007ff6`4275ffb0)]
0:000> $$ ===== variant 3 safe: self-move avoided =====
0:000> dv /t /v
00000098`4336fd48 class std::vector<int,std::allocator<int> > values = { size=0x3 }
0:000> g
container: payload, source.size=0
owner: 7, source empty=1
self: size=3 (self-move avoided)
```

---

## 5. The unsafe target

The `_unsafe` target is a contrast target. I stopped before the null `unique_ptr` dereference and moved the instruction pointer to the function epilogue so the same CDB run could also reach the self-move variant. That skip is not evidence about normal execution; the measurements before it are.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:11`
0:000> bp `main.cpp:23`
0:000> bp `main.cpp:34`
0:000> g
Breakpoint 0 hit
Trap23_MovedFrom_unsafe!moved_from_container+0x4e:
00007ff6`82b81cae 488d15abe20000  lea     rdx,[Trap23_MovedFrom_unsafe!std::_Digit_pairs<wchar_t>+0x2c0 (00007ff6`82b8ff60)]
0:000> $$ ===== variant 1 unsafe: assuming moved string content =====
0:000> dv /t /v
0000009e`151cf9e8 class std::basic_string<char,std::char_traits<char>,std::allocator<char> > destination = "payload"
0000009e`151cf9a8 class std::basic_string<char,std::char_traits<char>,std::allocator<char> > source = ""
0:000> g
Breakpoint 1 hit
Trap23_MovedFrom_unsafe!moved_from_owner+0x3f:
00007ff6`82b81d5f 488d1522e20000  lea     rdx,[Trap23_MovedFrom_unsafe!std::_Digit_pairs<wchar_t>+0x2e8 (00007ff6`82b8ff88)]
0:000> $$ ===== variant 2 unsafe: moved unique_ptr before null dereference =====
0:000> dv /t /v
0000009e`151cfa08 class std::unique_ptr<int,std::default_delete<int> > destination = unique_ptr 7
0000009e`151cf9e8 class std::unique_ptr<int,std::default_delete<int> > source = empty
0:000> dqs @@c++(&source) L1
0000009e`151cf9e8  00000000`00000000
0:000> dqs @@c++(&destination) L1
0000009e`151cfa08  0000028b`0b2e9930
0:000> r @rip=Trap23_MovedFrom_unsafe!moved_from_owner+0x81
0:000> g
Breakpoint 2 hit
Trap23_MovedFrom_unsafe!self_move+0xca:
00007ff6`82b81eaa 488d15dfe00000  lea     rdx,[Trap23_MovedFrom_unsafe!std::_Digit_pairs<wchar_t>+0x2f0 (00007ff6`82b8ff90)]
0:000> $$ ===== variant 3 unsafe: after self-move =====
0:000> dv /t /v
0000009e`151cf978 class std::vector<int,std::allocator<int> > values = { size=0x3 }
0:000> g
container: assuming old content: 
self: size=3
```

---

## 6. What the measurements prove

| Variant | Before/after event | Measurement | Lesson |
|---|---|---|---|
| moved string | after move | `destination = "payload"`, `source = ""` in this run | the source is valid; the empty string is an implementation result, not a portable promise |
| moved `unique_ptr` | after move | source raw pointer `00000000`, destination raw pointer non-null | this null state is guaranteed by `unique_ptr` move |
| self-move | after `values = std::move(values)` | `values = { size=0x3 }` in this run | the object is valid, but code must not rely on that size |

The divergence between the two `unique_ptr` raw pointers is the hard guarantee. The string and vector observations are deliberately weaker: they show what MSVC did in this Debug run, not a contract to write code against.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `.symopt-100` | allow unqualified C++ local names |
| `.lines -e` | enable source-line breakpoints |
| `` bp `main.cpp:23` `` | stop before the null dereference |
| `dv /t /v` | show local variables with type and debugger value |
| `dqs @@c++(&source) L1` | read the raw pointer stored in a `unique_ptr` object |
| `r @rip=...` | skip the dangerous dereference after measuring it, only to reach the next variant |
| `g` | continue to the next stop |
| `q` | quit the debugger |

---

## 8. Left to you

1. Remove the `r @rip=...` skip in a private run of the unsafe target. Where does the null dereference stop?
2. Repeat the string move in Release. Does the debugger still display the same moved-from string?
3. Replace `std::string` with a larger string that does not fit the small-string buffer. Which displayed addresses change?
4. Add a guard `if (source)` before the unsafe `unique_ptr` read. Which CDB measurement proves the branch is not taken?
5. Try the self-move with a different standard-library implementation and compare only contractual facts, not incidental sizes.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by inspecting state after the move | shows one implementation result for unspecified states |
| MSVC AddressSanitizer | only the null dereference if executed | does not diagnose valid-but-unspecified assumptions |
| Compiler warnings | not reliably | moved-from use is often well formed |
| Release build | no | fewer locals and different library behavior may hide the evidence |
