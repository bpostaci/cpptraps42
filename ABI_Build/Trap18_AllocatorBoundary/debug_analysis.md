# Trap18_AllocatorBoundary - CDB debug analysis

**Question:** may memory allocated on one side of a boundary be released by any allocator?  
**Short answer:** no. Allocation and release must use the same allocator family or an explicit exported destroy contract.

---

## 1. Read the source first

Directory contents: `main.cpp`.

```cpp
 1  #include <cstdlib>
 2  #include <iostream>
 3  struct FreeDeleter { void operator()(void* p) const { std::free(p); } };
 4  #include <memory>
 5  int main(){ std::unique_ptr<void,FreeDeleter> buffer(std::malloc(256)); // BP: matching allocator/deallocator.
 6      std::cout<<(buffer?"allocated":"failed")<<'\n';
 7      // Across a DLL, export a matching destroy function or caller-owned buffer contract.
 8  }
```

| Entity | Role |
|---|---|
| `std::malloc(256)` | allocation family: C heap |
| `FreeDeleter::operator()` | release path calls `std::free` |
| `unique_ptr<void, FreeDeleter>` | stores the release policy with the pointer |

---

## 2. Build without a sanitizer

```powershell
cmake --build build\cdb --config Debug --target Trap18_AllocatorBoundary
```

The build already exists. ASan is not the primary tool: allocator-boundary mistakes are contract mistakes and may happen to work in one Debug executable.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug -srcpath C:\src\cpptraps42\ABI_Build\Trap18_AllocatorBoundary C:\src\cpptraps42\build\cdb\Debug\Trap18_AllocatorBoundary.exe
```

Use `.symopt-100` for C++ names and `.lines -e` for source-line breakpoints.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:6`
0:000> bm Trap18_AllocatorBoundary!FreeDeleter::operator()
  1: 00007ff7`a0ed2450 @!"Trap18_AllocatorBoundary!FreeDeleter::operator()"
0:000> g
Breakpoint 0 hit
Trap18_AllocatorBoundary!main+0x30:
00007ff7`a0ed15b0 488b0581eb0000  mov     rax,qword ptr [Trap18_AllocatorBoundary!_imp_?coutstd (00007ff7`a0ee0138)] ds:00007ff7`a0ee0138={MSVCP140D!std::cout (00007ffc`5a2eb040)}
0:000> ?? buffer._Mypair._Myval2
void * 0x000001c8`dc894490
0:000> r $t0 = @@c++(buffer._Mypair._Myval2)
0:000> .printf "allocated pointer = %p\n", @$t0
allocated pointer = 000001c8dc894490
0:000> db @$t0 L10
000001c8`dc894490  cd cd cd cd cd cd cd cd-cd cd cd cd cd cd cd cd  ................
0:000> g
Breakpoint 1 hit
Trap18_AllocatorBoundary!FreeDeleter::operator():
00007ff7`a0ed2450 4889542410      mov     qword ptr [rsp+10h],rdx ss:000000b0`da0ffa68=cccccccccccccccc
0:000> .printf "deleter receives p = %p\n", @rdx
deleter receives p = 000001c8dc894490
0:000> k
Child-SP          RetAddr               Call Site
000000b0`da0ffa58 00007ff7`a0ed22e3     Trap18_AllocatorBoundary!FreeDeleter::operator() [C:\src\cpptraps42\ABI_Build\Trap18_AllocatorBoundary\main.cpp @ 3]
000000b0`da0ffa60 00007ff7`a0ed160b     Trap18_AllocatorBoundary!std::unique_ptr<void,FreeDeleter>::~unique_ptr<void,FreeDeleter>+0x43 [C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\include\memory @ 3456]
000000b0`da0ffaa0 00007ff7`a0ed2c29     Trap18_AllocatorBoundary!main+0x8b [C:\src\cpptraps42\ABI_Build\Trap18_AllocatorBoundary\main.cpp @ 8]
0:000> g
allocated
ntdll!NtTerminateProcess+0x14:
00007ffc`a0500904 c3              ret
```

The allocated address and the deleter argument match. The call stack proves the release path is `unique_ptr` destructor -> `FreeDeleter::operator()` -> `std::free`.

---

## 5. Why there is no unsafe target

`build\cdb\Debug` has no `Trap18_AllocatorBoundary_unsafe.exe`. A convincing unsafe case needs two independently built modules or allocator families. In one Debug executable, the repository teaches the reusable contract instead of manufacturing a fragile crash.

---

## 6. What the measurements prove

| Measurement | Evidence | Meaning |
|---|---|---|
| allocation | `0x...dc894490` | `malloc` result stored in owner |
| bytes | `cd cd ...` | Debug CRT allocated-uninitialized fill |
| release argument | same address in `rdx` | same pointer is released |
| stack | `~unique_ptr` -> deleter | release is tied to scope |

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `` bp `main.cpp:6` `` | stop after allocation |
| `bm ...FreeDeleter::operator()` | stop in the deleter |
| `?? buffer._Mypair._Myval2` | inspect stored pointer |
| `db` | inspect bytes |
| `k` | prove the release path |
| `lm` | use in real DLL cases to verify modules |

---

## 8. Left to you

1. Replace `free` with the wrong deallocator and inspect the first divergent stack frame.
2. Move allocation into a DLL and export a destroy function; verify modules with `lm`.
3. Build two sides with different runtime-library settings and compare behavior.
4. Change the interface to caller-owned storage; what release contract remains?
5. Repeat in Release and note which Debug CRT byte evidence disappears.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | release path | cannot prove all ABI contracts |
| Debug CRT fills | heap-state hints | Debug only |
| ASan | some mismatches | changes allocator behavior |
| Compiler | usually no | allocator family is often invisible |
