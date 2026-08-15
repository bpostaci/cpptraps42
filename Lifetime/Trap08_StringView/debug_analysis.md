# Trap08_StringView - CDB debug analysis

**Question:** if a `string_view` still has a pointer and size, does that prove the characters are still owned?

**Short answer:** no. `string_view` owns no storage. Compare `view.data()` with the current owner's `data()` and ask whether that owner still lives.

---

## 1. Read the source first

```cpp
10      return s; // BP: view outlives the string it points into.
27      std::string_view view = make_owner(); // BP: temporary destroyed after this statement.
40      owner = "a much longer replacement string that forces reallocation"; // BP
42      std::cout << "mutation: " << view << '\n'; // unsafe only
44      view = owner; // safe branch re-seats the view.
```

| Variant | Owner event | Measurement |
|---|---|---|
| returned view | local `s` dies | old `s.data()` vs `view.data()` |
| temporary | full-expression ends | `view.data()` with no owner |
| mutation | owner reallocates | `view.data()` vs current owner data |

---

## 2. Build without a sanitizer

The existing Debug safe and unsafe executables were used. CDB shows `std::string_view` fields directly; ASan is a later check, not the source of these measurements.

---

## 3. Start CDB

Use `-srcpath C:\src\cpptraps42\Lifetime\Trap08_StringView`, `.symopt-100`, and `.lines -e`. The fields `_Mydata`, `_Mysize`, `_Bx._Buf`, and `_Bx._Ptr` are MSVC implementation details used only for debugging.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:20`
0:000> bp `main.cpp:32`
0:000> bp `main.cpp:40`
0:000> bp `main.cpp:45`
0:000> g
Breakpoint 0 hit
Trap08_StringView!dangling_return+0x4f:
00007ff6`5e7c1b0f 488d15cad00000  lea     rdx,[Trap08_StringView!__xt_z+0x140 (00007ff6`5e7cebe0)]
0:000> .printf "view.data() = %p; owner.data() = %p\n", @@c++(view._Mydata), @@c++(owner._Mypair._Myval2._Bx._Buf)
view.data() = 000000071c4ffcb0; owner.data() = 000000071c4ffcb0
0:000> g
Breakpoint 1 hit
Trap08_StringView!temporary_binding+0x48:
00007ff6`5e7c1c08 488d15e1cf0000  lea     rdx,[Trap08_StringView!__xt_z+0x150 (00007ff6`5e7cebf0)]
0:000> .printf "view.data() = %p; owner.data() = %p\n", @@c++(view._Mydata), @@c++(owner._Mypair._Myval2._Bx._Buf)
view.data() = 000000071c4ffcb0; owner.data() = 000000071c4ffcb0
0:000> g
Breakpoint 2 hit
Trap08_StringView!mutated_owner+0x4e:
00007ff6`5e7c1d0e 488d15f3ce0000  lea     rdx,[Trap08_StringView!__xt_z+0x168 (00007ff6`5e7cec08)]
0:000> .printf "view.data() = %p; owner.data() = %p\n", @$t0, @$t1
view.data() = 000000071c4ffca0; owner.data() = 000000071c4ffca0
0:000> g
Breakpoint 3 hit
Trap08_StringView!mutated_owner+0x8c:
00007ff6`5e7c1d4c 488d15f5ce0000  lea     rdx,[Trap08_StringView!__xt_z+0x1a8 (00007ff6`5e7cec48)]
0:000> .printf "view.data() = %p; current owner.data() = %p\n", @@c++(view._Mydata), @$t2
view.data() = 00000139c38198f0; current owner.data() = 00000139c38198f0
0:000> g
return: stable owner
temporary: temporary owner
mutation: a much longer replacement string that forces reallocation
```

---

## 5. The unsafe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:10`
0:000> bp `main.cpp:16`
0:000> bp `main.cpp:28`
0:000> bp `main.cpp:40`
0:000> bp `main.cpp:42`
0:000> g
Breakpoint 0 hit
Trap08_StringView_unsafe!returned_view+0x43:
00007ff7`229e1a63 488b942480000000 mov     rdx,qword ptr [rsp+80h] ss:00000031`386ff620=00000031386ff648
0:000> r $t0 = @@c++(s._Mypair._Myval2._Bx._Buf)
0:000> .printf "s.data() = %p\n", @$t0
s.data() = 00000031386ff5d0
0:000> da @$t0 Lf
00000031`386ff5d0  "temporary owner"
0:000> g
Breakpoint 1 hit
Trap08_StringView_unsafe!dangling_return+0x26:
00007ff7`229e1ae6 488d15e3d00000  lea     rdx,[Trap08_StringView_unsafe!__xt_z+0x130 (00007ff7`229eebd0)]
0:000> .printf "view.data() = %p; old s.data() = %p\n", @@c++(view._Mydata), @$t0
view.data() = 00000031386ff5d0; old s.data() = 00000031386ff5d0
0:000> ?? view._Mysize
unsigned int64 0xf
0:000> da @@c++(view._Mydata) Lf
00000031`386ff5d0  ""
0:000> g
Breakpoint 2 hit
Trap08_StringView_unsafe!temporary_binding+0x6c:
00007ff7`229e1bdc 488d15fdcf0000  lea     rdx,[Trap08_StringView_unsafe!__xt_z+0x140 (00007ff7`229eebe0)]
0:000> r $t1 = @@c++(view._Mydata)
0:000> .printf "view.data() = %p\n", @$t1
view.data() = 00000031386ff630
0:000> ?? view._Mysize
unsigned int64 0xf
0:000> g
Breakpoint 4 hit
Trap08_StringView_unsafe!mutated_owner+0x60:
00007ff7`229e1ce0 488d1551cf0000  lea     rdx,[Trap08_StringView_unsafe!__xt_z+0x198 (00007ff7`229eec38)]
0:000> .printf "view.data() = %p; old owner.data() = %p; current owner.data() = %p\n", @@c++(view._Mydata), @$t3, @$t4
view.data() = 00000031386ff610; old owner.data() = 00000031386ff610; current owner.data() = 00000175a137cf00
0:000> ?? view._Mysize
unsigned int64 5
0:000> ?? owner._Mypair._Myval2._Mysize
unsigned int64 0x39
0:000> da @$t4 L20
00000175`a137cf00  "a much longer replacement string"
```

---

## 6. What the measurements prove

| Variant | Before | After | Lesson |
|---|---|---|---|
| returned view | `s.data() = ...386ff5d0` | `view.data()` still equals it, but `s` is gone | same pointer, no owner |
| temporary | `view._Mysize = 0xf` | no live temporary owner remains | size is not ownership |
| mutation | view and owner data match before assignment | current owner data becomes `...a137cf00` while view stays old | divergence is the evidence |

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `?? view._Mysize` | show stored view length |
| `@@c++(view._Mydata)` | read `string_view` data pointer |
| `owner._Mypair._Myval2._Bx._Buf` | SSO buffer |
| `owner._Mypair._Myval2._Bx._Ptr` | heap buffer after reallocation |
| `da addr Lf` | display characters |

---

## 8. Left to you

1. Use a longer first string so the first owner is heap-backed; compare evidence.
2. Re-run unsafe under ASan and note which variants it catches.
3. Step into `operator<<` for `string_view` and identify the stale read.
4. Replace `string_view` with `string`; which measurements disappear?
5. Mutate without reallocation and ask whether lifetime is still valid.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by `data()` comparison | uses MSVC private layout |
| ASan | often for stale reads | does not prove all view protocols |
| Debug CRT fills | partial | SSO stack storage may not show heap fills |
| Compiler warnings | inconsistent | many dangling views compile cleanly |
