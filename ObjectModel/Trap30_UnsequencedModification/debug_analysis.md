# Trap30_UnsequencedModification - CDB debug analysis

**Question:** the unsafe executable prints a stable value on this machine. Is that the value of `value++ + ++value`?

**Short answer:** no. The expression modifies the same scalar more than once without sequencing, so the source has undefined behavior. CDB can show the order this MSVC Debug build emitted; that order is not a portable answer.

---

## 1. Read the source first

```cpp
 1  #include <iostream>
 2  int main() {
 3      int value = 1;
 4  #if defined(RUN_UNSAFE_EXAMPLE)
 5      // ANTI-PATTERN: multiple unsequenced modifications of the same scalar object.
 6      value = value++ + ++value; // BP
 7  #else
 8      // CORRECT: express each state transition as a separate sequenced statement.
 9      const int old = value;
10      ++value;
11      const int after_first_increment = value;
12      ++value;
13      value = old + after_first_increment;
14  #endif
15      std::cout << value << '\n';
16  }
```

| Entity | Meaning |
|---|---|
| `value` | scalar modified more than once in the unsafe full-expression |
| `old` | safe copy of the initial state |
| `after_first_increment` | safe named state after one sequenced increment |

---

## 2. Build without a sanitizer

Use the existing Debug executables. I compiled the source only as a scratch object with `/DRUN_UNSAFE_EXAMPLE /W4 /Wall` to look for an MSVC diagnostic; this compiler emitted only `main.cpp` and no warning. The shared build directory was not cleaned or rebuilt.

---

## 3. Start CDB

Use `.symopt-100` and `.lines -e`. For this trap, `u`/`uf` is evidence of the emitted order, not a proof that the source expression has meaning.

---

## 4. The safe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:10`
0:000> bp `main.cpp:16`
0:000> g
Breakpoint 0 hit
Trap30_UnsequencedModification!main+0xe:
00007ff6`9943153e 8b442420        mov     eax,dword ptr [rsp+20h] ss:00000048`2feff6a0=00000001
0:000> $$ SAFE stop before sequenced statements
0:000> dv /t /v
00000048`2feff6a4 int old = 0n-1139227632
00000048`2feff6a8 int after_first_increment = 0n0
00000048`2feff6a0 int value = 0n1
0:000> g
Breakpoint 1 hit
Trap30_UnsequencedModification!main+0x42:
00007ff6`99431572 8b542420        mov     edx,dword ptr [rsp+20h] ss:00000048`2feff6a0=00000003
0:000> $$ SAFE stop before printing result
0:000> dv /t /v
00000048`2feff6a4 int old = 0n1
00000048`2feff6a8 int after_first_increment = 0n2
00000048`2feff6a0 int value = 0n3
0:000> ?? value
int 0n3
0:000> ?? old
int 0n1
0:000> ?? after_first_increment
int 0n2
0:000> g
3
```

---

## 5. The unsafe target

```text
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:7`
0:000> bp `main.cpp:16`
0:000> g
Breakpoint 0 hit
Trap30_UnsequencedModification_unsafe!main+0xe:
00007ff6`a1a2153e 8b442420        mov     eax,dword ptr [rsp+20h] ss:00000061`900ffcd0=00000001
0:000> $$ UNSAFE stop before one unsequenced full-expression
0:000> dv /t /v
00000061`900ffcd0 int value = 0n1
0:000> u Trap30_UnsequencedModification_unsafe!main+0xe L12
Trap30_UnsequencedModification_unsafe!main+0xe [C:\src\cpptraps42\ObjectModel\Trap30_UnsequencedModification\main.cpp @ 7]:
00007ff6`a1a2153e 8b442420        mov     eax,dword ptr [rsp+20h]
00007ff6`a1a21542 ffc0            inc     eax
00007ff6`a1a21544 89442420        mov     dword ptr [rsp+20h],eax
00007ff6`a1a21548 8b442420        mov     eax,dword ptr [rsp+20h]
00007ff6`a1a2154c 8b4c2420        mov     ecx,dword ptr [rsp+20h]
00007ff6`a1a21550 03c8            add     ecx,eax
00007ff6`a1a21552 8bc1            mov     eax,ecx
00007ff6`a1a21554 89442424        mov     dword ptr [rsp+24h],eax
00007ff6`a1a21558 8b442420        mov     eax,dword ptr [rsp+20h]
00007ff6`a1a2155c ffc0            inc     eax
00007ff6`a1a2155e 89442420        mov     dword ptr [rsp+20h],eax
00007ff6`a1a21562 8b442424        mov     eax,dword ptr [rsp+24h]
00007ff6`a1a21566 89442420        mov     dword ptr [rsp+20h],eax
0:000> g
Breakpoint 1 hit
Trap30_UnsequencedModification_unsafe!main+0x3a:
00007ff6`a1a2156a 8b542420        mov     edx,dword ptr [rsp+20h] ss:00000061`900ffcd0=00000004
0:000> $$ UNSAFE stop before printing compiler-chosen result
0:000> dv /t /v
00000061`900ffcd0 int value = 0n4
0:000> ?? value
int 0n4
0:000> g
4
```

---

## 6. What the measurements prove

| Measurement | Safe target | Unsafe target |
|---|---|---|
| starting `value` | `1` | `1` |
| source operations | separate statements | one unsequenced full-expression |
| named intermediate state | `old=1`, `after_first_increment=2` | none |
| value before print | `3` | `4` in this build |
| conclusion | well-defined | undefined behavior despite stable output |

The unsafe disassembly explains this run's `4`: this build increments, reads twice, adds, increments again, then stores the temporary. It does not make the source expression valid.

---

## 7. Command reference used here

| Command | Why type it |
|---|---|
| `bp main.cpp:7` | stop before the unsequenced full-expression |
| `bp main.cpp:16` | inspect what will be printed |
| `u address L12` | see the emitted instruction order |
| `uf module!main` | inspect line-to-instruction mapping |
| `dv /t /v` | compare locals before and after the expression |

---

## 8. Left to you

1. Build a scratch optimized object. Does the emitted order or printed value change?
2. Compile with Clang or GCC warnings. Which diagnostic do they produce?
3. Split the unsafe expression into two differently ordered safe versions. Which results are well-defined?
4. Break at individual instruction addresses on line 7. Does stepping make undefined behavior defined?

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB disassembly | shows one emitted order | not a portable semantic answer |
| MSVC warnings here | no warning observed | silence does not prove defined behavior |
| AddressSanitizer | no | not a sequencing sanitizer |
| Language rules | yes | must be applied before trusting output |
