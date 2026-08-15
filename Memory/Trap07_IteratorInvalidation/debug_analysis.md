# Trap07_IteratorInvalidation - CDB debug analysis

**Question:** if an iterator's stored address still points somewhere, may it still be used after a container mutation?

**Short answer:** no. Validity is a container contract, not just an address. This lab compares vector reallocation, vector erase, and node-based map erasure.

---

## 1. Read the source first

```cpp
 6  void reallocation_invalidation() {
 7      std::vector<int> v{1,2,3}; v.shrink_to_fit();
 8      auto old = v.begin();
 9      int& old_reference = v.front();
10      const int* old_buffer = v.data();
11      v.push_back(4); // BP
14      std::cout << "realloc: " << *old << " / " << old_reference << '\n'; // unsafe BP
17      std::cout << "realloc: " << *v.begin() << " / " << v.front() << '\n';
22  void erase_invalidation() {
24      auto it = v.begin() + 1;
25      auto next = v.erase(it); // BP
27      std::cout << "erase: " << *it << '\n'; // unsafe BP
30      std::cout << "erase: " << *next << '\n';
35  void node_invalidation() {
37      auto erased = m.find(2); auto survivor = m.find(3);
39      m.erase(erased); // BP
41      std::cout << "node: " << erased->second << '\n'; // unsafe BP
44      std::cout << "node: survivor still valid: " << survivor->second << '\n';
```

| Entity | Invariant |
|---|---|
| `old`, `old_reference`, `old_buffer` | all die if vector growth reallocates |
| `next` | `erase` returns the valid successor |
| `survivor` | map erasing one node does not kill other nodes |

---

## 2. Build without a sanitizer

```powershell
cmake -S . -B build\cdb -DTRAPS_BUILD_UNSAFE=ON -DTRAPS_SANITIZER=none
cmake --build build\cdb --config Debug --target Trap07_IteratorInvalidation Trap07_IteratorInvalidation_unsafe
```

AddressSanitizer may catch some stale dereferences, but iterator validity is broader than allocator poisoning. The debugger is useful because it shows container internals and addresses before the invalid use.

---

## 3. Start CDB

Use `-srcpath C:\src\cpptraps42\Memory\Trap07_IteratorInvalidation`, then `.symopt-100` and `.lines -e`.

---

## 4. The safe target

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:11`
0:000> bp `main.cpp:12`
0:000> bp `main.cpp:25`
0:000> bp `main.cpp:30`
0:000> bp `main.cpp:39`
0:000> bp `main.cpp:44`
0:000> g
Breakpoint 0 hit
Trap07_IteratorInvalidation!reallocation_invalidation+0xff:
0:000> $$ ===== reallocation: before push_back =====
0:000> dv /t /v
0000002a`327dfce8 class std::_Vector_iterator<...> old = 1
0000002a`327dfd08 int * old_reference = 0x0000019e`f630ac40
0000002a`327dfd10 int * old_buffer = 0x0000019e`f630ac40
0000002a`327dfc98 class std::vector<int,std::allocator<int> > v = { size=0x3 }
0:000> ?? old._Ptr
int * 0x0000019e`f630ac40
0:000> ?? v._Mypair._Myval2._Myfirst
int * 0x0000019e`f630ac40
0:000> ?? v._Mypair._Myval2._Myend - v._Mypair._Myval2._Myfirst
int64 0n3
0:000> g
Breakpoint 1 hit
Trap07_IteratorInvalidation!reallocation_invalidation+0x11d:
0:000> $$ ===== reallocation: after push_back =====
0:000> dv /t /v
0000002a`327dfce8 class std::_Vector_iterator<...> old = -17891602
0000002a`327dfd10 int * old_buffer = 0x0000019e`f630ac40
0000002a`327dfc98 class std::vector<int,std::allocator<int> > v = { size=0x4 }
0:000> ?? old._Ptr
int * 0x0000019e`f630ac40
0:000> ?? v._Mypair._Myval2._Myfirst
int * 0x0000019e`f630cc30
0:000> ?? v._Mypair._Myval2._Myend - v._Mypair._Myval2._Myfirst
int64 0n4
0:000> g
Breakpoint 2 hit
Trap07_IteratorInvalidation!erase_invalidation+0x11a:
0:000> $$ ===== erase: before erase =====
0:000> ?? it._Ptr
int * 0x0000019e`f630aa34
0:000> ?? *it._Ptr
int 0n2
0:000> g
Breakpoint 3 hit
Trap07_IteratorInvalidation!erase_invalidation+0x15f:
0:000> $$ ===== erase: after erase, use returned iterator =====
0:000> ?? next._Ptr
int * 0x0000019e`f630aa34
0:000> ?? *next._Ptr
int 0n3
0:000> g
Breakpoint 4 hit
Trap07_IteratorInvalidation!node_invalidation+0x148:
0:000> $$ ===== node map: before erase =====
0:000> ?? erased._Ptr->_Myval.second
int 0n20
0:000> ?? survivor._Ptr->_Myval.second
int 0n30
0:000> g
Breakpoint 5 hit
Trap07_IteratorInvalidation!node_invalidation+0x19a:
0:000> $$ ===== node map: after erase, survivor remains valid =====
0:000> dv /t /v
0000002a`327dfd18 class std::_Tree_iterator<...> survivor = 3, 30
0000002a`327dfc98 class std::map<int,int,...> m = { size=0x2 }
0000002a`327dfce8 class std::_Tree_iterator<...> erased = end
0:000> ?? survivor._Ptr->_Myval.second
int 0n30
```

---

## 5. The unsafe target

Reallocation:

```
0:000> bp `main.cpp:14`
0:000> g
Breakpoint 0 hit
Trap07_IteratorInvalidation_unsafe!reallocation_invalidation+0x197:
0:000> $$ ===== reallocation unsafe: stale iterator and reference =====
0:000> dv /t /v
000000ba`94eff5f8 class std::_Vector_iterator<...> old = -17891602
000000ba`94eff618 int * old_reference = 0x000001bf`43b37100
000000ba`94eff620 int * old_buffer = 0x000001bf`43b37100
000000ba`94eff5a8 class std::vector<int,std::allocator<int> > v = { size=0x4 }
0:000> ?? old._Ptr
int * 0x000001bf`43b37100
0:000> ?? v._Mypair._Myval2._Myfirst
int * 0x000001bf`43b2f9f0
0:000> ?? v._Mypair._Myval2._Myend - v._Mypair._Myval2._Myfirst
int64 0n4
0:000> ?? *old._Ptr
int 0n-17891602
```

Erase:

```
0:000> bp Trap07_IteratorInvalidation_unsafe!main+0x6
0:000> bp `main.cpp:27`
0:000> g
Breakpoint 0 hit
Trap07_IteratorInvalidation_unsafe!main+0x6:
0:000> r rip = Trap07_IteratorInvalidation_unsafe!main+0xb
0:000> g
Breakpoint 1 hit
Trap07_IteratorInvalidation_unsafe!erase_invalidation+0x15f:
0:000> $$ ===== erase unsafe: erased iterator after erase =====
0:000> dv /t /v
00000095`a93af518 class std::_Vector_iterator<...> it = 3
00000095`a93af548 class std::_Vector_iterator<...> next = 3
00000095`a93af4c8 class std::vector<int,std::allocator<int> > v = { size=0x3 }
0:000> ?? it._Ptr
int * 0x000001df`614fa354
0:000> ?? next._Ptr
int * 0x000001df`614fa354
0:000> ?? *next._Ptr
int 0n3
0:000> ?? *it._Ptr
int 0n3
```

Node-based map:

```
0:000> bp Trap07_IteratorInvalidation_unsafe!main+0x6
0:000> bp `main.cpp:41`
0:000> g
Breakpoint 0 hit
Trap07_IteratorInvalidation_unsafe!main+0x6:
0:000> r rip = Trap07_IteratorInvalidation_unsafe!main+0x10
0:000> g
Breakpoint 1 hit
Trap07_IteratorInvalidation_unsafe!node_invalidation+0x19a:
0:000> $$ ===== node unsafe: erased map node after erase =====
0:000> dv /t /v
0000000c`e0eff788 class std::_Tree_iterator<...> survivor = 3, 30
0000000c`e0eff708 class std::map<int,int,...> m = { size=0x2 }
0000000c`e0eff758 class std::_Tree_iterator<...> erased = end
0:000> ?? erased._Ptr
struct std::_Tree_node<std::pair<int const ,int>,void *> * 0x00000147`f2789960
   +0x000 _Left            : 0xfeeefeee`feeefeee std::_Tree_node<std::pair<int const ,int>,void *>
0:000> ?? survivor._Ptr->_Myval.second
int 0n30
0:000> ?? erased._Ptr->_Myval.second
int 0n-17891602
```

---

## 6. What the measurements prove

| Variant | Before/after evidence | Rule |
|---|---|---|
| vector reallocation | old buffer `...ac40`, new buffer `...cc30` | all old iterators, references, and pointers are invalid |
| vector erase | `next` is the returned successor | the erased iterator must not be reused even if its raw pointer still reads `3` |
| map erase | survivor reads `30`; erased node shows `0xfeeefeee` fill | node-based containers preserve other iterators, not the erased node |

Do not transfer one container's invalidation rule to another. `std::map` preserving `survivor` says nothing about a `std::vector` iterator after reallocation.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `?? old._Ptr` | inspect the iterator's stored pointer |
| `?? v._Mypair._Myval2._Myfirst` | MSVC STL internal equivalent of `data()` |
| `?? v._Mypair._Myval2._Myend - ..._Myfirst` | capacity in this debug layout |
| `?? survivor._Ptr->_Myval.second` | inspect a map node value |
| `r rip = ...` | skip earlier unsafe variants to reach later ones |

---

## 8. Left to you

1. Add `v.reserve(4)` before capturing `old` and measure whether `push_back` still reallocates.
2. Replace `std::vector` with `std::deque` and record which addresses are stable.
3. Continue after the unsafe erase dereference under the debug STL and compare debugger output with program output.
4. Erase `survivor` instead and prove that `erased` is no longer the only dead iterator.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, with addresses and iterator internals | MSVC STL internals are implementation details |
| Debug STL checks | often | Debug-only and not a complete formal proof |
| AddressSanitizer | catches some stale storage reads | may not catch logical invalidation if storage remains live |
| Release build | usually no diagnosis | invalid use may appear to work |
