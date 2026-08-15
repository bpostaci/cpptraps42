# Trap21_CheckThenAct - CDB debug analysis

**Question:** if a check succeeded in the debugger, is a later use safe?

**Short answer:** no. The check and the use are separate events. CDB can make the window visible, but breakpoints serialize execution and can hide the race. Use CDB to explain the mechanism; use TSan on Clang/Linux or WSL for real threaded race detection. MSVC has ASan only.

---

## 1. Read the source first

```cpp
13  { std::scoped_lock lock(m); snapshot = shared; } // variant 1: safe handoff
23  { std::scoped_lock lock(m); has_item = !queue.empty(); } // unsafe CHECK
24  if (has_item) { std::scoped_lock lock(m); std::cout << queue.back(); queue.pop_back(); } // unsafe USE
36  if (std::filesystem::exists(path)) { // filesystem CHECK
39      std::cout << "file: " << line << '\n'; // filesystem USE
42  std::ifstream in(path); // safe: open first, then check handle
43  if (in) { std::string line; std::getline(in, line); std::cout << "file: " << line << '\n'; }
```

| Variant | Check | Use | Point |
|---|---|---|---|
| split locking | copy `shared` | dereference snapshot | one protocol must cover both |
| queue | `empty()` | `back()`/`pop_back()` | stale boolean is not ownership |
| filesystem | `exists()` | open/read | another process can change the path |

---

## 2. Build without a sanitizer

Use the existing `build\cdb\Debug` binaries. Do not clean or rebuild. This is not an MSVC sanitizer exercise: MSVC has no ThreadSanitizer or MemorySanitizer.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
  -srcpath C:\src\cpptraps42\Concurrency\Trap21_CheckThenAct ^
  C:\src\cpptraps42\build\cdb\Debug\Trap21_CheckThenAct.exe
```

Run `.symopt-100` and `.lines -e` first. `!locks` was tested here and failed without full `ntdll` symbols, so it is omitted.

---

## 4. The safe target

Variant 1 copies the shared pointer under one protocol.

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:13`
0:000> bp `main.cpp:26`
0:000> bp `main.cpp:27`
0:000> bp `main.cpp:42`
0:000> bp `main.cpp:43`
0:000> g
Breakpoint 0 hit
Trap21_CheckThenAct!check_then_act_lock+0x54:
00007ff6`79c93ef4 488d542430      lea     rdx,[rsp+30h]
0:000> $$ SAFE variant 1: shared_ptr snapshot under one protocol
0:000> dv /t /v
000000e4`068ff818 class std::scoped_lock<std::mutex> lock = ({...})
000000e4`068ff7b8 class std::shared_ptr<int> shared = 42
000000e4`068ff7e8 class std::shared_ptr<int> snapshot = empty
000000e4`068ff750 class std::mutex m = unlocked
0:000> ?? shared
class std::shared_ptr<int>
   +0x000 _Ptr             : 0x00000286`130ef970  -> 0n42
   +0x008 _Rep             : 0x00000286`130ef960 std::_Ref_count_base
```

Variant 2 keeps the check and pop under the same lock.

```text
0:000> g
Breakpoint 1 hit
Trap21_CheckThenAct!empty_then_pop+0xde:
00007ff6`79c940ae 488d542430      lea     rdx,[rsp+30h]
0:000> $$ SAFE variant 2: lock before check and pop
0:000> dv /t /v
000000e4`068ff7b8 class std::scoped_lock<std::mutex> lock = ({...})
000000e4`068ff768 class std::vector<int,std::allocator<int> > queue = { size=0x3 }
000000e4`068ff700 class std::mutex m = unlocked
0:000> g
Breakpoint 2 hit
Trap21_CheckThenAct!empty_then_pop+0xf1:
00007ff6`79c940c1 488d8c2498000000 lea     rcx,[rsp+98h]
0:000> $$ SAFE variant 2: check and pop still in same critical section
0:000> dv /t /v
000000e4`068ff7b8 class std::scoped_lock<std::mutex> lock = (locked)
000000e4`068ff768 class std::vector<int,std::allocator<int> > queue = { size=0x3 }
000000e4`068ff700 class std::mutex m = locked
```

Variant 3 opens first and checks the resulting stream.

```text
0:000> g
Breakpoint 3 hit
Trap21_CheckThenAct!exists_then_open+0x8b:
00007ff6`79c9420b c744242001000000 mov     dword ptr [rsp+20h],1 ss:000000e4`068ff510=00000001
0:000> $$ SAFE variant 3: open first, then test handle
0:000> dv /t /v
000000e4`068ff6a0 class std::basic_ifstream<char,std::char_traits<char> > in = class std::basic_ifstream<char,std::char_traits<char> >
000000e4`068ff808 class std::error_code ec = { value=-858993460, category={...} }
000000e4`068ff528 class std::filesystem::path path = "trap21_sample.txt"
0:000> g
lock: 42
pop: 3
file: data
```

---

## 5. The unsafe target

Variant 1 is unchanged in the unsafe target; variants 2 and 3 show the windows.

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:13`
0:000> bp `main.cpp:23`
0:000> bp `main.cpp:24`
0:000> bp `main.cpp:36`
0:000> bp `main.cpp:39`
0:000> g
Breakpoint 0 hit
Trap21_CheckThenAct_unsafe!check_then_act_lock+0x54:
00007ff7`f5794274 488d542430      lea     rdx,[rsp+30h]
0:000> $$ UNSAFE variant 1: same single-protocol handoff as safe target
0:000> dv /t /v
00000018`b0eff858 class std::scoped_lock<std::mutex> lock = ({...})
00000018`b0eff7f8 class std::shared_ptr<int> shared = 42
00000018`b0eff828 class std::shared_ptr<int> snapshot = empty
00000018`b0eff790 class std::mutex m = unlocked
```

Queue check and use are separate stops.

```text
0:000> g
Breakpoint 1 hit
Trap21_CheckThenAct_unsafe!empty_then_pop+0xde:
00007ff7`f579442e 488d542430      lea     rdx,[rsp+30h]
0:000> $$ UNSAFE variant 2 CHECK: empty observed while lock is scoped only to this line
0:000> dv /t /v
00000018`b0eff7c8 class std::scoped_lock<std::mutex> lock = ({...})
00000018`b0eff7b8 bool has_item = true
00000018`b0eff778 class std::vector<int,std::allocator<int> > queue = { size=0x3 }
00000018`b0eff710 class std::mutex m = unlocked
0:000> g
Breakpoint 2 hit
Trap21_CheckThenAct_unsafe!empty_then_pop+0x139:
00007ff7`f5794489 0fb68424d8000000 movzx   eax,byte ptr [rsp+0D8h] ss:00000018`b0eff7b8=01
0:000> $$ UNSAFE variant 2 USE: has_item is stale-capable after the lock was released
0:000> dv /t /v
00000018`b0eff7b8 bool has_item = true
00000018`b0eff778 class std::vector<int,std::allocator<int> > queue = { size=0x3 }
00000018`b0eff710 class std::mutex m = unlocked
```

The filesystem example has the same check/use shape.

```text
0:000> g
Breakpoint 3 hit
Trap21_CheckThenAct_unsafe!exists_then_open+0x8b:
00007ff7`f57945eb 488d4c2438      lea     rcx,[rsp+38h]
0:000> $$ UNSAFE variant 3 CHECK: exists observes the path now
0:000> dv /t /v
00000018`b0eff848 class std::error_code ec = { value=-858993460, category={...} }
00000018`b0eff568 class std::filesystem::path path = "trap21_sample.txt"
0:000> g
Breakpoint 4 hit
Trap21_CheckThenAct_unsafe!exists_then_open+0xea:
00007ff7`f579464a 488d152bef0100  lea     rdx,[Trap21_CheckThenAct_unsafe!std::_Digit_pairs<char>+0x8bc (00007ff7`f57b357c)]
0:000> $$ UNSAFE variant 3 USE: open/read relies on the earlier observation
0:000> dv /t /v
00000018`b0eff6e0 class std::basic_ifstream<char,std::char_traits<char> > in = class std::basic_ifstream<char,std::char_traits<char> >
00000018`b0eff808 class std::basic_string<char,std::char_traits<char>,std::allocator<char> > line = "data"
00000018`b0eff848 class std::error_code ec = { value=-858993460, category={...} }
00000018`b0eff568 class std::filesystem::path path = "trap21_sample.txt"
```

---

## 6. What the measurements prove

The safe target keeps check and use inside one protocol. The unsafe target shows a stale-capable value at the use. It does not force a failure; it illustrates the window. A debugger can hide the race by stopping the participants.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `bp `main.cpp:23`` | queue check |
| `bp `main.cpp:24`` | queue use |
| `bp `main.cpp:36`` | filesystem check |
| `bp `main.cpp:39`` | filesystem use |
| `dv /t /v` | inspect locals and lock state |
| `~`, `~*k` | inspect threads when needed |

---

## 8. Left to you

1. Add a second thread that drains the queue between check and use.
2. Delete or rename the file between `exists()` and open.
3. Replace `exists()`/open with open-and-check.
4. Build a true threaded queue variant under TSan on WSL.
5. Explain why a successful CDB run proves nothing about TOCTOU safety.

---

## 9. Tool limits

| Tool | Use here | Limitation |
|---|---|---|
| CDB | shows the window | serializes timing |
| MSVC ASan | memory errors | no TSan/MSan in MSVC |
| TSan | true data-race detection | Clang/Linux or WSL |
| code review | filesystem TOCTOU | must reason about external actors |
