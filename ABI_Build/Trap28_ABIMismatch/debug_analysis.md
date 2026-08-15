# Trap28_ABIMismatch - CDB debug analysis

**Question:** is matching field spelling enough for a binary boundary?  
**Short answer:** no. Size, alignment, offsets, architecture, runtime library, packing, calling convention, ownership, and byte order are part of the contract. This executable simulates a real cross-module ABI mismatch by giving producer and consumer different packing.

---

## 1. Read the source first

Directory contents: `layout.hpp`, `main.cpp`, `producer.cpp`.

`layout.hpp`:
```cpp
 1  #pragma once
 2  #include <cstddef>
 3  struct Layout { std::size_t size; std::size_t alignment; std::size_t id_offset; };
 4  Layout producer_layout();
 5  Layout consumer_layout();
```

`producer.cpp`:
```cpp
 1  #include "layout.hpp"
 2  #include <cstdint>
 3  #pragma pack(push, 1)
 4  struct ProducerMessage { std::uint32_t version; std::uint64_t request_id; };
 5  #pragma pack(pop)
 6  Layout producer_layout() {
 7      return {sizeof(ProducerMessage), alignof(ProducerMessage), offsetof(ProducerMessage, request_id)};
 8  }
```

`main.cpp`:
```cpp
 1  #include "layout.hpp"
 2  #include <cstdint>
 3  #include <iostream>
 4  struct ConsumerMessage { std::uint32_t version; std::uint64_t request_id; };
 5  Layout consumer_layout() {
 6      return {sizeof(ConsumerMessage), alignof(ConsumerMessage), offsetof(ConsumerMessage, request_id)};
 7  }
 8  int main() {
 9      const Layout producer = producer_layout();
10      const Layout consumer = consumer_layout();
11      std::cout << "producer: size=" << producer.size << " align=" << producer.alignment
12                << " id-offset=" << producer.id_offset << '\n';
13      std::cout << "consumer: size=" << consumer.size << " align=" << consumer.alignment
14                << " id-offset=" << consumer.id_offset << '\n'; // BP: layouts disagree across boundary.
15  }
```

---

## 2. Build without a sanitizer

```powershell
cmake --build build\cdb --config Debug --target Trap28_ABIMismatch
```

No sanitizer diagnoses ABI mismatch. The debugger measures the ABI facts.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\ABI_Build\Trap28_ABIMismatch C:\src\cpptraps42\build\cdb\Debug\Trap28_ABIMismatch.exe
```

Enable `.symopt-100` and `.lines -e`.

---

## 4. The safe target

There is one target and it is a simulation, not two separately built modules.

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:11`
0:000> x Trap28_ABIMismatch!*layout*
00007ff6`7b2d25b0 Trap28_ABIMismatch!producer_layout (void)
00007ff6`7b2d1560 Trap28_ABIMismatch!consumer_layout (void)
0:000> dt Trap28_ABIMismatch!Layout
   +0x000 size             : Uint8B
   +0x008 alignment        : Uint8B
   +0x010 id_offset        : Uint8B
0:000> g
Breakpoint 0 hit
Trap28_ABIMismatch!main+0x6b:
00007ff6`7b2d163b 488d15c6960000  lea     rdx,[Trap28_ABIMismatch!__xt_z+0x268 (00007ff6`7b2dad08)]
0:000> dv /t /v
000000f5`13d7f7c8 struct Layout consumer = struct Layout
000000f5`13d7f798 struct Layout producer = struct Layout
0:000> ?? consumer
struct Layout
   +0x000 size             : 0x10
   +0x008 alignment        : 8
   +0x010 id_offset        : 8
0:000> ?? producer
struct Layout
   +0x000 size             : 0xc
   +0x008 alignment        : 1
   +0x010 id_offset        : 4
0:000> uf Trap28_ABIMismatch!producer_layout
Trap28_ABIMismatch!producer_layout [C:\src\cpptraps42\ABI_Build\Trap28_ABIMismatch\producer.cpp @ 6]:
    7 00007ff6`7b2d25d5 48c7000c000000  mov     qword ptr [rax],0Ch
    7 00007ff6`7b2d25e1 48c7400801000000 mov     qword ptr [rax+8],1
    7 00007ff6`7b2d25ee 48c7401004000000 mov     qword ptr [rax+10h],4
0:000> lm m Trap28_ABIMismatch
start             end                 module name
00007ff6`7b2d0000 00007ff6`7b2e5000   Trap28_ABIMismatch C (private pdb symbols)  d:\hive\cpptrapscode\build\cdb\debug\Trap28_ABIMismatch.pdb
0:000> g
producer: size=12 align=1 id-offset=4
consumer: size=16 align=8 id-offset=8
```

`dt Layout` shows the report structure. `?? producer` and `?? consumer` show the real mismatch: a packed producer offset 4 versus a naturally aligned consumer offset 8.

---

## 5. Why there is no unsafe target

There is no `Trap28_ABIMismatch_unsafe.exe`. The normal target already reports the disagreement. A production failure would usually involve an EXE/DLL, two DLLs, or another binary boundary built with different settings; this repository keeps it as an honest single-executable simulation.

---

## 6. What the measurements prove

| Check | Producer | Consumer | Risk |
|---|---:|---:|---|
| `sizeof` | 12 | 16 | buffer size disagreement |
| `alignof` | 1 | 8 | under-aligned storage risk |
| `request_id` offset | 4 | 8 | bytes decoded differently |
| `lm` | one module | one module | simulation, not real DLL split |

Reusable checklist: architecture, runtime library, packing, calling convention, ownership, byte order, size, alignment, member offsets, exception policy, and allocator contract.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `x <module>!*layout*` | find boundary-reporting functions |
| `dt Trap28_ABIMismatch!Layout` | show report member offsets |
| `` bp `main.cpp:11` `` | stop after both reports are computed |
| `dv /t /v` | locate locals |
| `?? producer`, `?? consumer` | read measured ABI facts |
| `uf producer_layout` | show constants from producer packing |
| `lm` | verify module boundary |

---

## 8. Left to you

1. Move `producer_layout` into a DLL and compare `lm` output.
2. Remove `#pragma pack(push, 1)` and rerun the measurements.
3. Add an owning pointer field and define who frees it.
4. Replace the struct boundary with explicit byte serialization and endian conversion.
5. Add `static_assert`s for size and offsets on both sides.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB `dt`/`??` | layout facts | cannot know intended ABI |
| `lm` | module list | not compiler flags by itself |
| Sanitizers | no | ABI mismatch is outside MSVC ASan |
| Compiler | sometimes | only if mismatch visible in one compilation |
