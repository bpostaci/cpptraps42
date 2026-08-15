# Trap36_HiddenOverload - CDB debug analysis

**Question:** why did `h.log(1)` call the `double` overload when `Base::log(int)` exists?

**Short answer:** a declaration named `log` in `Hiding` hides every base declaration named `log`. The program is well formed; the evidence is the resolved function name in the CDB call stack, not a crash. No sanitizer reports this trap.

---

## 1. Read the source first

```cpp
 5  struct Base {
 6      void log(int value) { std::cout << "Base::log(int) " << value << '\n'; }
 7      void log(const std::string& text) { std::cout << "Base::log(string) " << text << '\n'; }
10  struct Hiding : Base {
11      void log(double value) { std::cout << "Hiding::log(double) " << value << '\n'; }
17      h.log(1); // int converts to double and calls Hiding::log(double).
19      h.Base::log(1); // Qualification reaches the hidden base overload explicitly.
23  struct Exposing : Base {
24      using Base::log;
25      void log(double value) { std::cout << "Exposing::log(double) " << value << '\n'; }
30      e.log(1);        // Exact match wins: Base::log(int).
31      e.log(1.5);      // Exposing::log(double).
32      e.log("text");   // Base::log(const string&) is visible again.
43      void draw(int layer) const override { std::cout << "Circle::draw layer=" << layer << '\n'; }
49      s.draw(3); // Dispatches to Circle::draw because the signature matches exactly.
```

| Entity | Role |
|---|---|
| `Base::log(int)`, `Base::log(string)` | Hidden until explicitly qualified or re-exposed. |
| `Hiding::log(double)` | Hides the base overload set. |
| `using Base::log` | Re-introduces base overloads into `Exposing`. |
| `Circle::draw(int) const override` | Compile-time protection against signature drift. |

---

## 2. Build and compiler evidence

The existing executable `build\cdb\Debug\Trap36_HiddenOverload.exe` was used. MSVC need not warn: these calls are well formed. If the commented `draw(long) const override` were enabled, compilation would fail before a debugger session.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload -cf %TEMP%\h2_trap36.cdb C:\src\cpptraps42\build\cdb\Debug\Trap36_HiddenOverload.exe
```

The script starts with `.symopt-100` and `.lines -e`; without them, unqualified locals and source-line breakpoints are unreliable.

---

## 4. Transcript: resolved overloads

```text
0:000> .symopt-100
0:000> .lines -e
0:000> x Trap36_HiddenOverload!*log*
*** WARNING: Unable to verify checksum for Trap36_HiddenOverload.exe
00007ff6`cb1f4ed0 Trap36_HiddenOverload!Exposing::log (double)
00007ff6`cb1f4e20 Trap36_HiddenOverload!Base::log (class std::basic_string<char,std::char_traits<char>,std::allocator<char> > *)
00007ff6`cb1f4e70 Trap36_HiddenOverload!Base::log (int)
00007ff6`cb1f4f30 Trap36_HiddenOverload!Hiding::log (double)
0:000> bp `main.cpp:11`
0:000> bp `main.cpp:6`
0:000> bp `main.cpp:7`
0:000> bp `main.cpp:25`
0:000> bp `main.cpp:43`
0:000> g
Breakpoint 0 hit
Trap36_HiddenOverload!Hiding::log:
00007ff6`cb1f4f30 f20f114c2410    movsd   mmword ptr [rsp+10h],xmm1 ss:00000028`b84ffa18=00007ffc429df6ab
0:000> $$ ===== STOP 1: h.log(1) resolved after hiding =====
0:000> k
Child-SP          RetAddr               Call Site
00000028`b84ffa08 00007ff6`cb1f1a19     Trap36_HiddenOverload!Hiding::log [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 11]
00000028`b84ffa10 00007ff6`cb1f1b7b     Trap36_HiddenOverload!name_hiding_changes_overload_resolution+0x29 [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 19]
0:000> g
Breakpoint 1 hit
Trap36_HiddenOverload!Base::log:
00007ff6`cb1f4e70 89542410        mov     dword ptr [rsp+10h],edx ss:00000028`b84ffa18=00000000
0:000> $$ ===== STOP 2: h.Base::log(1) explicit qualification =====
0:000> k
Child-SP          RetAddr               Call Site
00000028`b84ffa08 00007ff6`cb1f1a28     Trap36_HiddenOverload!Base::log [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 6]
00000028`b84ffa10 00007ff6`cb1f1b7b     Trap36_HiddenOverload!name_hiding_changes_overload_resolution+0x38 [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 19]
0:000> g
Breakpoint 1 hit
Trap36_HiddenOverload!Base::log:
00007ff6`cb1f4e70 89542410        mov     dword ptr [rsp+10h],edx ss:00000028`b84ff9e8=42a32a4e
0:000> $$ ===== STOP 3: e.log(1) with using-declaration =====
0:000> k
Child-SP          RetAddr               Call Site
00000028`b84ff9d8 00007ff6`cb1f1a85     Trap36_HiddenOverload!Base::log [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 6]
00000028`b84ff9e0 00007ff6`cb1f1b80     Trap36_HiddenOverload!using_declaration_restores_the_set+0x35 [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 31]
0:000> g
Breakpoint 3 hit
Trap36_HiddenOverload!Exposing::log:
00007ff6`cb1f4ed0 f20f114c2410    movsd   mmword ptr [rsp+10h],xmm1 ss:00000028`b84ff9e8=00007ffc00000001
0:000> $$ ===== STOP 4: e.log(1.5) derived overload =====
0:000> k
Child-SP          RetAddr               Call Site
00000028`b84ff9d8 00007ff6`cb1f1a97     Trap36_HiddenOverload!Exposing::log [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 25]
00000028`b84ff9e0 00007ff6`cb1f1b80     Trap36_HiddenOverload!using_declaration_restores_the_set+0x47 [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 31]
0:000> g
Breakpoint 2 hit
Trap36_HiddenOverload!Base::log:
00007ff6`cb1f4e20 4889542410      mov     qword ptr [rsp+10h],rdx ss:00000028`b84ff9e8=00007ff6cb1fdbb0
0:000> $$ ===== STOP 5: e.log("text") base string overload =====
0:000> k
Child-SP          RetAddr               Call Site
00000028`b84ff9d8 00007ff6`cb1f1ab9     Trap36_HiddenOverload!Base::log [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 7]
00000028`b84ff9e0 00007ff6`cb1f1b80     Trap36_HiddenOverload!using_declaration_restores_the_set+0x69 [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 32]
0:000> g
Breakpoint 4 hit
Trap36_HiddenOverload!Circle::draw:
00007ff6`cb1f4ce0 89542410        mov     dword ptr [rsp+10h],edx ss:00000028`b84ffa08=cccccccc
0:000> $$ ===== STOP 6: s.draw(3) virtual override =====
0:000> k
Child-SP          RetAddr               Call Site
00000028`b84ff9f8 00007ff6`cb1f1b3b     Trap36_HiddenOverload!Circle::draw [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 43]
00000028`b84ffa00 00007ff6`cb1f1b85     Trap36_HiddenOverload!override_keyword_catches_mismatch+0x4b [C:\src\cpptraps42\ObjectModel\Trap36_HiddenOverload\main.cpp @ 49]
```

---

## 5. Why there is no unsafe target

`Trap36_HiddenOverload_unsafe.exe` does not exist. The safe and contrasting cases are all compile-time overload-resolution examples in one executable. No memory is corrupted, and no sanitizer applies.

---

## 6. What the measurements prove

| Call | Resolved body | Meaning |
|---|---|---|
| `h.log(1)` | `Hiding::log(double)` | Base overload set was hidden before overload resolution. |
| `h.Base::log(1)` | `Base::log(int)` | Qualification reaches the hidden base function. |
| `e.log(1)` | `Base::log(int)` | `using Base::log` restored the exact match. |
| `e.log("text")` | `Base::log(string)` | The string overload is visible again. |
| `s.draw(3)` | `Circle::draw` | `override` confirms the virtual signature. |

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `x Trap36_HiddenOverload!*log*` | Enumerate overload symbols. |
| `` bp `main.cpp:11` `` | Stop in the selected function body. |
| `k` | Read the resolved function name. |
| `g` | Continue to the next call variant. |

---

## 8. Left to you

1. Compile a scratch copy with the commented `draw(long) const override` enabled. What exact diagnostic appears?
2. Remove `using Base::log` from `Exposing` in a scratch copy. Which calls stop compiling?
3. Add `e.log(1.0f)` and inspect the stack. Which overload wins?
4. Add `h.log(std::string{"text"})`. Does name hiding still block the base string overload?

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB call stack | Yes | Shows the selected body, not programmer intent. |
| Compiler | Partly | Catches `override` mismatches, but ordinary hiding may be silent. |
| AddressSanitizer | No | There is no invalid memory access. |
| Release build | No | Same overload selection, less symbolic detail. |
