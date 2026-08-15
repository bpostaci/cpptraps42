# Trap42_ThreadJoinDetach - CDB debug analysis

**Question:** what happens when a joinable `std::thread` reaches its destructor?

**Short answer:** `std::terminate` is called. The unsafe target is expected to abort. CDB is useful here because it shows the destructor path and the thread state; it is still not a general proof tool for concurrent lifetime safety.

---

## 1. Read the source first

```cpp
15  std::thread t{worker, 1}; // unsafe: no join, no detach
19  std::thread t{worker, 1};
20  t.join();
27  std::jthread guarded{worker, 2};
28  throw std::runtime_error{"failure after the thread started"};
37  int local = 7;
38  std::thread t{[&local] { ... }};
42  t.join();
47  pool.emplace_back([i] { worker(i); });
```

| Variant | Trap | Safe pattern |
|---|---|---|
| joinable destruction | `~thread` while joinable | join or detach before destruction |
| exception path | manual join skipped | `std::jthread`/RAII join |
| detached data lifetime | reference outlives owner | keep owner alive or capture/own data |

---

## 2. Build without a sanitizer

Use the existing `build\cdb\Debug` binaries. Do not clean or rebuild. MSVC ASan may help with some detached lifetime bugs, but MSVC has no TSan or MSan.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
  -srcpath C:\src\cpptraps42\Concurrency\Trap42_ThreadJoinDetach ^
  C:\src\cpptraps42\build\cdb\Debug\Trap42_ThreadJoinDetach.exe
```

Run `.symopt-100` and `.lines -e` first. Use `~` and `~*k` for thread context. `!locks` was tested and omitted because required `ntdll` symbols were unavailable.

---

## 4. The safe target

Variant 1 joins before the `std::thread` destructor.

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:19`
0:000> bp `main.cpp:20`
0:000> bp `main.cpp:27`
0:000> bp `main.cpp:30`
0:000> bp `main.cpp:42`
0:000> g
Breakpoint 0 hit
Trap42_ThreadJoinDetach!destroyed_while_joinable+0x2b:
00007ff6`26e51e9b c744244401000000 mov     dword ptr [rsp+44h],1 ss:00000051`8bb2fb64=cccccccc
0:000> $$ SAFE variant 1: std::thread object before join
0:000> dv /t /v
00000051`8bb2fb48 class std::thread t = { id=0xcccccccc }
0:000> ?? t
class std::thread
   +0x000 _Thr             : _Thrd_t
0:000> g
Breakpoint 1 hit
Trap42_ThreadJoinDetach!destroyed_while_joinable+0x4a:
00007ff6`26e51eba 488d4c2428      lea     rcx,[rsp+28h]
0:000> $$ SAFE variant 1: join is called before destruction
0:000> dv /t /v
00000051`8bb2fb48 class std::thread t = { id=0x4200 }
```

Variant 2 uses `std::jthread`, so unwinding joins.

```text
0:000> g
Breakpoint 2 hit
Trap42_ThreadJoinDetach!exception_skips_manual_join+0x1a:
00007ff6`26e51f0a c744246402000000 mov     dword ptr [rsp+64h],2 ss:00000051`8bb2fb34=cccccccc
0:000> $$ SAFE variant 2: jthread starts before exception
0:000> dv /t /v
00000051`8bb2faf8 class std::jthread guarded = { id=0xcccccccc }
0:000> ?? guarded
class std::jthread
   +0x000 _Impl            : std::thread
   +0x010 _Ssource         : std::stop_source
0:000> g
(88fc.1e2c): C++ EH exception - code e06d7363 (first chance)
Breakpoint 3 hit
Trap42_ThreadJoinDetach!`exception_skips_manual_join'::`1'::catch$1+0x13:
00007ff6`26e5fac9 488d15e8260000  lea     rdx,[Trap42_ThreadJoinDetach!__xt_z+0x718 (00007ff6`26e621b8)]
0:000> $$ SAFE variant 2: exception caught after jthread destructor joined
0:000> dv /t /v
00000051`8bb2fb28 class std::exception * e = 0x00000051`8bb2fb38
```

Variant 3 keeps the referenced local alive until `join()`; the later pool uses value capture and `jthread`.

```text
0:000> g
Breakpoint 4 hit
Trap42_ThreadJoinDetach!detach_outlives_its_data+0x42:
00007ff6`26e51fc2 488d4c2448      lea     rcx,[rsp+48h]
0:000> $$ SAFE variant 3: local is captured, but this safe branch joins before scope exit
0:000> dv /t /v
00000051`8bb2faa4 int local = 0n7
00000051`8bb2fac8 class std::thread t = { id=0x2c50 }
00000051`8bb2faf8 class std::vector<std::jthread,std::allocator<std::jthread> > pool = { size=0x0 }
0:000> ?? local
int 0n7
0:000> g
joinable-destroy: skipped (~thread on a joinable thread calls std::terminate)
worker 1 done
worker 2 done
caught: failure after the thread started (jthread still joined)
pool size=3 (all joined at scope exit)
```

---

## 5. The unsafe target

The unsafe target aborts in variant 1, so variants 2 and 3 are not reached in that executable. That abort is the intended evidence.

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:15`
0:000> bm Trap42_ThreadJoinDetach_unsafe!std::thread::~thread
  1: 00007ff7`f7fb6ba0 @!"Trap42_ThreadJoinDetach_unsafe!std::thread::~thread"
0:000> g
Breakpoint 0 hit
Trap42_ThreadJoinDetach_unsafe!destroyed_while_joinable+0x17:
00007ff7`f7fb1e87 c744244401000000 mov     dword ptr [rsp+44h],1 ss:0000005a`c336f8a4=cccccccc
0:000> $$ UNSAFE variant 1: thread object has been created and will leave scope joinable
0:000> dv /t /v
0000005a`c336f888 class std::thread t = { id=0xcccccccc }
0:000> ?? t
class std::thread
   +0x000 _Thr             : _Thrd_t
0:000> g
Breakpoint 1 hit
Trap42_ThreadJoinDetach_unsafe!std::thread::~thread:
00007ff7`f7fb6ba0 48894c2408      mov     qword ptr [rsp+8],rcx ss:0000005a`c336f860=0000005ac336f888
0:000> $$ UNSAFE destructor path: std::thread::~thread was reached
0:000> k
Child-SP          RetAddr               Call Site
0000005a`c336f858 00007ff7`f7fb1eb0     Trap42_ThreadJoinDetach_unsafe!std::thread::~thread [C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\include\thread @ 96]
0000005a`c336f860 00007ff7`f7fb216b     Trap42_ThreadJoinDetach_unsafe!destroyed_while_joinable+0x40 [C:\src\cpptraps42\Concurrency\Trap42_ThreadJoinDetach\main.cpp @ 22]
0000005a`c336f8c0 00007ff7`f7fb9ec9     Trap42_ThreadJoinDetach_unsafe!main+0xb [C:\src\cpptraps42\Concurrency\Trap42_ThreadJoinDetach\main.cpp @ 54]
0:000> g
Debug Error!

Program: ...pTrapsCode\build\cdb\Debug\Trap42_ThreadJoinDetach_unsafe.exe

abort() has been called

(Press Retry to debug the application)
worker 1 done
ntdll!NtTerminateProcess+0x14:
00007ffc`a0500904 c3              ret
0:000> q
quit:
```

---

## 6. What the measurements prove

The destructor stack proves a joinable `std::thread` reached `~thread`; the abort proves `std::terminate` was called. The safe target covers all three variants: explicit join, exception-safe `jthread`, and data lifetime protected by joining/value capture.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `bp `main.cpp:15`` | stop where unsafe thread is created |
| `bm module!std::thread::~thread` | bind destructor breakpoint |
| `k` | show destructor call path |
| `dv /t /v` | inspect thread objects |
| `~`, `~*k` | inspect thread context |
| `g` | continue to the expected abort |

---

## 8. Left to you

1. Replace `std::thread` with `std::jthread` in variant 1.
2. Throw before a manual `join()` and compare with RAII joining.
3. Change the reference-capture example to `detach()` in a private experiment and run under sanitizers where available.
4. Capture worker data by value and explain the lifetime change.
5. Break on `std::terminate` and compare that stack with `~thread`.

---

## 9. Tool limits

| Tool | Use here | Limitation |
|---|---|---|
| CDB | shows destructor path and abort | not proof of all lifetime safety |
| MSVC ASan | some memory lifetime bugs | no TSan/MSan in MSVC |
| TSan | detached data races | Clang/Linux or WSL |
| repeated runs | variant 1 abort is deterministic | unsafe target cannot reach later variants |
