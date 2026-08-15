# Trap38_MostVexingParse - CDB debug analysis

**Question:** did `Timer t();` create a `Timer` object?

**Short answer:** no. Anything that can be parsed as a declaration is parsed as a declaration, so `Timer t();` declares a function named `t` returning `Timer`. MSVC warning C4930 is the primary evidence. CDB then confirms no local `t` object exists. No sanitizer reports this trap.

---

## 1. Read the source first

```cpp
 5  struct Timer {
 6      int ticks = 7;
 7      Timer() = default;
 8      explicit Timer(int value) : ticks(value) {}
19      Timer t(); // declares a function 't' returning Timer - no object is created.
26      Timer braced{};   // real object.
27      Timer plain;      // real object.
37      std::vector<char> from_iterators(source.begin(), source.end());
38      std::vector<char> braced_form{source.begin(), source.end()};
45      std::vector<int> parens(3, 0); // Three elements, all zero.
46      std::vector<int> braces{3, 0}; // Two elements: 3 and 0.
```

| Entity | Role |
|---|---|
| `Timer t();` | Function declaration. |
| `Timer braced{}` / `Timer plain` | Real local objects. |
| `vector<int> parens(3,0)` | Size constructor: `0,0,0`. |
| `vector<int> braces{3,0}` | Initializer-list constructor: `3,0`. |

---

## 2. Build and compiler warning

The repository suppresses C4930 locally so the normal target can build with warnings-as-errors. A scratch copy with only that suppression removed produced this real warning without touching `build\`:

```text
h2_trap38_warning.cpp
C:\Temp\h2_trap38_warning.cpp(18): warning C4930: 'Timer t(void)': prototyped function not called (was a variable definition intended?)
```

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\ObjectModel\Trap38_MostVexingParse -cf %TEMP%\h2_trap38.cdb C:\src\cpptraps42\build\cdb\Debug\Trap38_MostVexingParse.exe
```

`.symopt-100` and `.lines -e` must be at the top of the script.

---

## 4. Transcript: no local `t`, vector contents differ

```text
0:000> .symopt-100
0:000> .lines -e
0:000> x Trap38_MostVexingParse!*empty_parens*
*** WARNING: Unable to verify checksum for Trap38_MostVexingParse.exe
00007ff6`71d81d80 Trap38_MostVexingParse!empty_parens_declare_a_function (void)
0:000> bp `main.cpp:26`
0:000> bp `main.cpp:47`
0:000> g
Breakpoint 0 hit
Trap38_MostVexingParse!empty_parens_declare_a_function+0x1a:
00007ff6`71d81d9a 488d442424      lea     rax,[rsp+24h]
0:000> $$ ===== STOP 1 after Timer t declaration =====
0:000> dv /t /v
0000003e`fa8ffad4 struct Timer plain = struct Timer
0000003e`fa8ffab4 struct Timer braced = struct Timer
0:000> ?? t
*************************************************************************
***                                                                   ***
***                                                                   ***
***    Your debugger is not using the correct symbols                 ***
***                                                                   ***
***    In order for this command to work properly, your symbol path   ***
***    must point to .pdb files that have full type information.      ***
***                                                                   ***
***    Certain .pdb files (such as the public OS symbols) do not      ***
***    contain the required information.  Contact the group that      ***
***    provided you with these symbols if you need this command to    ***
***    work.                                                          ***
***                                                                   ***
***    Type referenced: t                                             ***
***                                                                   ***
*************************************************************************
Couldn't resolve error at 't'
0:000> g
Breakpoint 1 hit
Trap38_MostVexingParse!braces_have_their_own_rule+0xf3:
00007ff6`71d821c3 488d15deed0000  lea     rdx,[Trap38_MostVexingParse!std::_Digit_pairs<wchar_t>+0x308 (00007ff6`71d90fa8)]
0:000> $$ ===== STOP 2 vector parentheses versus braces =====
0:000> dv /t /v
0000003e`fa8ffa28 class std::vector<int,std::allocator<int> > braces = { size=0x2 }
0000003e`fa8ff9e8 class std::vector<int,std::allocator<int> > parens = { size=0x3 }
0000003e`fa8ffa74 struct Timer sized = struct Timer
0:000> dx -r1 parens
parens           : { size=0x3 } [Type: std::vector<int,std::allocator<int> >]
    [<Raw View>]     [Type: std::vector<int,std::allocator<int> >]
    [capacity]       : 0x3 [Type: size_t]
    [allocator]      : allocator [Type: std::_Compressed_pair<std::allocator<int>,std::_Vector_val<std::_Simple_types<int> >,1>]
    [0]              : 0 [Type: int]
    [1]              : 0 [Type: int]
    [2]              : 0 [Type: int]
0:000> dx -r1 braces
braces           : { size=0x2 } [Type: std::vector<int,std::allocator<int> >]
    [<Raw View>]     [Type: std::vector<int,std::allocator<int> >]
    [capacity]       : 0x2 [Type: size_t]
    [allocator]      : allocator [Type: std::_Compressed_pair<std::allocator<int>,std::_Vector_val<std::_Simple_types<int> >,1>]
    [0]              : 3 [Type: int]
    [1]              : 0 [Type: int]
0:000> g
vexing: braced=7 plain=7
iterators size=5 braced size=5
parens size=3 braces size=2
sized ticks=3
```

---

## 5. Why there is no unsafe target

`Trap38_MostVexingParse_unsafe.exe` does not exist. The trap is a grammar rule resolved before execution. `Timer t();` creates no object and performs no invalid access, so AddressSanitizer has nothing to diagnose.

---

## 6. What the measurements prove

| Measurement | Evidence |
|---|---|
| Compiler warning | C4930 says `Timer t(void)` is a prototype. |
| `dv /t /v` at line 26 | `braced` and `plain` are locals; `t` is absent. |
| `?? t` | CDB cannot resolve a local object named `t`. |
| `dx parens` | Size 3, elements `0,0,0`. |
| `dx braces` | Size 2, elements `3,0`. |

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `cl /c /W4 ... h2_trap38_warning.cpp` | Capture the real C4930 warning from a scratch copy. |
| `` bp `main.cpp:26` `` | Stop after the declaration and before real objects are printed. |
| `dv /t /v` | List real local objects. |
| `?? t` | Attempt to evaluate the supposed object. |
| `dx -r1 parens` | Inspect vector contents through NatVis. |

---

## 8. Left to you

1. Change a scratch copy to `Timer t{};`. Does C4930 disappear, and does `t` appear in `dv`?
2. Add `std::vector<int> single{3};`. What does `dx` show?
3. Replace named iterator variables with iterator temporaries. Which syntax becomes a declaration candidate?
4. Try `auto t = Timer();`. Compare the compiler warning and local list.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| MSVC C4930 | Yes | Only visible when not suppressed. |
| CDB locals | Yes | Shows compiled result, not alternate parses. |
| AddressSanitizer | No | No runtime invalid access. |
| Runtime output | Partly | Only objects that actually exist can print. |
