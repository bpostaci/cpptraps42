# Trap32_SharedPtrCycle - CDB debug analysis

**Question:** why do two `shared_ptr` nodes leak even though every local variable goes out of scope?

**Short answer:** the locals are not the only owners. After the back edge is assigned, each node is also owned by the other node. Scope exit drops the local owners, but the cycle keeps both reference counts above zero, so no destructor runs.

---

## 1. Read the source first

```cpp
 5  struct CyclicNode {
 6      std::shared_ptr<CyclicNode> peer;
 7      int id;
 8      explicit CyclicNode(int value) : id(value) { std::cout << "ctor  " << id << '\n'; }
 9      ~CyclicNode() { std::cout << "dtor  " << id << '\n'; } // BP: never reached in the cyclic case.
10  };
12  void shared_ptr_cycle_leaks() {
13      auto a = std::make_shared<CyclicNode>(1);
14      auto b = std::make_shared<CyclicNode>(2);
15      a->peer = b;
16      b->peer = a; // BP: use_count of both is now 2; scope exit only drops it back to 1.
17      std::cout << "cycle use_count: " << a.use_count() << '\n';
18  } // BP: no destructor output here - the two nodes are leaked.
29  void weak_ptr_breaks_cycle() {
30      auto a = std::make_shared<WeakNode>(1);
31      auto b = std::make_shared<WeakNode>(2);
32      a->next = b;
33      b->prev = a; // Owning count of 'a' stays 1, so scope exit destroys both.
34      std::cout << "weak use_count: " << a.use_count() << '\n';
35  }
38  void locking_a_weak_ptr() {
39      std::weak_ptr<WeakNode> observer;
40      {
41          auto owner = std::make_shared<WeakNode>(3);
42          observer = owner;
43          if (auto locked = observer.lock()) {
44              std::cout << "locked: " << locked->id << '\n';
45          }
46      }
47      std::cout << "expired: " << std::boolalpha << observer.expired() << '\n';
48  }
```

| Variant | Key entity | What changes |
|---|---|---|
| owning cycle | `a._Rep->_Uses`, `b._Rep->_Uses` | back edge adds ownership |
| weak back edge | `a` remains singly owned | observer does not increase `_Uses` |
| expired observer | `observer._Rep->_Uses` | drops to zero after owner scope |

---

## 2. Build without a sanitizer

I used the existing `Trap32_SharedPtrCycle.exe` in `build\cdb\Debug`. There is no `_unsafe` sibling in that directory. This is defined C++ behavior and a resource leak, not a crash.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
    -srcpath C:\src\cpptraps42\Lifetime\Trap32_SharedPtrCycle ^
    C:\src\cpptraps42\build\cdb\Debug\Trap32_SharedPtrCycle.exe
```

Use `.symopt-100` for local names and `.lines -e` for source-line breakpoints.

---

## 4. The safe target

This target contains all three variants. The transcript uses MSVC's `_Rep->_Uses` control-block field after confirming the layout with `?? a`.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:16`
0:000> bp `main.cpp:17`
0:000> bp `main.cpp:18`
0:000> bp `main.cpp:33`
0:000> bp `main.cpp:34`
0:000> bp `main.cpp:35`
0:000> g
Breakpoint 0 hit
Trap32_SharedPtrCycle!shared_ptr_cycle_leaks+0x61:
00007ff6`8cca19f1 488d4c2458      lea     rcx,[rsp+58h]
0:000> $$ ===== variant 1 before assigning back edge =====
0:000> ?? a._Rep->_Uses
unsigned long 1
0:000> ?? b._Rep->_Uses
unsigned long 2
0:000> g
Breakpoint 1 hit
Trap32_SharedPtrCycle!shared_ptr_cycle_leaks+0x78:
00007ff6`8cca1a08 488d15a1c10000  lea     rdx,[Trap32_SharedPtrCycle!__xt_z+0x110 (00007ff6`8ccadbb0)]
0:000> $$ ===== variant 1 after assigning back edge =====
0:000> ?? a._Rep->_Uses
unsigned long 2
0:000> ?? b._Rep->_Uses
unsigned long 2
0:000> g
Breakpoint 2 hit
Trap32_SharedPtrCycle!shared_ptr_cycle_leaks+0xc4:
00007ff6`8cca1a54 488d4c2458      lea     rcx,[rsp+58h]
0:000> $$ ===== variant 1 at scope exit before locals are destroyed =====
0:000> ?? a._Rep->_Uses
unsigned long 2
0:000> ?? b._Rep->_Uses
unsigned long 2
0:000> g
Breakpoint 3 hit
Trap32_SharedPtrCycle!weak_ptr_breaks_cycle+0x61:
00007ff6`8cca1af1 488d4c2458      lea     rcx,[rsp+58h]
0:000> $$ ===== variant 2 before weak back edge =====
0:000> ?? a._Rep->_Uses
unsigned long 1
0:000> ?? b._Rep->_Uses
unsigned long 2
0:000> g
Breakpoint 4 hit
Trap32_SharedPtrCycle!weak_ptr_breaks_cycle+0x7c:
00007ff6`8cca1b0c 488d15b5c00000  lea     rdx,[Trap32_SharedPtrCycle!__xt_z+0x128 (00007ff6`8ccadbc8)]
0:000> $$ ===== variant 2 after weak back edge =====
0:000> ?? a._Rep->_Uses
unsigned long 1
0:000> ?? b._Rep->_Uses
unsigned long 2
```

For the weak observer variant:

```
0:000> bp `main.cpp:44`
0:000> bp `main.cpp:47`
0:000> g
Breakpoint 0 hit
Trap32_SharedPtrCycle!locking_a_weak_ptr+0x7a:
00007ff6`8cca1c0a 488d15cfbf0000  lea     rdx,[Trap32_SharedPtrCycle!__xt_z+0x140 (00007ff6`8ccadbe0)]
0:000> $$ ===== variant 3 locked owner exists =====
0:000> dv /t /v
0000003f`12effbb8 class std::shared_ptr<WeakNode> locked = {...}
0000003f`12effb88 class std::shared_ptr<WeakNode> owner = {...}
0000003f`12effb58 class std::weak_ptr<WeakNode> observer = {...}
0:000> ?? observer._Rep->_Uses
unsigned long 2
0:000> ?? locked._Rep->_Uses
unsigned long 2
0:000> g
Breakpoint 1 hit
Trap32_SharedPtrCycle!locking_a_weak_ptr+0xe5:
00007ff6`8cca1c75 488d1574bf0000  lea     rdx,[Trap32_SharedPtrCycle!__xt_z+0x150 (00007ff6`8ccadbf0)]
0:000> $$ ===== variant 3 after owner scope ended =====
0:000> dv /t /v
0000003f`12effb58 class std::weak_ptr<WeakNode> observer = {...}
0:000> ?? observer._Rep->_Uses
unsigned long 0
0:000> ?? observer._Rep->_Weaks
unsigned long 1
0:000> g
ctor  1
ctor  2
cycle use_count: 2
weak use_count: 1
dtor  weak1
dtor  weak2
locked: 3
dtor  weak3
expired: true
```

---

## 5. Why there is no unsafe target

`Trap32_SharedPtrCycle_unsafe.exe` does not exist. The cyclic version is already the teaching case. It does not dereference invalid memory and it does not need a separate crash target. ASan leak detection can report leaked allocations at process exit, but it reports the leak, not the ownership cycle that caused it. The missing `dtor  1` and `dtor  2` messages are the debugger evidence.

---

## 6. What the measurements prove

| Stop | `a` uses | `b` uses | Destructor evidence |
|---|---:|---:|---|
| before `b->peer = a` | 1 | 2 | both nodes still reachable |
| after `b->peer = a` | 2 | 2 | cycle formed |
| at cyclic scope exit | 2 | 2 | locals will drop counts to 1, not 0 |
| weak back edge after assignment | 1 | 2 | `dtor  weak1`, `dtor  weak2` later appear |
| observer after owner scope | 0 owning uses | 1 weak count | expired observer locks to no owner |

The leak is proved by a count that cannot reach zero plus missing destructors. Nothing needs to crash.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `?? a` | discover MSVC `shared_ptr` layout |
| `?? a._Rep->_Uses` | inspect the owning reference count |
| `bp main.cpp:16/17/18` | compare before, after, and at scope exit |
| `dv /t /v` | confirm which smart pointers are in scope |
| `g` | include program output, especially destructor messages |

---

## 8. Left to you

1. Break in `CyclicNode::~CyclicNode`. Why is it not hit for the first variant?
2. Change `peer` to `weak_ptr` and remeasure `_Uses` at line 18.
3. Run an ASan leak build and compare the leak report with the count evidence.
4. Add `a->peer.reset()` before line 18. Which destructor messages return?
5. Inspect `_Weaks` as well as `_Uses` and explain why the control block can outlive the object.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by counts and missing destructors | uses MSVC internal field names |
| ASan leak detection | reports leaked blocks | does not identify the cycle itself |
| Compiler warnings | no | ownership graph is runtime state |
| Crash dump | usually no | the program exits normally |
