# Trap14_Alignment - CDB debug analysis

**Question:** the unsafe target writes through a `uint64_t*` and x64 does not fault. Does that prove the pointer is correctly aligned?

**Short answer:** no. CDB shows the address is misaligned; x64 hardware tolerance is not C++ correctness. MSVC has AddressSanitizer only here, not UndefinedBehaviorSanitizer, so the runtime alignment diagnosis belongs to Clang UBSan.

---

## 1. Read the source first

```cpp
 1  #include <cstddef>
 2  #include <cstdint>
 3  #include <iostream>
 4  int main(){ alignas(std::uint64_t) std::byte storage[sizeof(std::uint64_t)+1];
 5  #if defined(RUN_UNSAFE_EXAMPLE)
 6      auto* p=reinterpret_cast<std::uint64_t*>(storage+1); *p=7; // BP: misaligned typed access.
 7  #else
 8      auto* p=reinterpret_cast<std::uint64_t*>(storage); *p=7; std::cout<<*p<<'\n';
 9  #endif
10  }
```

| Entity | Meaning |
|---|---|
| `storage` | 9 raw bytes, aligned for an 8-byte integer |
| safe `p` | `storage + 0`, should have low three bits zero |
| unsafe `p` | `storage + 1`, deliberately not 8-byte aligned |

---

## 2. Build without a sanitizer

Use the existing executables in `build\cdb\Debug`; do not clean or rebuild. The debugger stop is before the typed access. ASan is not an alignment checker, and MSVC does not provide UBSan.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\ObjectModel\Trap14_Alignment -cf %TEMP%\g2_trap14_safe2.cdb C:\src\cpptraps42\build\cdb\Debug\Trap14_Alignment.exe
```

Every script begins with `.symopt-100` so local names resolve and `.lines -e` so source breakpoints work.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:6`
0:000> g
Breakpoint 0 hit
Trap14_Alignment!main+0x26:
00007ff7`fa0a1556 488d442428      lea     rax,[rsp+28h]
0:000> $$ SAFE stop before aligned typed access
0:000> ?? &storage[0]
std::byte * 0x00000088`a7fff8a8
0:000> r $t0 = @@c++(&storage[0])
0:000> .printf "storage[0] / aligned p = %p\n", @$t0
storage[0] / aligned p = 00000088a7fff8a8
0:000> ? @$t0 & 7
Evaluate expression: 0 = 00000000`00000000
0:000> ?? sizeof(storage)
unsigned int64 9
0:000> dv /t /v
00000088`a7fff8c8 unsigned int64 * p = 0xcccccccc`cccccccc
00000088`a7fff8a8 std::byte [9] storage = std::byte [9]
0:000> g
7
```

`address & 7` is zero, so the base address is suitable for an 8-byte access.

---

## 5. The unsafe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:5`
0:000> g
Breakpoint 0 hit
Trap14_Alignment_unsafe!main+0x26:
00007ff6`40cc1506 488d442429      lea     rax,[rsp+29h]
0:000> $$ expression probes
0:000> ?? &storage[0]
std::byte * 0x000000bf`b5eff758
0:000> ?? &storage[1]
std::byte * 0x000000bf`b5eff759
0:000> r $t0 = @@c++(&storage[1])
0:000> .printf "storage[1] address = %p\n", @$t0
storage[1] address = 000000bfb5eff759
0:000> ? @$t0 & 7
Evaluate expression: 1 = 00000000`00000001
0:000> ?? sizeof(storage)
unsigned int64 9
0:000> dv /t /v
000000bf`b5eff778 unsigned int64 * p = 0xcccccccc`cccccccc
000000bf`b5eff758 std::byte [9] storage = std::byte [9]
0:000> g
ntdll!NtTerminateProcess+0x14:
00007ffc`a0500904 c3              ret
```

The unsafe address has low bits `001`. The process still exits; that observation proves only that this hardware and build tolerated the access.

---

## 6. What the measurements prove

| Measurement | Safe | Unsafe |
|---|---:|---:|
| storage size | `9` | `9` |
| chosen byte | `storage[0]` | `storage[1]` |
| `address & 7` | `0` | `1` |
| run result | prints `7` | no fault observed |

The divergence is the low-bit measurement. It is enough to identify the bad address, but not a substitute for UBSan's language-level diagnosis.

---

## 7. Command reference used here

| Command | Why type it |
|---|---|
| `.symopt-100` / `.lines -e` | make locals and line breakpoints usable |
| `?? &storage[n]` | ask CDB for the candidate byte address |
| `r $t0 = @@c++(...)` | pin the address for arithmetic |
| `? @$t0 & 7` | test 8-byte alignment |
| `dv /t /v` | confirm the local objects and types |

---

## 8. Left to you

1. Run the unsafe source with Clang UBSan and compare the diagnostic with CDB's low-bit evidence.
2. Change `storage+1` to `storage+4`; what does `address & 7` show?
3. Change the target type to `uint32_t`; which alignment mask is appropriate?
4. Compare x64 and ARM64 behavior. Does a hardware fault change the C++ rule?

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | shows misaligned address | does not enforce C++ UB |
| MSVC Debug | often runs anyway | hardware tolerance is not correctness |
| AddressSanitizer | no | not an alignment sanitizer |
| Clang UBSan | yes | separate toolchain needed |
