# Trap39_MemberInitOrder - CDB debug analysis

**Question:** does the textual order in the member-init-list control member initialization?

**Short answer:** no. Bases are initialized first, then members in declaration order. The list order does not change that. This ordering is defined; it becomes dangerous when an initializer reads a member that has not been initialized yet. Trap39 has an `_unsafe` sibling for that dependency.

---

## 1. Read the source first

```cpp
 6  struct Reordered {
 7      int doubled;    // Declared first, so it is initialised first.
 8      int count;      // Declared second, even though the init-list writes count first.
 9      explicit Reordered(int value)
10          : count(value)
11          , doubled(count * 2) // reads count before count(value) runs.
18      Reordered r{21};
19      std::cout << "reordered: count=" << r.count << " doubled=" << r.doubled << '\n';
27  struct Ordered { int count; int doubled; explicit Ordered(int value) : count(value), doubled(count * 2) {} };
33  struct BodyComputed { int count; int doubled; explicit BodyComputed(int value) : count(value), doubled(0) { doubled = count * 2; } };
49  struct Buffer { std::vector<int> storage; explicit Buffer(std::size_t n) : storage(n, 0) { ... } };
54  struct View : Buffer { std::size_t size; explicit View(std::size_t n) : Buffer(n), size(storage.size()) { ... } };
```

| Entity | Role |
|---|---|
| `Reordered` | Unsafe: `doubled` is declared before `count` but depends on it. |
| `Ordered` | Safe: declarations match dependency order. |
| `BodyComputed` | Safe: derives the dependent value in the body. |
| `View : Buffer` | Safe: base construction precedes member initialization. |

---

## 2. Build and compiler evidence

The existing `Trap39_MemberInitOrder.exe` and `Trap39_MemberInitOrder_unsafe.exe` were used. A standalone `/W4` scratch compile of the unsafe source produced no MSVC warning in this environment; the object layout and constructor order below are the evidence.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\ObjectModel\Trap39_MemberInitOrder -cf %TEMP%\h2_trap39_unsafe2.cdb C:\src\cpptraps42\build\cdb\Debug\Trap39_MemberInitOrder_unsafe.exe
```

The script starts with `.symopt-100` and `.lines -e`, then uses `dt` for layout, `uf` for constructor instruction order, and instruction breakpoints for the two member initializers.

---

## 4. Transcript: unsafe target follows declaration order

```text
0:000> .symopt-100
0:000> .lines -e
0:000> x Trap39_MemberInitOrder_unsafe!*Reordered*
*** WARNING: Unable to verify checksum for Trap39_MemberInitOrder_unsafe.exe
00007ff6`21fa3480 Trap39_MemberInitOrder_unsafe!Reordered::Reordered (int)
0:000> dt Trap39_MemberInitOrder_unsafe!Reordered
   +0x000 doubled          : Int4B
   +0x004 count            : Int4B
0:000> uf Trap39_MemberInitOrder_unsafe!Reordered::Reordered
Trap39_MemberInitOrder_unsafe!Reordered::Reordered [C:\src\cpptraps42\ObjectModel\Trap39_MemberInitOrder\main.cpp @ 12]:
   12 00007ff6`21fa3480 89542410        mov     dword ptr [rsp+10h],edx
   12 00007ff6`21fa3484 48894c2408      mov     qword ptr [rsp+8],rcx
   12 00007ff6`21fa3489 57              push    rdi
   11 00007ff6`21fa348a 488b442410      mov     rax,qword ptr [rsp+10h]
   11 00007ff6`21fa348f 8b4004          mov     eax,dword ptr [rax+4]
   11 00007ff6`21fa3492 d1e0            shl     eax,1
   11 00007ff6`21fa3494 488b4c2410      mov     rcx,qword ptr [rsp+10h]
   11 00007ff6`21fa3499 8901            mov     dword ptr [rcx],eax
   10 00007ff6`21fa349b 488b442410      mov     rax,qword ptr [rsp+10h]
   10 00007ff6`21fa34a0 8b4c2418        mov     ecx,dword ptr [rsp+18h]
   10 00007ff6`21fa34a4 894804          mov     dword ptr [rax+4],ecx
   12 00007ff6`21fa34a7 488b442410      mov     rax,qword ptr [rsp+10h]
   12 00007ff6`21fa34ac 5f              pop     rdi
   12 00007ff6`21fa34ad c3              ret
0:000> bp Trap39_MemberInitOrder_unsafe!Reordered::Reordered+0xa
0:000> bp Trap39_MemberInitOrder_unsafe!Reordered::Reordered+0x1b
0:000> bp Trap39_MemberInitOrder_unsafe!Reordered::Reordered+0x24
0:000> bp `main.cpp:19`
0:000> g
Breakpoint 0 hit
Trap39_MemberInitOrder_unsafe!Reordered::Reordered+0xa:
00007ff6`21fa348a 488b442410      mov     rax,qword ptr [rsp+10h] ss:00000052`9f2ff770=000000529f2ff798
0:000> $$ ===== STOP 1 about to read count for doubled =====
0:000> dv /t /v
00000052`9f2ff770 struct Reordered * this = 0x00000052`9f2ff798
00000052`9f2ff778 int value = 0n21
0:000> r $t0 = @@c++(this)
0:000> dt Trap39_MemberInitOrder_unsafe!Reordered @$t0
   +0x000 doubled          : 0n-858993460
   +0x004 count            : 0n-858993460
0:000> g
Breakpoint 1 hit
Trap39_MemberInitOrder_unsafe!Reordered::Reordered+0x1b:
00007ff6`21fa349b 488b442410      mov     rax,qword ptr [rsp+10h] ss:00000052`9f2ff770=000000529f2ff798
0:000> $$ ===== STOP 2 doubled has been written first =====
0:000> dt Trap39_MemberInitOrder_unsafe!Reordered @$t0
   +0x000 doubled          : 0n-1717986920
   +0x004 count            : 0n-858993460
0:000> g
Breakpoint 2 hit
Trap39_MemberInitOrder_unsafe!Reordered::Reordered+0x24:
00007ff6`21fa34a4 894804          mov     dword ptr [rax+4],ecx ds:00000052`9f2ff79c=cccccccc
0:000> $$ ===== STOP 3 count is written second =====
0:000> dt Trap39_MemberInitOrder_unsafe!Reordered @$t0
   +0x000 doubled          : 0n-1717986920
   +0x004 count            : 0n-858993460
0:000> g
Breakpoint 3 hit
Trap39_MemberInitOrder_unsafe!init_list_order_is_a_lie+0x26:
00007ff6`21fa1986 488d1533c20000  lea     rdx,[Trap39_MemberInitOrder_unsafe!__xt_z+0x120 (00007ff6`21fadbc0)]
0:000> $$ ===== STOP 4 after construction at print line =====
0:000> dv /t /v
00000052`9f2ff798 struct Reordered r = struct Reordered
0:000> ?? r.count
int 0n21
0:000> ?? r.doubled
int 0n-1717986920
0:000> dt Trap39_MemberInitOrder_unsafe!Reordered @@c++(&r)
   +0x000 doubled          : 0n-1717986920
   +0x004 count            : 0n21
0:000> g
reordered: count=21 doubled=-1717986920
ordered: 21/42  body: 21/42
Buffer ctor n=4
View ctor size=4
view storage=4
```

---

## 5. The unsafe target

Trap39 has an unsafe sibling. It is unsafe because `doubled(count * 2)` reads `count` before `count(value)` has executed. Declaration-order initialization itself is not a bug; the dependency on a not-yet-initialized member is the bug.

---

## 6. What the measurements prove

| Evidence | Meaning |
|---|---|
| `dt Reordered` | `doubled` lives at offset `+0x000`; `count` at `+0x004`. |
| `uf Reordered::Reordered` | Instructions for line 11 execute before line 10. |
| STOP 2 | `doubled` has been computed while `count` is still `0xCCCCCCCC`. |
| STOP 4 | `count` becomes `21`, but `doubled` remains the earlier bad result. |

`-858993460` is Debug CRT stack fill (`0xCCCCCCCC`) interpreted as an `int`; it is not guaranteed by the language.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `dt Trap39_MemberInitOrder_unsafe!Reordered` | Show member layout. |
| `uf ...!Reordered::Reordered` | Show constructor execution order. |
| `bp ...+0xa`, `+0x1b`, `+0x24` | Stop around the two member initializers. |
| `r $t0 = @@c++(this)` | Keep the object address stable across stops. |
| `dt Type @$t0` | Re-read the same object's members. |

---

## 8. Left to you

1. In a scratch copy, declare `count` before `doubled`. How do `dt` and `uf` change?
2. Move `doubled = count * 2` into the body. Which unsafe breakpoint disappears?
3. Add a third member between `doubled` and `count`. Where does it appear in `dt`?
4. Compile a scratch Release object. Which Debug fill values disappear?

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB `dt`/`uf` | Yes | Requires symbols and careful instruction breakpoints. |
| MSVC `/W4` | Not in this run | The scratch compile emitted no warning here. |
| AddressSanitizer | No direct report | This is not a heap or bounds violation. |
| Debug fill bytes | Symptom only | `0xCCCCCCCC` is a debugger artifact, not the rule. |
