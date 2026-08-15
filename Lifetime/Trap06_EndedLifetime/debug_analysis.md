# Trap06_EndedLifetime - CDB debug analysis

**Question:** if a raw pointer is numerically unchanged after scope exit, `delete`, or vector growth, is it still valid?

**Short answer:** no. The address is only a number. The owner state and the lifetime-ending event decide whether a live object exists.

---

## 1. Read the source first

```cpp
 7      { int local = 42; observer = &local; } // BP: local lifetime ends at brace.
 9      std::cout << "scope: " << *observer << '\n'; // unsafe only
19      delete observer; // BP: storage returned to the allocator; observer is now stale.
21      std::cout << "heap: " << *observer << '\n'; // unsafe only
33      values.reserve(values.capacity() + 1); // BP: buffer moves.
35      std::cout << "vector: " << *observer << '\n'; // unsafe only
```

| Variant | Owner before event | Lifetime-ending event |
|---|---|---|
| scope | automatic `local` | closing brace |
| heap | allocated `int` | `delete observer` |
| vector | `values` element buffer | `reserve` reallocates |

---

## 2. Build without a sanitizer

The existing Debug safe and unsafe executables were used. ASan is useful later, but the CDB lesson is the before/after measurement.

---

## 3. Start CDB

Run CDB with `-srcpath C:\src\cpptraps42\Lifetime\Trap06_EndedLifetime`, then type `.symopt-100` and `.lines -e` before source breakpoints.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:19`
0:000> bp `main.cpp:23`
0:000> bp `main.cpp:33`
0:000> bp `main.cpp:38`
0:000> g
Breakpoint 1 hit
Trap06_EndedLifetime!deleted_heap_dangle+0x47:
00007ff7`791c1ab7 488b442420      mov     rax,qword ptr [rsp+20h] ss:000000ca`93affba0=000001f8d5611060
0:000> dv /t /v
000000ca`93affba0 int * observer = 0x000001f8`d5611060
0:000> r $t0 = @@c++(observer)
0:000> dd @$t0 L1
000001f8`d5611060  0000002a
0:000> g
Breakpoint 2 hit
Trap06_EndedLifetime!deleted_heap_dangle+0x61:
00007ff7`791c1ad1 488d1508c10000  lea     rdx,[Trap06_EndedLifetime!__xt_z+0x140 (00007ff7`791cdbe0)]
0:000> dd @$t0 L4
000001f8`d5611060  feeefeee feeefeee feeefeee feeefeee
0:000> g
Breakpoint 3 hit
Trap06_EndedLifetime!reallocation_dangle+0xcd:
00007ff7`791c1bcd 488d4c2428      lea     rcx,[rsp+28h]
0:000> r $t1 = @@c++(observer)
0:000> r $t2 = @@c++(values._Mypair._Myval2._Myfirst)
0:000> .printf "observer = %p; vector first = %p\n", @$t1, @$t2
observer = 000001f8d5611060; vector first = 000001f8d5611060
0:000> g
Breakpoint 4 hit
Trap06_EndedLifetime!reallocation_dangle+0xe7:
00007ff7`791c1be7 488d1522c00000  lea     rdx,[Trap06_EndedLifetime!__xt_z+0x170 (00007ff7`791cdc10)]
0:000> r $t3 = @@c++(values._Mypair._Myval2._Myfirst)
0:000> .printf "observer = %p; old first = %p; current first = %p\n", @@c++(observer), @$t2, @$t3
observer = 000001f8d5611060; old first = 000001f8d5611060; current first = 000001f8d560a2f0
0:000> dd @$t1 L3
000001f8`d5611060  feeefeee feeefeee feeefeee
0:000> dd @$t3 L3
000001f8`d560a2f0  00000001 00000002 00000003
0:000> g
scope: observer intentionally not dereferenced
heap: pointer cleared instead of reused
vector: re-read by index instead: 1
```

---

## 5. The unsafe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:7`
0:000> bp `main.cpp:9`
0:000> bp `main.cpp:19`
0:000> bp `main.cpp:21`
0:000> bp `main.cpp:33`
0:000> bp `main.cpp:35`
0:000> g
Breakpoint 0 hit
Trap06_EndedLifetime_unsafe!scope_exit_dangle+0x20:
00007ff6`b0231a20 c74424342a000000 mov     dword ptr [rsp+34h],2Ah ss:00000023`246ff824=cccccccc
0:000> dv /t /v
00000023`246ff824 int local = 0n-858993460
00000023`246ff810 int * observer = 0x00000000`00000000
0:000> g
Breakpoint 1 hit
Trap06_EndedLifetime_unsafe!scope_exit_dangle+0x32:
00007ff6`b0231a32 488d1577c10000  lea     rdx,[Trap06_EndedLifetime_unsafe!__xt_z+0x110 (00007ff6`b023dbb0)]
0:000> dv /t /v
00000023`246ff810 int * observer = 0x00000023`246ff824
0:000> r $t0 = @@c++(observer)`n0:000> .printf "observer = %p\n", @$t0`nobserver = 00000023246ff824`n0:000> dd @$t0 L1`n00000023`246ff824  0000002a
0:000> g
Breakpoint 2 hit
Trap06_EndedLifetime_unsafe!deleted_heap_dangle+0x47:
00007ff6`b0231ad7 488b442420      mov     rax,qword ptr [rsp+20h] ss:00000023`246ff820=000001499309d060
0:000> r $t1 = @@c++(observer)
0:000> dd @$t1 L1
00000149`9309d060  0000002a
0:000> g
Breakpoint 3 hit
Trap06_EndedLifetime_unsafe!deleted_heap_dangle+0x61:
00007ff6`b0231af1 488d15c0c00000  lea     rdx,[Trap06_EndedLifetime_unsafe!__xt_z+0x118 (00007ff6`b023dbb8)]
0:000> dd @$t1 L4
00000149`9309d060  feeefeee feeefeee feeefeee feeefeee
0:000> g
Breakpoint 4 hit
Trap06_EndedLifetime_unsafe!reallocation_dangle+0xcd:
00007ff6`b0231c0d 488d4c2428      lea     rcx,[rsp+28h]
0:000> r $t2 = @@c++(observer)
0:000> r $t3 = @@c++(values._Mypair._Myval2._Myfirst)
0:000> .printf "observer = %p; first = %p; last = %p; end = %p\n", @$t2, @$t3, @$t4, @$t5
observer = 00000149930a3780; first = 00000149930a3780; last = 00000149930a378c; end = 00000149930a378c
0:000> dd @$t2 L3
00000149`930a3780  00000001 00000002 00000003
0:000> g
Breakpoint 5 hit
Trap06_EndedLifetime_unsafe!reallocation_dangle+0xe7:
00007ff6`b0231c27 488d1592bf0000  lea     rdx,[Trap06_EndedLifetime_unsafe!__xt_z+0x120 (00007ff6`b023dbc0)]
0:000> .printf "observer = %p; old first = %p; new first = %p; new last = %p; new end = %p\n", @@c++(observer), @$t3, @$t6, @$t7, @$t8
observer = 00000149930a3780; old first = 00000149930a3780; new first = 0000014993099f30; new last = 0000014993099f3c; new end = 0000014993099f40
0:000> dd @$t2 L3
00000149`930a3780  feeefeee feeefeee feeefeee
0:000> dd @$t6 L3
00000149`93099f30  00000001 00000002 00000003
0:000> g
scope: 42
heap: -17891602
vector: -17891602
```

---

## 6. What the measurements prove

| Variant | Before | After | Lesson |
|---|---|---|---|
| scope | stack slot receives `42` | observer names a dead stack slot | mapped address is not lifetime |
| heap | bytes are `0000002a` | same address reads `feeefeee` | debug fill exposes freed storage |
| vector | `observer == first` | `new first` differs; old bytes are freed fill | the vector owner moved |

`0xFEEEFEEE` is a Debug CRT/Win32 heap courtesy, not a C++ guarantee. Release builds need not write it.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `r $t0 = @@c++(observer)` | pin a raw address |
| `values._Mypair._Myval2._Myfirst` | MSVC vector begin pointer |
| `dd addr L3` | dump `int` slots |
| `dv /t /v` | show locals with types |

---

## 8. Left to you

1. Run the unsafe target under ASan and compare the evidence.
2. Set a hardware read breakpoint on the freed heap address.
3. Change the vector reserve amount and look for accidental non-reallocation.
4. Split the scope variant into more lines in a private copy and capture a cleaner before/after.
5. Convert the raw observer to an index for the vector variant and re-measure.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by measurement | stack lifetime is not always visible in bytes |
| Debug CRT fills | heap/vector evidence | Debug-only courtesy |
| ASan | many unsafe reads | changes allocator and does not prove protocols |
| Compiler warnings | limited | raw pointer lifetime is hard to infer |

