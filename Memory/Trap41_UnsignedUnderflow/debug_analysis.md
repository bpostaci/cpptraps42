# Trap41_UnsignedUnderflow - CDB debug analysis

**Question:** Is `size() - 1` on an empty container a memory bug?

**Short answer:** No. Unsigned arithmetic wraps modulo its range. The result is huge, defined, and usually the wrong invariant for indexing.

---

## 1. Read the source first

`main.cpp` (numbered). The `BP:` comments mark the intended stops.

```cpp
 1  #include <cstddef>
 2  #include <iostream>
 3  #include <iterator>
 4  #include <vector>
 5  
 6  // Type 1: unsigned subtraction below zero wraps to a huge value instead of going negative.
 7  void size_minus_one_on_empty() {
 8  	const std::vector<int> empty;
 9  	const std::size_t last = empty.size() - 1; // BP: 0u - 1 wraps to SIZE_MAX; well defined, still wrong.
10  	std::cout << "empty.size()-1 = " << last << '\n';
11  
12  	if (!empty.empty()) {                       // Guard the container instead of trusting arithmetic.
13  		std::cout << "last element: " << empty[empty.size() - 1] << '\n';
14  	} else {
15  		std::cout << "empty: no last element\n";
16  	}
17  }
18  
19  // Type 2: a reverse loop with an unsigned index never terminates on its own.
20  void reverse_loop_never_ends() {
21  	const std::vector<int> v{10, 20, 30};
22  	// for (std::size_t i = v.size() - 1; i >= 0; --i) {} // BP: i >= 0 is always true for unsigned.
23  
24  	for (std::size_t i = v.size(); i-- > 0;) {  // Post-decrement in the condition stops at zero.
25  		std::cout << "down " << v[i] << '\n';
26  	}
27  	for (auto it = v.rbegin(); it != v.rend(); ++it) { // Reverse iterators avoid indices altogether.
28  		std::cout << "rit  " << *it << '\n';
29  	}
30  }
31  
32  // Type 3: mixing signed and unsigned converts the signed operand, flipping the comparison.
33  void signed_unsigned_comparison() {
34  	const int offset = -1;
35  	const std::size_t count = 3;
36  	std::cout << "(-1 < 3u) = " << std::boolalpha
37  			  << (offset < static_cast<int>(count))          // Correct: compare in the signed domain.
38  			  << "  raw mixed compare would be: " << (static_cast<std::size_t>(offset) < count)
39  			  << '\n'; // BP: -1 converts to SIZE_MAX, so the mixed comparison is false.
40  
41  	const std::vector<int> v{10, 20, 30};
42  	std::cout << "ssize based loop:";
43  	for (std::ptrdiff_t i = std::ssize(v) - 1; i >= 0; --i) { // std::ssize gives a signed size.
44  		std::cout << ' ' << v[static_cast<std::size_t>(i)];
45  	}
46  	std::cout << '\n';
47  }
48  
49  int main() {
50  	size_minus_one_on_empty();
51  	reverse_loop_never_ends();
52  	signed_unsigned_comparison();
53  }
```

| Name | Role | Invariant to check |
|---|---|---|
| empty | vector<int> | size is zero |
| last | size_t | wrap result must not be used as an index |
| i | size_t reverse index | post-decrement condition stops before wrap is used |
| offset/count | signed and unsigned operands | choose one comparison domain explicitly |

---

## 2. Build without a sanitizer

No build, clean, or rebuild was run for this document. The existing Debug executable was used:

```powershell
C:\src\cpptraps42\build\cdb\Debug\Trap41_UnsignedUnderflow.exe
```

No sanitizer applies. Unsigned wrap and signed-to-unsigned conversion are defined C++; MSVC ASan checks memory accesses, not arithmetic intent.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
       -srcpath C:\src\cpptraps42\Memory\\Trap41_UnsignedUnderflow ^
       C:\src\cpptraps42\build\cdb\Debug\Trap41_UnsignedUnderflow.exe
```

Two settings are required before using source lines and local names:

```
.symopt-100
.lines -e
```

---

## 4. Transcript: one defined-behaviour target

Command output below is copied from real CDB runs. Addresses are from one run and are not stable across runs.

### All variants

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp Trap41_UnsignedUnderflow!size_minus_one_on_empty+0x37
0:000> bp Trap41_UnsignedUnderflow!reverse_loop_never_ends+0xc1
0:000> bp Trap41_UnsignedUnderflow!reverse_loop_never_ends+0x117
0:000> bp Trap41_UnsignedUnderflow!signed_unsigned_comparison+0x3e
0:000> g
Breakpoint 0 hit
Trap41_UnsignedUnderflow!size_minus_one_on_empty+0x37:
00007ff7`6aeb1b37 488d154ad10000  lea     rdx,[Trap41_UnsignedUnderflow!__xt_z+0x1e8 (00007ff7`6aebec88)]
0:000> $$ ===== VARIANT 1: after empty.size() - 1 =====
0:000> dv /t /v
00000088`ce13f668 class std::vector<int,std::allocator<int> > empty = { size=0x0 }
00000088`ce13f698 unsigned int64 last = 0xffffffff`ffffffff
0:000> ?? empty._Mypair._Myval2._Mylast - empty._Mypair._Myval2._Myfirst
int64 0n0
0:000> ?? last
unsigned int64 0xffffffff`ffffffff
0:000> ?? ((unsigned int64)0 - 1)
unsigned int64 0xffffffff`ffffffff
0:000> g
Breakpoint 1 hit
Trap41_UnsignedUnderflow!reverse_loop_never_ends+0xc1:
00007ff7`6aeb1cd1 488d4c2428      lea     rcx,[rsp+28h]
0:000> $$ ===== VARIANT 2: before the safe post-decrement reverse loop =====
0:000> dv /t /v
00000088`ce13f5c8 unsigned int64 i = 0xcccccccc`cccccccc
00000088`ce13f588 class std::vector<int,std::allocator<int> > v = { size=0x3 }
0:000> ?? v._Mypair._Myval2._Mylast - v._Mypair._Myval2._Myfirst
int64 0n3
0:000> ?? ((unsigned int64)0 - 1)
unsigned int64 0xffffffff`ffffffff
0:000> ?? (((unsigned int64)0 - 1) >= 0)
bool true
0:000> g
Breakpoint 2 hit
Trap41_UnsignedUnderflow!reverse_loop_never_ends+0x117:
00007ff7`6aeb1d27 488d159acf0000  lea     rdx,[Trap41_UnsignedUnderflow!__xt_z+0x228 (00007ff7`6aebecc8)]
0:000> $$ ===== VARIANT 2: first safe reverse-loop body =====
0:000> dv /t /v
00000088`ce13f5c8 unsigned int64 i = 2
00000088`ce13f588 class std::vector<int,std::allocator<int> > v = { size=0x3 }
0:000> ?? i
unsigned int64 2
0:000> bc 2
0:000> g
Breakpoint 3 hit
Trap41_UnsignedUnderflow!signed_unsigned_comparison+0x3e:
00007ff7`6aeb1ebe 488d1533ce0000  lea     rdx,[Trap41_UnsignedUnderflow!__xt_z+0x258 (00007ff7`6aebecf8)]
0:000> $$ ===== VARIANT 3: mixed signed/unsigned comparison =====
0:000> dv /t /v
00000088`ce13f5d8 unsigned int64 count = 3
00000088`ce13f5d0 int offset = 0n-1
0:000> ?? offset
int 0n-1
0:000> ?? count
unsigned int64 3
0:000> ?? (unsigned int64)offset
unsigned int64 0xffffffff`ffffffff
0:000> ?? ((unsigned int64)offset < count)
bool false
0:000> ?? (offset < (int)count)
bool true
0:000> g
empty.size()-1 = 18446744073709551615
empty: no last element
down 30
down 20
down 10
rit  30
rit  20
rit  10
(-1 < 3u) = true  raw mixed compare would be: false
ssize based loop: 30 20 10
```

---

## 5. Why there is no `_unsafe` target

There is no `_unsafe` executable because all three variants are defined behaviour. `0u - 1` wraps, an unsigned value is always `>= 0`, and converting `-1` to an unsigned 64-bit value yields `0xffffffffffffffff`. Wrong means a range or loop invariant was expressed in the wrong type domain.

---

## 6. What the measurements prove

| Variant | Measurement | Observed | Meaning |
|---|---|---|---|
| empty size()-1 | last | 0xffffffffffffffff | wrap, not corruption |
| reverse loop | 0u-1 >= 0 | true | a naive unsigned loop condition cannot become false |
| safe reverse loop | first body i | 2 | post-decrement starts at last valid index |
| mixed sign | (unsigned)offset | 0xffffffffffffffff | raw mixed comparison is false |

The debugger confirms the arithmetic contract. Guard the container, use `std::ssize` for signed indexing, or compare after an explicit conversion that preserves the intended domain.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| .symopt-100 | resolve unqualified C++ symbols |
| .lines -e | enable source-line information |
| bp | set a source, function, or offset breakpoint |
| g | run to the next breakpoint |
| dv /t /v | show local variables with types |
| ?? expr | evaluate a C++ expression |
| dx | display C++ objects through debugger visualizers |
| dt | display a type or local object's fields |
| bc | clear a loop-body breakpoint after the first hit |

---

## 8. Left to you

1. Break on the second and third safe-loop iterations and record `i` before it reaches zero.
2. Change `count` to zero and rerun the mixed-sign stop. Which comparison remains meaningful?
3. Replace the index loop with reverse iterators only and verify that no unsigned index exists in locals.
4. Add an assertion before using `size()-1`; make it state the container invariant rather than the arithmetic result.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, as state inspection | It shows the state change; it does not know the programmer's invariant. |
| AddressSanitizer | no | MSVC ASan diagnoses memory safety errors, not defined container/API semantics. |
| UBSan | no | There is no undefined operation here to diagnose. |
| TSan | no | No data race is involved. |
| Assertions/code review | yes | The useful check is the intended invariant: size, type, tolerance, or signed domain. |
