# Trap09_LambdaCapture - CDB debug analysis

**Question:** when a callback object still exists, does that prove the objects it captured are still alive?

**Short answer:** no. A closure stores exactly what the capture list says: a value, a raw reference/pointer, or an owning object.

---

## 1. Read the source first

```cpp
 6  auto bad_by_reference() { int local=42; return [&local]{ return local; }; }
 7  auto good_by_value() { int local=42; return [local]{ return local; }; }
24          return [this]{ return id; }; // unsafe raw this
26          return [copy = id]{ return copy; }; // safe value
44          reader = [&ref]{ return ref.id; }; // unsafe raw reference
46          reader = [owner]{ return owner->id; }; // safe shared owner
```

| Variant | Unsafe stores | Safe stores |
|---|---|---|
| reference | address of dead stack local | copied `int` |
| `this` | raw `Session*` | copied `id` |
| owned | raw `Session*` through `ref` | `shared_ptr<Session>` |

---

## 2. Build without a sanitizer

The existing Debug safe and unsafe executables were used. CDB can dump the closure storage; ASan is only a supplemental runtime check.

---

## 3. Start CDB

Use `-srcpath C:\src\cpptraps42\Lifetime\Trap09_LambdaCapture`, then `.symopt-100` and `.lines -e`. `dqs` and `db` show whether the closure stores an integer, a raw pointer, or a `shared_ptr` pair.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:15`
0:000> bp `main.cpp:33`
0:000> bp `main.cpp:34`
0:000> bp `main.cpp:49`
0:000> g
Breakpoint 0 hit
Trap09_LambdaCapture!escaping_reference_capture+0x21:
00007ff7`d05a19d1 488d15f0c30000  lea     rdx,[Trap09_LambdaCapture!__xt_z+0x328 (00007ff7`d05addc8)]
0:000> ?? callback
class good_by_value::__l2::<lambda_1>
   +0x000 local            : 0n42
0:000> dd @@c++(&callback) L1
00000052`32bef794  0000002a
0:000> g
Breakpoint 2 hit
Trap09_LambdaCapture!this_capture+0x82:
00007ff7`d05a1ab2 488d151bc30000  lea     rdx,[Trap09_LambdaCapture!__xt_z+0x334 (00007ff7`d05addd4)]
0:000> dqs @@c++(&reader) L8
00000052`32bef6e0  00007ff7`d05ae380 Trap09_LambdaCapture!std::_Func_impl_no_alloc<`Session::make_reader'::`2'::<lambda_1>,int>::`vftable'
00000052`32bef6e8  cccccccc`00000007
00000052`32bef6f0  cccccccc`cccccccc
00000052`32bef6f8  cccccccc`cccccccc
00000052`32bef700  cccccccc`cccccccc
00000052`32bef708  cccccccc`cccccccc
00000052`32bef710  cccccccc`cccccccc
00000052`32bef718  00000052`32bef6e0
0:000> db @@c++(&reader) L40
00000052`32bef6e0  80 e3 5a d0 f7 7f 00 00-07 00 00 00 cc cc cc cc  ..Z.............
00000052`32bef6f0  cc cc cc cc cc cc cc cc-cc cc cc cc cc cc cc cc  ................
00000052`32bef700  cc cc cc cc cc cc cc cc-cc cc cc cc cc cc cc cc  ................
00000052`32bef710  cc cc cc cc cc cc cc cc-e0 f6 be 32 52 00 00 00  ...........2R...
0:000> g
Breakpoint 0 hit
Trap09_LambdaCapture!owned_member_capture+0xa1:
00007ff7`d05a1be1 488d15f8c10000  lea     rdx,[Trap09_LambdaCapture!__xt_z+0x340 (00007ff7`d05adde0)]
0:000> dqs @@c++(&reader) L8
000000e1`fd4ff910  00007ff7`d05ae3c8 Trap09_LambdaCapture!std::_Func_impl_no_alloc<`owned_member_capture'::`3'::<lambda_1>,int>::`vftable'
000000e1`fd4ff918  0000026e`0931a780
000000e1`fd4ff920  0000026e`0931a770
000000e1`fd4ff928  cccccccc`cccccccc
000000e1`fd4ff930  cccccccc`cccccccc
000000e1`fd4ff938  cccccccc`cccccccc
000000e1`fd4ff940  cccccccc`cccccccc
000000e1`fd4ff948  000000e1`fd4ff910
0:000> dd @$t0 L1
0000026e`0931a780  00000007
0:000> g
reference: 42
this: 7
owned: 7
```

---

## 5. The unsafe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:12`
0:000> bp `main.cpp:33`
0:000> bp `main.cpp:34`
0:000> bp `main.cpp:43`
0:000> bp `main.cpp:49`
0:000> g
Breakpoint 0 hit
Trap09_LambdaCapture_unsafe!escaping_reference_capture+0x21:
00007ff7`b12d1991 488d1530b40000  lea     rdx,[Trap09_LambdaCapture_unsafe!__xt_z+0x328 (00007ff7`b12dcdc8)]
0:000> ?? callback
class bad_by_reference::__l2::<lambda_1>
   +0x000 local            : 0x0000009f`d690fd04  -> 0n42
0:000> dqs @@c++(&callback) L2
0000009f`d690fd58  0000009f`d690fd04
0000009f`d690fd60  cccccccc`cccccccc
0:000> g
Breakpoint 2 hit
Trap09_LambdaCapture_unsafe!this_capture+0x82:
00007ff7`b12d1a72 488d155bb30000  lea     rdx,[Trap09_LambdaCapture_unsafe!__xt_z+0x334 (00007ff7`b12dcdd4)]
0:000> dqs @@c++(&reader) L8
0000009f`d690fca0  00007ff7`b12dd380 Trap09_LambdaCapture_unsafe!std::_Func_impl_no_alloc<`Session::make_reader'::`2'::<lambda_1>,int>::`vftable'
0000009f`d690fca8  0000009f`d690fcf4
0000009f`d690fcb0  cccccccc`cccccccc
0000009f`d690fcb8  cccccccc`cccccccc
0000009f`d690fcc0  cccccccc`cccccccc
0000009f`d690fcc8  cccccccc`cccccccc
0000009f`d690fcd0  cccccccc`cccccccc
0000009f`d690fcd8  0000009f`d690fca0
0:000> g
Breakpoint 1 hit
Trap09_LambdaCapture_unsafe!owned_member_capture+0x8b:
00007ff7`b12d1b8b 488d154eb20000  lea     rdx,[Trap09_LambdaCapture_unsafe!__xt_z+0x340 (00007ff7`b12dcde0)]
0:000> dd @$t1 L4
000001d0`46b0ce30  feeefeee feeefeee feeefeee feeefeee
0:000> dqs @@c++(&reader) L8
000000a2`aa2ff990  00007ff7`b12dd3c8 Trap09_LambdaCapture_unsafe!std::_Func_impl_no_alloc<`owned_member_capture'::`3'::<lambda_1>,int>::`vftable'
000000a2`aa2ff998  000001d0`46b0ce30
000000a2`aa2ff9a0  cccccccc`cccccccc
000000a2`aa2ff9a8  cccccccc`cccccccc
000000a2`aa2ff9b0  cccccccc`cccccccc
000000a2`aa2ff9b8  cccccccc`cccccccc
000000a2`aa2ff9c0  cccccccc`cccccccc
000000a2`aa2ff9c8  000000a2`aa2ff990
0:000> g
reference: -858993460
this: 7
owned: -17891602
```

---

## 6. What the measurements prove

| Variant | Unsafe closure | Safe closure | Lesson |
|---|---|---|---|
| reference | word is a stack address | word is `0000002a` | capture value if lifetime must survive |
| `this` | `reader+8` is raw old `Session*` | bytes contain `07 00 00 00` | `[this]` does not copy members |
| owned | raw pointer to `feeefeee` storage | object pointer plus control block | capture the owner to extend lifetime |

The unsafe `this` variant printed `7` in this run, but that is still undefined behavior: stale storage happened to contain the old value.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `?? callback` | show lambda members |
| `dqs @@c++(&reader) L8` | dump `std::function` storage |
| `db @@c++(&reader) L40` | inspect captured bytes |
| `dd addr L4` | inspect captured pointee |

---

## 8. Left to you

1. Break on the lambda `operator()` symbols from `x *lambda*` and inspect closure `this`.
2. Change `[this]` to `[*this]` in a private copy and compare bytes.
3. Force `std::function` heap allocation with a larger capture.
4. Re-run the unsafe target several times and compare stale results.
5. Build with ASan and list which captures it catches.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by closure storage | `std::function` layout is implementation-specific |
| ASan | some stale reads | cannot prove capture policy correct |
| Debug CRT fills | useful for freed heap object | stack captures may just show old bytes |
| Compiler warnings | limited | captures can be syntactically valid |
