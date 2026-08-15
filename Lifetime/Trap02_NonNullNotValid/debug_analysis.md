# Trap02_NonNullNotValid - CDB debug analysis

**Question:** a debugger can still show a non-null stored address after the owner was reset. Does that prove a live object exists?

**Short answer:** no. A non-null numeric address is not a lifetime proof. The valid test is whether `weak_ptr::lock()` can create a new owning `shared_ptr`.

---

## 1. Read the source first

```cpp
 1  #include <iostream>
 2  #include <memory>
 3  int main() {
 4      auto owner = std::make_shared<int>(42);
 5      std::weak_ptr<int> observer = owner;
 6      owner.reset(); // BP: the observer remains, but the int lifetime ends here.
 7      if (auto snapshot = observer.lock()) std::cout << *snapshot << '\n';
 8      else std::cout << "No live object - numeric non-null checks would be insufficient\n";
 9  }
```

| Name | Holds | Liveness evidence |
|---|---|---|
| `owner` | `shared_ptr<int>` | strong owner while non-empty |
| `observer` | `weak_ptr<int>` | observer only; may retain non-null implementation pointers |
| `snapshot` | `shared_ptr<int>` from `lock()` | actual proof if non-empty |

---

## 2. Build without a sanitizer

The existing Debug executable was used; no rebuild was needed. This is defined code, so AddressSanitizer is not expected to report anything.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\Lifetime\Trap02_NonNullNotValid C:\src\cpptraps42\build\cdb\Debug\Trap02_NonNullNotValid.exe
```

Use `.symopt-100` for local names and `.lines -e` for source-line breakpoints.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:6`
0:000> bp `main.cpp:7`
0:000> bp `main.cpp:8`
0:000> g
Breakpoint 0 hit
Trap02_NonNullNotValid!main+0x48:
00007ff7`399f1818 488d4c2428      lea     rcx,[rsp+28h]
0:000> dv /t /v
0000004d`f978f898 class std::shared_ptr<int> owner = 42
0000004d`f978f8c8 class std::weak_ptr<int> observer = 42
0:000> r $t0 = poi(@@c++(&owner))
0:000> r $t1 = poi(@@c++(&owner)+8)
0:000> .printf "owner stored pointer = %p; control block = %p\n", @$t0, @$t1
owner stored pointer = 0000022fef5f9920; control block = 0000022fef5f9910
0:000> dqs @@c++(&owner) L2
0000004d`f978f898  0000022f`ef5f9920
0000004d`f978f8a0  0000022f`ef5f9910
0:000> dqs @@c++(&observer) L2
0000004d`f978f8c8  0000022f`ef5f9920
0000004d`f978f8d0  0000022f`ef5f9910
0:000> dd @$t0 L1
0000022f`ef5f9920  0000002a
0:000> g
Breakpoint 1 hit
Trap02_NonNullNotValid!main+0x53:
00007ff7`399f1823 488d942488000000 lea     rdx,[rsp+88h]
0:000> dv /t /v
0000004d`f978f8f8 class std::shared_ptr<int> snapshot = class std::shared_ptr<int>
0000004d`f978f898 class std::shared_ptr<int> owner = empty
0000004d`f978f8c8 class std::weak_ptr<int> observer = 42
0:000> dqs @@c++(&owner) L2
0000004d`f978f898  00000000`00000000
0000004d`f978f8a0  00000000`00000000
0:000> dqs @@c++(&observer) L2
0000004d`f978f8c8  0000022f`ef5f9920
0000004d`f978f8d0  0000022f`ef5f9910
0:000> g
Breakpoint 2 hit
Trap02_NonNullNotValid!main+0xb1:
00007ff7`399f1881 488d1528a30000  lea     rdx,[Trap02_NonNullNotValid!__xt_z+0x110 (00007ff7`399fbbb0)]
0:000> dv /t /v
0000004d`f978f8f8 class std::shared_ptr<int> snapshot = empty
0000004d`f978f898 class std::shared_ptr<int> owner = empty
0000004d`f978f8c8 class std::weak_ptr<int> observer = 42
0:000> dqs @@c++(&snapshot) L2
0000004d`f978f8f8  00000000`00000000
0000004d`f978f900  00000000`00000000
0:000> g
No live object - numeric non-null checks would be insufficient
```

---

## 5. Why there is no unsafe target

Listing `build\cdb\Debug` for `Trap02*` produced:

```text
Trap02_NonNullNotValid.exe
Trap02_NonNullNotValid.pdb
```

There is no `_unsafe` sibling because the source has no `RUN_UNSAFE_EXAMPLE` branch. The trap demonstrates the safe protocol directly.

---

## 6. What the measurements prove

| Measurement | Before `reset()` | After `reset()` / `lock()` |
|---|---|---|
| `owner` | object pointer `0000022fef5f9920` | empty |
| `observer` | same object pointer and control block | still non-null internally |
| old object bytes | `0000002a` | not a lifetime proof |
| `snapshot` | not yet formed | empty |

The divergence between non-null observer storage and an empty `snapshot` is the lesson.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `dqs @@c++(&observer) L2` | show the observer's stored words |
| `r $t0 = poi(@@c++(&owner))` | pin the object address before reset |
| `dd @$t0 L1` | inspect old bytes |
| `dv /t /v` | show locals and their types |

---

## 8. Left to you

1. Step into `observer.lock()` and find where the empty `snapshot` is produced.
2. Add a raw pointer in a private copy. Which measurement becomes misleading?
3. Replace `weak_ptr` with a second `shared_ptr`; how does the table change?
4. Run the script twice and compare relationships rather than addresses.
5. Build with ASan and explain why no report is expected.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by comparing observer and snapshot | weak/shared layout is implementation-specific |
| ASan | no report expected | there is no invalid dereference |
| Debug CRT fills | not decisive | old bytes do not prove lifetime |
| Compiler warnings | no | the code is well formed |
