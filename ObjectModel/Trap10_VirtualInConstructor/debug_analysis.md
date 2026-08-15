# Trap10_VirtualInConstructor - CDB debug analysis

**Question:** why does `Base::Base()` call `Base::speak()` even though the object being constructed is a `Derived`?

**Short answer:** during base construction the object is in its base phase. That dispatch is defined behavior. CDB shows the mechanism: the vfptr first points at `Base::vftable`, then the derived constructor replaces it with `Derived::vftable`.

---

## 1. Read the source first

`main.cpp` (5 lines):

```cpp
 1  #include <iostream>
 2  struct Base { Base(){ speak(); } virtual ~Base()=default; virtual void speak(){std::cout<<"Base phase\n";} };
 3  struct Derived final: Base { int ready{99}; void speak() override {std::cout<<"Derived "<<ready<<'\n';} };
 4  int main(){ Derived d; // BP: Base constructor calls Base::speak; Derived part is not active yet.
 5              d.speak(); }
```

| Entity | Role |
|---|---|
| `Base::Base()` | installs the base vfptr and calls `speak()` |
| `Derived::Derived()` | runs after `Base::Base()` and installs the derived vfptr |
| `ready` | derived-only member at offset `+0x008` |

---

## 2. Build without a sanitizer

This document uses the existing `build\cdb\Debug\Trap10_VirtualInConstructor.exe` binary. No sanitizer is needed: the behavior is defined, and the evidence is object layout and dispatch state, not a memory error. If rebuilding in a private workspace, use the normal Debug target with `TRAPS_SANITIZER=none`.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
    -srcpath C:\src\cpptraps42\ObjectModel\Trap10_VirtualInConstructor ^
    C:\src\cpptraps42\build\cdb\Debug\Trap10_VirtualInConstructor.exe
```

Enable the two settings that are off by default:

```text
.symopt-100     resolve unqualified C++ names
.lines -e       make source-line breakpoints work
```

---

## 4. The safe target

Breakpoints: constructor offsets around the vfptr writes, both `speak()` implementations, and the call in `main`.

```text
0:000> dt Trap10_VirtualInConstructor!Base
   +0x000 __VFN_table : Ptr64 
0:000> dt Trap10_VirtualInConstructor!Derived
   +0x000 __VFN_table : Ptr64 
   +0x008 ready            : Int4B
0:000> bp Trap10_VirtualInConstructor!Base::Base+0x19
0:000> bp Trap10_VirtualInConstructor!Base::speak+0xa
0:000> bp Trap10_VirtualInConstructor!Derived::Derived+0x14
0:000> bp Trap10_VirtualInConstructor!Derived::Derived+0x23
0:000> bp Trap10_VirtualInConstructor!Derived::Derived+0x2f
0:000> bp Trap10_VirtualInConstructor!Derived::speak+0xa
0:000> bp `main.cpp:5`
0:000> g
Breakpoint 0 hit
Trap10_VirtualInConstructor!Base::Base+0x19:
00007ff7`b9e02099 488b4c2430      mov     rcx,qword ptr [rsp+30h] ss:000000e0`9ff0f5d0=000000e09ff0f628
0:000> $$ ===== STOP A: Base constructor installed Base vfptr, before speak() =====
0:000> .printf "this = %p\n", poi(@rsp+30)
this = 000000e09ff0f628
0:000> dps poi(@rsp+30) L1
000000e0`9ff0f628  00007ff7`b9e0afa0 Trap10_VirtualInConstructor!Base::`vftable'
0:000> k L4
Child-SP          RetAddr               Call Site
000000e0`9ff0f5a0 00007ff7`b9e020d4     Trap10_VirtualInConstructor!Base::Base+0x19
000000e0`9ff0f5d0 00007ff7`b9e01601     Trap10_VirtualInConstructor!Derived::Derived+0x14
000000e0`9ff0f600 00007ff7`b9e02d79     Trap10_VirtualInConstructor!main+0x21
000000e0`9ff0f660 00007ff7`b9e02c22     Trap10_VirtualInConstructor!invoke_main+0x39
0:000> g
Breakpoint 1 hit
Trap10_VirtualInConstructor!Base::speak+0xa:
00007ff7`b9e0256a 488d15478a0000  lea     rdx,[Trap10_VirtualInConstructor!`string' (00007ff7`b9e0afb8)]
0:000> $$ ===== STOP B: Base::speak reached during base construction =====
0:000> .printf "this = %p\n", poi(@rsp+30)
this = 000000e09ff0f628
0:000> dps poi(@rsp+30) L1
000000e0`9ff0f628  00007ff7`b9e0afa0 Trap10_VirtualInConstructor!Base::`vftable'
0:000> k L4
Child-SP          RetAddr               Call Site
000000e0`9ff0f570 00007ff7`b9e020a3     Trap10_VirtualInConstructor!Base::speak+0xa
000000e0`9ff0f5a0 00007ff7`b9e020d4     Trap10_VirtualInConstructor!Base::Base+0x23
000000e0`9ff0f5d0 00007ff7`b9e01601     Trap10_VirtualInConstructor!Derived::Derived+0x14
000000e0`9ff0f600 00007ff7`b9e02d79     Trap10_VirtualInConstructor!main+0x21
0:000> g
Breakpoint 2 hit
Trap10_VirtualInConstructor!Derived::Derived+0x14:
00007ff7`b9e020d4 488b442430      mov     rax,qword ptr [rsp+30h] ss:000000e0`9ff0f600=000000e09ff0f628
0:000> $$ ===== STOP C: Derived constructor after Base::Base returned =====
0:000> .printf "this = %p\n", poi(@rsp+30)
this = 000000e09ff0f628
0:000> dps poi(@rsp+30) L1
000000e0`9ff0f628  00007ff7`b9e0afa0 Trap10_VirtualInConstructor!Base::`vftable'
0:000> g
Breakpoint 3 hit
Trap10_VirtualInConstructor!Derived::Derived+0x23:
00007ff7`b9e020e3 488b442430      mov     rax,qword ptr [rsp+30h] ss:000000e0`9ff0f600=000000e09ff0f628
0:000> $$ ===== STOP D: Derived constructor has replaced the vfptr =====
0:000> .printf "this = %p\n", poi(@rsp+30)
this = 000000e09ff0f628
0:000> dps poi(@rsp+30) L1
000000e0`9ff0f628  00007ff7`b9e0afd0 Trap10_VirtualInConstructor!Derived::`vftable'
0:000> dd poi(@rsp+30)+8 L1
000000e0`9ff0f630  cccccccc
0:000> g
Breakpoint 4 hit
Trap10_VirtualInConstructor!Derived::Derived+0x2f:
00007ff7`b9e020ef 488b442430      mov     rax,qword ptr [rsp+30h] ss:000000e0`9ff0f600=000000e09ff0f628
0:000> $$ ===== STOP E: Derived member initializer has written ready =====
0:000> dps poi(@rsp+30) L1
000000e0`9ff0f628  00007ff7`b9e0afd0 Trap10_VirtualInConstructor!Derived::`vftable'
0:000> dd poi(@rsp+30)+8 L1
000000e0`9ff0f630  00000063
0:000> g
Breakpoint 6 hit
Trap10_VirtualInConstructor!main+0x22:
00007ff7`b9e01602 488d4c2428      lea     rcx,[rsp+28h]
0:000> $$ ===== STOP F: in main before d.speak() =====
0:000> dps @@c++(&d) L1
000000e0`9ff0f628  00007ff7`b9e0afd0 Trap10_VirtualInConstructor!Derived::`vftable'
0:000> ?? d.ready
int 0n99
0:000> g
Breakpoint 5 hit
Trap10_VirtualInConstructor!Derived::speak+0xa:
00007ff7`b9e0259a 488d15478a0000  lea     rdx,[Trap10_VirtualInConstructor!`string' (00007ff7`b9e0afe8)]
0:000> $$ ===== STOP G: Derived::speak reached after construction =====
0:000> dps poi(@rsp+40) L1
000000e0`9ff0f628  00007ff7`b9e0afd0 Trap10_VirtualInConstructor!Derived::`vftable'
0:000> dd poi(@rsp+40)+8 L1
000000e0`9ff0f630  00000063
0:000> g
Base phase
Derived 99
```

---

## 5. Why there is no unsafe target

`build\cdb\Debug` contains `Trap10_VirtualInConstructor.exe` but no `Trap10_VirtualInConstructor_unsafe.exe`. The source has no `RUN_UNSAFE_EXAMPLE` branch. That is accurate for this trap: the virtual call in the base constructor is not undefined behavior. It is defined dispatch to the base implementation while the base subobject is under construction.

---

## 6. What the measurements prove

| Stop | Object address | vfptr target | `ready` bytes | Meaning |
|---|---:|---|---|---|
| A/B | `000000e09ff0f628` | `Base::vftable` | not active | base phase; `Base::speak()` is selected |
| C | same | `Base::vftable` | not active | base constructor has returned, derived constructor has not rewritten vfptr yet |
| D | same | `Derived::vftable` | `cccccccc` | derived vfptr is installed before the member initializer shown here completes |
| E/G | same | `Derived::vftable` | `00000063` | complete derived state; `Derived::speak()` is selected |

The address did not change. The vfptr did. That divergence is the lesson.

---

## 7. Command reference used here

| Command | Why it was used |
|---|---|
| `dt module!Type` | show vfptr and member offsets |
| `bp symbol+offset` | stop after specific constructor writes, not just at function entry |
| `dps address L1` | dump the vfptr and resolve it to a vftable symbol |
| `dd address+8 L1` | read the `int ready` member at offset `+0x008` |
| `k L4` | prove which constructor or virtual function is on the stack |

---

## 8. Left to you

1. Break at `Base::Base+0x19` and change the next `g` to single-instruction stepping. Which instruction writes the base vfptr?
2. Add a second data member to `Derived` in a scratch copy. Does the vfptr offset change?
3. Replace `d.speak()` in `main` with `Base& b = d; b.speak();`. Which vftable does CDB show at the call?
4. Set a breakpoint on `Base::~Base`. During destruction, when does the vfptr stop identifying the most-derived type?

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB `dps` | yes | shows MSVC layout for this build, not a portable ABI guarantee |
| Sanitizers | no | no invalid access occurs |
| Compiler warnings | maybe | some tools warn about virtual calls in constructors, but the call is still defined |
| Source review | yes | explains the rule, but not the actual vfptr transition |
