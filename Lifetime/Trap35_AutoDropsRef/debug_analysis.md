# Trap35_AutoDropsRef - CDB debug analysis

**Question:** when a function or loop gives you a reference, does plain `auto` keep that reference?

**Short answer:** no. Plain `auto` deduces a value type. The behavior is fully defined, but it may silently copy a large object and send later writes to the copy.

---

## 1. Read the source first

```cpp
10  static Heavy& shared_instance() {
11      static Heavy instance{"large-payload", 0};
12      return instance;
13  }
16  void auto_copies_a_reference() {
17      auto copy = shared_instance(); // BP: deduced as Heavy, not Heavy& - a full copy is made.
18      copy.hits = 42;                // BP: mutates the copy; the shared object is untouched.
19      std::cout << "copy hits=" << copy.hits << " shared hits=" << shared_instance().hits << '\n';
21      auto& reference = shared_instance(); // 'auto&' keeps the binding to the original.
22      reference.hits = 42;
23      std::cout << "after auto&: shared hits=" << shared_instance().hits << '\n';
24  }
27  void range_for_copies_elements() {
28      std::vector<Heavy> items(3, Heavy{"element", 0});
29      for (auto item : items) { item.hits = 1; } // BP: each iteration copies and discards.
30      std::cout << "by value: items[0].hits=" << items[0].hits << '\n';
32      for (auto& item : items) { item.hits = 1; } // Mutating loop: bind by reference.
33      std::cout << "by ref  : items[0].hits=" << items[0].hits << '\n';
40  void keeping_the_exact_type() {
41      Heavy& source = shared_instance();
42      decltype(auto) exact = shared_instance(); // Deduces Heavy&, preserving the reference.
43      exact.hits = 7;
44      std::cout << "decltype(auto) shared hits=" << source.hits << '\n';
```

| Variant | Copying form | Correct form |
|---|---|---|
| function returning reference | `auto copy` | `auto& reference` |
| range-for | `for (auto item : items)` | `for (auto& item : items)` |
| exact return semantics | plain `auto` would copy | `decltype(auto)` preserves `Heavy&` |

---

## 2. Build without a sanitizer

I used the existing `Trap35_AutoDropsRef.exe`. There is no `_unsafe` sibling. This trap is fully defined behavior: the unwanted copy is legal, so no runtime sanitizer is expected to complain.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
    -srcpath C:\src\cpptraps42\Lifetime\Trap35_AutoDropsRef ^
    C:\src\cpptraps42\build\cdb\Debug\Trap35_AutoDropsRef.exe
```

Use `.symopt-100` for local names and `.lines -e` for source breakpoints.

---

## 4. The safe target

All variants live in the normal target. For the one-line range loops, the address breakpoints stop inside the generated loop body so the hidden loop variable can be compared with the vector element.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:18`
0:000> bp `main.cpp:22`
0:000> bp Trap35_AutoDropsRef!range_for_copies_elements+0xfa
0:000> bp `main.cpp:30`
0:000> bp Trap35_AutoDropsRef!range_for_copies_elements+0x1d5
0:000> bp `main.cpp:33`
0:000> bp `main.cpp:43`
0:000> g
Breakpoint 0 hit
Trap35_AutoDropsRef!auto_copies_a_reference+0x3f:
00007ff7`f8571c3f c74424502a000000 mov     dword ptr [rsp+50h],2Ah ss:00000053`af1dfcf0=00000000
0:000> $$ ===== variant 1 after plain auto copy =====
0:000> dv /t /v
00000053`af1dfd08 struct Heavy * reference = 0xcccccccc`cccccccc
00000053`af1dfcc8 struct Heavy copy = struct Heavy
0:000> ?? &copy
struct Heavy * 0x00000053`af1dfcc8
   +0x000 payload          : std::basic_string<char,std::char_traits<char>,std::allocator<char> >
   +0x028 hits             : 0n0
0:000> g
Breakpoint 1 hit
Trap35_AutoDropsRef!auto_copies_a_reference+0xbd:
00007ff7`f8571cbd 488b442468      mov     rax,qword ptr [rsp+68h] ss:00000053`af1dfd08={Trap35_AutoDropsRef!instance (00007ff7`f8585518)}
0:000> $$ ===== variant 1 auto reference binds original =====
0:000> dv /t /v
00000053`af1dfd08 struct Heavy * reference = 0x00007ff7`f8585518
00000053`af1dfcc8 struct Heavy copy = struct Heavy
0:000> ?? reference
struct Heavy * 0x00007ff7`f8585518
   +0x000 payload          : std::basic_string<char,std::char_traits<char>,std::allocator<char> >
   +0x028 hits             : 0n0
```

Range-for comparison:

```
0:000> g
Breakpoint 2 hit
Trap35_AutoDropsRef!range_for_copies_elements+0xfa:
00007ff7`f8571e4a c78424d000000001000000 mov dword ptr [rsp+0D0h],1 ss:00000053`af1dfcb0=00000000
0:000> $$ ===== variant 2 by value range variable is a stack copy =====
0:000> ? @rsp+0a8
Evaluate expression: 359420263560 = 00000053`af1dfc88
0:000> ? poi(@rsp+90)
Evaluate expression: 2360909321760 = 00000225`b1295620
0:000> ?? items._Mypair._Myval2._Myfirst
struct Heavy * 0x00000225`b1295620
   +0x000 payload          : std::basic_string<char,std::char_traits<char>,std::allocator<char> >
   +0x028 hits             : 0n0
0:000> bc 2
0:000> g
Breakpoint 3 hit
Trap35_AutoDropsRef!range_for_copies_elements+0x115:
00007ff7`f8571e65 488d15ccf10000  lea     rdx,[Trap35_AutoDropsRef!std::_Digit_pairs<wchar_t>+0x388 (00007ff7`f8581038)]
0:000> $$ ===== variant 2 after by value loop =====
0:000> ?? items._Mypair._Myval2._Myfirst[0].hits
int 0n0
0:000> bc 3
0:000> g
Breakpoint 4 hit
Trap35_AutoDropsRef!range_for_copies_elements+0x1d5:
00007ff7`f8571f25 488b842400010000 mov     rax,qword ptr [rsp+100h] ss:00000053`af1dfce0=00000225b1295620
0:000> $$ ===== variant 2 by reference range variable points at element =====
0:000> dv /t /v
00000053`af1dfce0 struct Heavy * item = 0x00000225`b1295620
00000053`af1dfcd8 struct Heavy * <end>$L1 = 0x00000225`b12956b0
00000053`af1dfcd0 struct Heavy * <begin>$L1 = 0x00000225`b1295620
00000053`af1dfcc8 class std::vector<Heavy,std::allocator<Heavy> > * <range>$L1 = 0x00000053`af1dfc08 { size=0x3 }
00000053`af1dfc08 class std::vector<Heavy,std::allocator<Heavy> > items = { size=0x3 }
0:000> ? poi(@rsp+100)
Evaluate expression: 2360909321760 = 00000225`b1295620
0:000> ?? items._Mypair._Myval2._Myfirst
struct Heavy * 0x00000225`b1295620
   +0x000 payload          : std::basic_string<char,std::char_traits<char>,std::allocator<char> >
   +0x028 hits             : 0n0
0:000> bc 4
0:000> g
Breakpoint 5 hit
Trap35_AutoDropsRef!range_for_copies_elements+0x1e6:
00007ff7`f8571f36 488d151bf10000  lea     rdx,[Trap35_AutoDropsRef!std::_Digit_pairs<wchar_t>+0x3a8 (00007ff7`f8581058)]
0:000> $$ ===== variant 2 after by reference loop =====
0:000> ?? items._Mypair._Myval2._Myfirst[0].hits
int 0n1
```

`decltype(auto)` comparison:

```
0:000> g
Breakpoint 6 hit
Trap35_AutoDropsRef!keeping_the_exact_type+0x1a:
00007ff7`f857206a 488b442428      mov     rax,qword ptr [rsp+28h] ss:00000053`af1dfd28={Trap35_AutoDropsRef!instance (00007ff7`f8585518)}
0:000> $$ ===== variant 3 decltype auto preserves reference =====
0:000> dv /t /v
00000053`af1dfd30 struct Heavy * observed = 0x00007ffc`5a2eb040
00000053`af1dfd20 struct Heavy * source = 0x00007ff7`f8585518
00000053`af1dfd28 struct Heavy * exact = 0x00007ff7`f8585518
0:000> ?? source
struct Heavy * 0x00007ff7`f8585518
   +0x000 payload          : std::basic_string<char,std::char_traits<char>,std::allocator<char> >
   +0x028 hits             : 0n42
0:000> ?? exact
struct Heavy * 0x00007ff7`f8585518
   +0x000 payload          : std::basic_string<char,std::char_traits<char>,std::allocator<char> >
   +0x028 hits             : 0n42
```

---

## 5. Why there is no unsafe target

`Trap35_AutoDropsRef_unsafe.exe` does not exist. Plain `auto` copying a reference result is not undefined behavior; it is a defined copy. A sanitizer has no invalid access to report. The correct evidence is address divergence and unchanged source state.

---

## 6. What the measurements prove

| Case | Deduced variable address | Source address | Result |
|---|---|---|---|
| `auto copy` | `00000053'af1dfcc8` | `00007ff7'f8585518` | different objects |
| `auto& reference` | `reference = 00007ff7'f8585518` | static instance `00007ff7'f8585518` | same object |
| range `auto item` | stack copy `00000053'af1dfc88` | element `00000225'b1295620` | write discarded; `hits` stays 0 |
| range `auto& item` | `item = 00000225'b1295620` | element `00000225'b1295620` | write lands; `hits` becomes 1 |
| `decltype(auto) exact` | `exact = 00007ff7'f8585518` | `source = 00007ff7'f8585518` | reference preserved |

The first and third rows diverge. That divergence is the lesson.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `?? &copy` | show the stack address of the copied object |
| `dv /t /v` | show references as the addresses they bind to |
| `bp module!function+offset` | stop inside one-line generated range-for code |
| `? @rsp+0a8` | compute the stack copy address for the by-value loop item |
| `? poi(@rsp+100)` | read the by-reference loop item's element address |
| `?? items._Mypair..._Myfirst` | compare with the vector's first element |
| `bc n` | clear one-shot breakpoints in a loop |

---

## 8. Left to you

1. Change `auto copy` to `const auto copy`. Does constness repair the copy?
2. Add a logging copy constructor to `Heavy` and compare the log with the CDB addresses.
3. Change the range loop to `const auto&`. Which address stays the same and which write is rejected by the compiler?
4. Use `auto&&` with `shared_instance()`. What does CDB show for the binding address?
5. Make `shared_instance()` return by value and remeasure `decltype(auto)`.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by address comparison | range-for internals require implementation-specific offsets |
| Sanitizers | no | behavior is defined and memory-safe |
| Compiler warnings | sometimes style-only | plain `auto` is legal |
| Unit tests | only with semantic assertions | copies can produce plausible output |
