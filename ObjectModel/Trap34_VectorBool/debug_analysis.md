# Trap34_VectorBool - CDB debug analysis

**Question:** Does `auto b = bits[0]` make a detached bool copy from `vector<bool>`?

**Short answer:** No. `vector<bool>` is a packed specialization. `operator[]` returns a proxy object, and `auto` preserves that proxy.

---

## 1. Read the source first

`main.cpp` (numbered). The `BP:` comments mark the intended stops.

```cpp
 1  #include <array>
 2  #include <bitset>
 3  #include <iostream>
 4  #include <vector>
 5  
 6  // Type 1: vector<bool> is a bit-packed specialisation, so operator[] returns a proxy, not bool&.
 7  void proxy_instead_of_reference() {
 8  	std::vector<bool> bits{true, false, true};
 9  	// bool& ref = bits[0];      // Would not compile: operator[] yields std::vector<bool>::reference.
10  	auto proxy = bits[0];        // BP: 'auto' deduces the proxy, which still aliases the container.
11  	proxy = false;               // BP: this writes into 'bits', although it looks like a local copy.
12  	std::cout << "proxy write changed container: " << std::boolalpha << bits[0] << '\n';
13  
14  	bool copy = bits[2];         // Naming the type explicitly forces a real, detached copy.
15  	copy = false;
16  	std::cout << "explicit bool copy is detached: " << bits[2] << '\n';
17  }
18  
19  // Type 2: there is no contiguous bool array underneath, so data() and pointers are unavailable.
20  void no_contiguous_storage() {
21  	std::vector<bool> bits(8, true);
22  	// const bool* raw = bits.data();     // Would not compile: no data() returning bool*.
23  	// for (bool& b : bits) { b = false; } // Would not compile: cannot bind bool& to the proxy.
24  	for (auto&& b : bits) { b = false; }   // Binding to the proxy by forwarding reference works.
25  	std::cout << "packed size=" << bits.size() << " front=" << bits.front() << '\n';
26  
27  	std::vector<char> flags(8, 1);          // A real contiguous buffer when interop is needed.
28  	std::cout << "char buffer is contiguous: " << static_cast<const void*>(flags.data()) << '\n';
29  }
30  
31  // Type 3: pick the container that matches the intent instead of fighting the specialisation.
32  void better_alternatives() {
33  	std::bitset<8> fixed_flags;             // Fixed-size flag set with bit operations.
34  	fixed_flags.set(1);
35  	std::cout << "bitset=" << fixed_flags << " count=" << fixed_flags.count() << '\n';
36  
37  	std::array<bool, 3> small{true, false, true}; // Real bools, real references, fixed size.
38  	for (bool& b : small) { b = !b; }
39  	std::cout << "array front=" << small.front() << '\n';
40  
41  	std::vector<char> dynamic_flags{1, 0, 1};     // Addressable byte elements with dynamic size.
42  	dynamic_flags.push_back(0);
43  	std::cout << "vector<char> size=" << dynamic_flags.size() << '\n';
44  }
45  
46  int main() {
47  	proxy_instead_of_reference();
48  	no_contiguous_storage();
49  	better_alternatives();
50  }
```

| Name | Role | Invariant to check |
|---|---|---|
| bits | vector<bool> | packed bits, not contiguous bool objects |
| proxy | std::_Vb_reference | assignment writes through to the container |
| flags | vector<char> | byte elements have contiguous storage |
| small | array<bool,3> | real bool elements and bool* addresses |

---

## 2. Build without a sanitizer

No build, clean, or rebuild was run for this document. The existing Debug executable was used:

```powershell
C:\src\cpptraps42\build\cdb\Debug\Trap34_VectorBool.exe
```

A sanitizer would add nothing. `vector<bool>` proxy references and deleted/absent `data()` access are defined library design choices, not invalid memory accesses.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
       -srcpath C:\src\cpptraps42\ObjectModel\\Trap34_VectorBool ^
       C:\src\cpptraps42\build\cdb\Debug\Trap34_VectorBool.exe
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
0:000> bp Trap34_VectorBool!proxy_instead_of_reference+0xb9
0:000> bp Trap34_VectorBool!proxy_instead_of_reference+0xc6
0:000> bp Trap34_VectorBool!no_contiguous_storage+0x57
0:000> bp Trap34_VectorBool!no_contiguous_storage+0x20a
0:000> bp Trap34_VectorBool!better_alternatives+0xaf
0:000> g
Breakpoint 0 hit
Trap34_VectorBool!proxy_instead_of_reference+0xb9:
00007ff6`c05e38e9 33d2            xor     edx,edx
0:000> $$ ===== VARIANT 1: auto deduced the proxy =====
0:000> dv /t /v
00000071`f7affc34 bool copy = true
00000071`f7affbb8 class std::vector<bool,std::allocator<bool> > bits = { size=0x3 }
00000071`f7affc08 class std::_Vb_reference<std::_Wrap_alloc<std::allocator<unsigned int> > > proxy = true
0:000> ?? proxy
class std::_Vb_reference<std::_Wrap_alloc<std::allocator<unsigned int> > >
   +0x000 _Myproxy         : 0x00000201`4aafd440 std::_Container_proxy
   +0x008 _Mynextiter      : (null) 
   +0x010 _Myptr           : 0x00000201`4aafa790  -> 5
   +0x018 _Myoff           : 0
0:000> dt proxy
Local var @ 0x71f7affc08 Type std::_Vb_reference<std::_Wrap_alloc<std::allocator<unsigned int> > >
   +0x010 _Myptr           : 0x00000201`4aafa790  -> 5
   +0x018 _Myoff           : 0
0:000> g
Breakpoint 1 hit
Trap34_VectorBool!proxy_instead_of_reference+0xc6:
00007ff6`c05e38f6 488d15db650100  lea     rdx,[Trap34_VectorBool!__xt_z+0x328 (00007ff6`c05f9ed8)]
0:000> $$ ===== VARIANT 1: after assigning through the proxy =====
0:000> dv /t /v
00000071`f7affc34 bool copy = true
00000071`f7affbb8 class std::vector<bool,std::allocator<bool> > bits = { size=0x3 }
00000071`f7affc08 class std::_Vb_reference<std::_Wrap_alloc<std::allocator<unsigned int> > > proxy = false
0:000> ?? proxy
class std::_Vb_reference<std::_Wrap_alloc<std::allocator<unsigned int> > >
   +0x010 _Myptr           : 0x00000201`4aafa790  -> 4
   +0x018 _Myoff           : 0
0:000> g
Breakpoint 2 hit
Trap34_VectorBool!no_contiguous_storage+0x57:
00007ff6`c05e3b07 488d442428      lea     rax,[rsp+28h]
0:000> $$ ===== VARIANT 2: vector<bool> has no bool data() =====
0:000> dv /t /v
00000071`f7affb88 class std::vector<bool,std::allocator<bool> > bits = { size=0x8 }
0:000> ?? &bits[0]
Type is not an array or pointer for operator[] '[0]'
0:000> ?? bits.data()
Type does not have given member error at 'data()'
0:000> g
Breakpoint 3 hit
Trap34_VectorBool!no_contiguous_storage+0x20a:
00007ff6`c05e3cba 488d1577620100  lea     rdx,[Trap34_VectorBool!__xt_z+0x388 (00007ff6`c05f9f38)]
0:000> $$ ===== VARIANT 2: vector<char> does have contiguous storage =====
0:000> dv /t /v
00000071`f7affc98 class std::vector<char,std::allocator<char> > flags = "\x01\x01\x01\x01\x01\x01\x01\x01"
0:000> ?? flags._Mypair._Myval2._Myfirst
char * 0x00000201`4aaf9b80
 "???"
0:000> ?? flags._Mypair._Myval2._Mylast - flags._Mypair._Myval2._Myfirst
int64 0n8
0:000> g
Breakpoint 4 hit
Trap34_VectorBool!better_alternatives+0xaf:
00007ff6`c05e3dff 488d442444      lea     rax,[rsp+44h]
0:000> $$ ===== VARIANT 3: array<bool> is real bool storage =====
0:000> dv /t /v
00000071`f7affc34 class std::array<bool,3> small = { size=3 }
00000071`f7affc14 class std::bitset<8> fixed_flags = { size=8 }
0:000> ?? small._Elems[0]
bool true
0:000> ?? &small._Elems[0]
bool * 0x00000071`f7affc34
0:000> ?? &small._Elems[1]
bool * 0x00000071`f7affc35
0:000> g
proxy write changed container: false
explicit bool copy is detached: true
packed size=8 front=false
char buffer is contiguous: 000002014AAF9B80
bitset=00000010 count=1
array front=false
vector<char> size=4
```

---

## 5. Why there is no `_unsafe` target

There is no `_unsafe` executable because all behaviour is defined. The specialization is allowed to return a proxy and to pack bits. Wrong means the programmer expected `auto` to detach a bool or expected `vector<bool>` to expose `bool*` storage. The debugger's type display is the evidence.

---

## 6. What the measurements prove

| Variant | Measurement | Observed | Meaning |
|---|---|---|---|
| proxy | type of `proxy` | std::_Vb_reference | not `bool` |
| proxy assignment | underlying word | 5 becomes 4 | assignment changed bit 0 in the container |
| storage | `&bits[0]` / `bits.data()` | debugger errors | no contiguous bool storage API |
| alternatives | array addresses | adjacent bool* addresses | array<bool> stores real bool elements |

The program is correct C++ but surprising. Choose `bitset` for fixed bit sets, `array<bool>` for small real bools, or `vector<char>` when addressable dynamic storage is part of the contract.

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

---

## 8. Left to you

1. Change `auto proxy` to `bool proxy` and rerun the first two stops. Which type and write-through evidence disappear?
2. Break inside the `for (auto&& b : bits)` loop and inspect the loop variable type.
3. Try to add `const bool* raw = bits.data()` in source. Confirm that the failure is compile-time, not runtime.
4. Compare `vector<char>` and `array<bool>` addresses for adjacent elements and explain which contracts each container can support.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, as state inspection | It shows the state change; it does not know the programmer's invariant. |
| AddressSanitizer | no | MSVC ASan diagnoses memory safety errors, not defined container/API semantics. |
| UBSan | no | There is no undefined operation here to diagnose. |
| TSan | no | No data race is involved. |
| Assertions/code review | yes | The useful check is the intended invariant: size, type, tolerance, or signed domain. |
