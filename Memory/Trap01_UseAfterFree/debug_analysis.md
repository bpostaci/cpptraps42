# Trap01_UseAfterFree - CDB debug analysis

**Question:** the pointer value did not change after the object was released.
Does that prove the object is still there?

**Short answer:** no. A pointer value and an object lifetime are two different
things. This session shows both, side by side, in the debugger.

---

## 1. Read the source first

`main.cpp` (12 lines). The `BP:` comments mark the lines that matter.

```cpp
 1  #include <iostream>
 2  #include <memory>
 3  struct Widget { int value{42}; };
 4  int main() {
 5      auto owner = std::make_unique<Widget>();
 6      Widget* observer = owner.get();
 7      std::cout << observer->value << '\n'; // Safe: owner still controls a live Widget.
 8      owner.reset(); // BP: lifetime ends and storage is returned to the allocator.
 9  #if defined(RUN_UNSAFE_EXAMPLE)
10      std::cout << observer->value << '\n'; // BP: heap-use-after-free; enable ASan.
11  #endif
12  }
```

Two variables hold the same address, but only one of them owns the object:

| Name | Holds | Owns the object |
|---|---|---|
| `owner` | `unique_ptr<Widget>` | yes |
| `observer` | raw `Widget*` | no |

Line 8 ends the lifetime. Line 10 exists only in the `_unsafe` target.

---

## 2. Build without a sanitizer

```powershell
cmake -S . -B build\cdb -DTRAPS_BUILD_UNSAFE=ON -DTRAPS_SANITIZER=none
cmake --build build\cdb --config Debug --target Trap01_UseAfterFree Trap01_UseAfterFree_unsafe
```

AddressSanitizer is deliberately off here. ASan replaces the allocator and puts
freed memory into quarantine, which hides the Debug CRT fill bytes that this
analysis uses as evidence. Build with the `msvc-asan-unsafe` preset when you
want a sanitizer report instead of a debugger session.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
       -srcpath C:\src\cpptraps42\Memory\Trap01_UseAfterFree ^
       C:\src\cpptraps42\build\cdb\Debug\Trap01_UseAfterFree.exe
```

- `-o` follows child processes.
- `-y` is the symbol path; the PDB sits next to the executable.
- `-srcpath` lets you use `` bp `main.cpp:8` `` instead of an address.

Then, at the `0:000>` prompt, two settings that are **off by default** and
without which the rest of this session does not work:

```
.symopt-100     resolve unqualified symbols, needed to type `observer` and `owner`
.lines -e       load line information, needed for source line breakpoints
```

---

## 4. The safe target

Command output is verbatim. Addresses change on every run because of ASLR;
compare the *relationships* between them, not the digits.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:8`             ← just before the lifetime ends
0:000> bp `main.cpp:12`            ← end of main; line 10 does not exist here
0:000> g
Breakpoint 0 hit
Trap01_UseAfterFree!main+0x4f:
00007ff7`f95316df 33d2            xor     edx,edx

0:000> $$ ===== STOP A: before owner.reset() =====
0:000> dv /t /v
00000045`b350fc88 class std::unique_ptr<Widget,std::default_delete<Widget> > owner = unique_ptr {...}
00000045`b350fc98 struct Widget * observer = 0x00000151`482ca6c0

0:000> r $t0 = @@c++(observer)      $$ pin the address into a pseudo-register
0:000> ?? *observer
struct Widget
   +0x000 value            : 0n42

0:000> db @$t0 L10
00000151`482ca6c0  2a 00 00 00 fd fd fd fd-ab ab ab ab ab ab ab ab  *...............

0:000> dqs @@c++(&owner) L1         $$ the raw pointer inside the unique_ptr
00000045`b350fc88  00000151`482ca6c0

0:000> g
Breakpoint 1 hit
Trap01_UseAfterFree!main+0x5c:

0:000> $$ ===== STOP B: after owner.reset() =====
0:000> dqs @@c++(&owner) L1
00000045`b350fc88  00000000`00000000

0:000> db @$t0 L10
00000151`482ca6c0  ee fe ee fe ee fe ee fe-ee fe ee fe ee fe ee fe  ................

0:000> g
42
```

Read the bytes at STOP A:

```
2a 00 00 00   fd fd fd fd   ab ab ab ab ab ab ab ab
^^^^^^^^^^^   ^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^
value = 42    guard band    heap fill after the block
(0x2a)        no-man's-land
```

**The important surprise:** at STOP B the safe target's memory is *also* freed
and *also* filled with `0xfeeefeee`. The safe target is not safe because the
memory is in a better state. It is safe because nobody reads it. The program
prints `42` once, from line 7, while the object was alive.

---

## 5. The unsafe target

Same session against `Trap01_UseAfterFree_unsafe.exe`, where line 10 compiles.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:8`             ← lifetime ends here
0:000> bp `main.cpp:10`            ← the stale read
0:000> g
Breakpoint 0 hit
Trap01_UseAfterFree_unsafe!main+0x4f:

0:000> $$ ===== STOP 1: before owner.reset() =====
0:000> dv /t /v
00000079`60b3f6d8 class std::unique_ptr<Widget,std::default_delete<Widget> > owner = unique_ptr {...}
00000079`60b3f6e8 struct Widget * observer = 0x000001b8`446e0340

0:000> r $t0 = @@c++(observer)
0:000> .printf "observer = %p\n", @$t0
observer = 000001b8446e0340

0:000> ?? *observer
struct Widget
   +0x000 value            : 0n42

0:000> db @$t0 L10
000001b8`446e0340  2a 00 00 00 fd fd fd fd-ab ab ab ab ab ab ab ab  *...............

0:000> dqs @@c++(&owner) L1
00000079`60b3f6d8  000001b8`446e0340

0:000> g
Breakpoint 1 hit
Trap01_UseAfterFree_unsafe!main+0x5b:

0:000> $$ ===== STOP 2: after owner.reset() =====
0:000> .printf "observer = %p   <-- numerically unchanged\n", @@c++(observer)
observer = 000001b8446e0340   <-- numerically unchanged

0:000> dqs @@c++(&owner) L1
00000079`60b3f6d8  00000000`00000000

0:000> db @$t0 L10
000001b8`446e0340  ee fe ee fe ee fe ee fe-ee fe ee fe ee fe ee fe  ................

0:000> ?? observer->value
int 0n-17891602

0:000> g
42
-17891602
```

---

## 6. What the four measurements prove

| Measurement | Before `reset()` | After `reset()` |
|---|---|---|
| `observer` | `1b8446e0340` | `1b8446e0340` — **unchanged** |
| raw pointer inside `owner` | `1b8446e0340` | `00000000` — **released** |
| bytes at the object | `2a 00 00 00 fd fd …` | `ee fe ee fe ee fe …` |
| `observer->value` | `42` | `-17891602` |

The two pointers diverge. That divergence *is* the lesson.

`-17891602` is `0xFEEEFEEE` read as a signed `int`. The program does not crash;
it prints the allocator's free-fill pattern as if it were data. Looking correct
is the most dangerous form of undefined behavior, because tests pass.

The fill bytes are a Debug CRT courtesy, not a language guarantee:

| Pattern | Meaning |
|---|---|
| `0xCD` | freshly allocated, not yet initialized |
| `0xFD` | no-man's-land guard band around the block |
| `0xDD` | freed block, already returned to the CRT |
| `0xFEEEFEEE` | freed back to the Win32 heap |

A Release build writes none of these. The same read then returns whatever the
allocator left behind, so the bug becomes intermittent instead of visible.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `.symopt-100` | resolve unqualified symbols so `observer` can be typed by name |
| `.lines -e` | load line information for source line breakpoints |
| `` bp `main.cpp:8` `` | breakpoint on a source line |
| `bl` | list breakpoints and confirm they resolved |
| `g` | run to the next breakpoint |
| `p` | step one instruction; a source line usually needs several |
| `dv /t /v` | local variables with type and address |
| `r $t0 = @@c++(expr)` | store a C++ expression in a pseudo-register |
| `?? expr` | evaluate a C++ expression |
| `db addr L10` | dump 0x10 bytes |
| `dqs addr L1` | dump one pointer-sized value with symbol resolution |
| `.printf "%p\n", @$t0` | formatted output inside a script |

`$t0` matters here: after `reset()`, the expression `observer` still works, but
pinning the address up front keeps the two stops comparable even when the
variable is optimized away or goes out of scope.

---

## 8. Left to you

1. Rebuild the unsafe target as Release. Does `0xFEEEFEEE` still appear? What
   does the program print now, and what changed about the *evidence*?
2. Build with the `msvc-asan-unsafe` preset and run it. ASan names the bug, but
   which of the four measurements above can you no longer make, and why?
3. `ba r4 <address>` sets a hardware read breakpoint. Put one on the object and
   find every access, including the one inside `reset()`.
4. Change line 6 to `std::shared_ptr<Widget> observer = owner;`. What do the two
   raw pointers look like at STOP 2 now, and which measurement replaces the
   byte dump as your evidence?
5. `!heap -p -a <address>` can identify the owning heap block. It needs public
   `ntdll` symbols and page heap via gflags. Set that up and compare what it
   reports before and after the release.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB + Debug CRT fills | yes, as a byte pattern | Debug builds only; a pattern is a hint, not a diagnosis |
| AddressSanitizer | yes, precisely | hides the fill bytes; slows the program; changes the allocator |
| Release build | no | the read silently returns plausible data |
| Compiler warnings | no | the code is well formed; the defect is in the lifetime |
