# Trap24_TemporaryLifetime - CDB debug analysis

**Question:** if `c_str()` returns an address, can that address outlive the temporary `std::string`?

**Short answer:** no. The pointer is only a borrow. In the unsafe target the full-expression ends at the semicolon on line 5, so line 6 reads through a dangling pointer. That is undefined behavior.

---

## 1. Read the source first

```cpp
 1  #include <iostream>
 2  #include <string>
 3  int main() {
 4  #if defined(RUN_UNSAFE_EXAMPLE)
 5      const char* p=std::string("hello").c_str(); // BP: owner dies at semicolon.
 6      std::cout << p << '\n'; // BP: dangling borrowed pointer.
 7  #else
 8      std::string owner="hello"; std::cout << owner.c_str() << '\n';
 9  #endif
10  }
```

| Name | Holds | Owns characters | Lifetime |
|---|---|---|---|
| `owner` | `std::string` | yes | until end of scope |
| `p` | `const char*` | no | valid only while the temporary string lives |

A `const std::string&` bound directly to `std::string("hello")` would extend that temporary's lifetime. A pointer returned by `c_str()` does not.

---

## 2. Build without a sanitizer

I used the existing binaries and did not build, clean, or rebuild:

```powershell
C:\src\cpptraps42\build\cdb\Debug\Trap24_TemporaryLifetime.exe
C:\src\cpptraps42\build\cdb\Debug\Trap24_TemporaryLifetime_unsafe.exe
```

MSVC AddressSanitizer may catch some later uses of invalid storage, but the debugger lesson here is the exact lifetime boundary: the temporary owner is gone before line 6 begins.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
    -srcpath C:\src\cpptraps42\Lifetime\Trap24_TemporaryLifetime ^
    C:\src\cpptraps42\build\cdb\Debug\Trap24_TemporaryLifetime.exe
```

```
.symopt-100     resolve local names such as owner and p
.lines -e       enable source line breakpoints
```

---

## 4. The safe target

Stop at line 9, after line 8 has executed but before the named owner leaves scope.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:9`
0:000> g
Breakpoint 0 hit
Trap24_TemporaryLifetime!main+0x70:
00007ff7`bbba1990 488d4c2428      lea     rcx,[rsp+28h]
0:000> $$ ===== safe target: named owner still alive before leaving scope =====
0:000> dv /t /v
000000b6`d3cff9d8 class std::basic_string<char,std::char_traits<char>,std::allocator<char> > owner = "hello"
0:000> ?? owner._Mypair._Myval2._Bx._Buf
char [16] 0x000000b6`d3cff9e0
104 'h'
0:000> ?? owner._Mypair._Myval2._Mysize
unsigned int64 5
```

---

## 5. The unsafe target

Stop at line 6. The pointer still contains the address borrowed from the temporary's small-string buffer, but the temporary object has already been destroyed.

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:6`
0:000> g
Breakpoint 0 hit
Trap24_TemporaryLifetime_unsafe!main+0x45:
00007ff6`0fb71965 488b542420      mov     rdx,qword ptr [rsp+20h] ss:00000002`1a8ffc70=000000021a8ffc80
0:000> $$ ===== unsafe target after full expression ended =====
0:000> dv /t /v
00000002`1a8ffc70 char * p = 0x00000002`1a8ffc80 ""
0:000> .printf "p = %p\n", @@c++(p)
p = 000000021a8ffc80
0:000> db @@c++(p) L10
00000002`1a8ffc80  00 65 6c 6c 6f 00 00 00-00 00 00 00 00 00 00 00  .ello...........
0:000> da @@c++(p)
00000002`1a8ffc80  ""
0:000> g

ntdll!NtTerminateProcess+0x14:
00007ffc`a0500904 c3              ret
```

---

## 6. What the measurements prove

| Measurement | Safe named owner | Unsafe temporary |
|---|---|---|
| owner object | `owner = "hello"` exists at line 9 | no `std::string` owner remains at line 6 |
| character address | buffer begins at `...f9e0` in a live object | `p = ...fc80`, a stale borrowed address |
| bytes read | first byte is `104 'h'` | bytes start `00 65 6c 6c 6f`, displayed as empty string |
| language category | defined | undefined behavior |

The divergence is between ownership and address. The stale address still points somewhere, but no live string owns the characters for `p` to borrow.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `` bp `main.cpp:6` `` | stop after the temporary's full-expression ended |
| `dv /t /v` | show the local pointer or string object |
| `?? owner._Mypair...` | inspect MSVC string storage in this Debug build |
| `.printf "%p"` | record the borrowed pointer value |
| `db addr L10` | dump bytes at the borrowed address |
| `da addr` | display the same address as a C string |

---

## 8. Left to you

1. Rewrite the unsafe line as `const std::string& r = std::string("hello"); const char* p = r.c_str();` and measure the lifetime extension.
2. Change `"hello"` to a long string that allocates on the heap. How do the bytes after destruction differ?
3. Build with AddressSanitizer and compare its report with the CDB byte dump.
4. Split the safe line 8 into two lines and break between construction and printing. What is easier to inspect?
5. Test Release and note which locals CDB can no longer display.

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, by showing a stale borrow | bytes after destruction are implementation detail |
| MSVC AddressSanitizer | may report use-after-scope/use-after-free | not needed to prove the lifetime rule |
| Compiler warnings | not reliably | the borrow crosses a full-expression, not a type error |
| Debug CRT fills | not decisive here | small-string storage is inside the destroyed stack object |
