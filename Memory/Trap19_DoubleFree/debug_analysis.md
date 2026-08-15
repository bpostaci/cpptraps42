# Trap19_DoubleFree - CDB debug analysis

**Question:** when does a double free begin: at the crash, or when the second owner first appears?

**Short answer:** at the first ownership mistake. CDB is used here to stop before the later failure and list every pointer that believes it owns the same allocation.

---

## 1. Read the source first

`main.cpp` (51 lines). Three variants run in sequence.

```cpp
 5  void explicit_double_delete() {
 7      int* p = new int(7); delete p; // BP: first release.
 8      delete p; // BP: second release may corrupt allocator metadata.
17  void duplicated_ownership() {
19      int* raw = new int(7);
20      std::unique_ptr<int> first(raw);
21      std::unique_ptr<int> second(raw); // BP: two owners for one allocation.
30  struct Buffer {
31      int* data;
32      Buffer() : data(new int(7)) {}
33      ~Buffer() { delete data; }
35      Buffer(const Buffer& other) : data(new int(*other.data)) {} // safe build only
40  void shallow_copy_double_free() {
41      Buffer a;
42      Buffer b = a; // BP: unsafe build copies the pointer; both destructors delete it.
43      std::cout << "copy: " << *b.data << '\n';
```

| Variant | Who owns the allocation in the unsafe case | First bad state |
|---|---|---|
| explicit double delete | one raw pointer used twice | after the first `delete`, `p` still stores the freed address |
| duplicated ownership | `first` and `second` | both `unique_ptr` objects store the same address |
| shallow copy | `a.data` and `b.data` | compiler-generated copy duplicates the pointer |

---

## 2. Build without a sanitizer

```powershell
cmake -S . -B build\cdb -DTRAPS_BUILD_UNSAFE=ON -DTRAPS_SANITIZER=none
cmake --build build\cdb --config Debug --target Trap19_DoubleFree Trap19_DoubleFree_unsafe
```

ASan is a good next run for a diagnostic report. This debugger pass is different: it records the first release and the duplicate owner state before the allocator reports damage.

---

## 3. Start CDB

```powershell
cdb -o -y C:\src\cpptraps42\build\cdb\Debug ^
       -srcpath C:\src\cpptraps42\Memory\Trap19_DoubleFree ^
       C:\src\cpptraps42\build\cdb\Debug\Trap19_DoubleFree.exe
```

Run `.symopt-100` and `.lines -e` before setting source-line breakpoints.

---

## 4. The safe target

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:11`
0:000> bp `main.cpp:12`
0:000> bp `main.cpp:25`
0:000> bp `main.cpp:43`
0:000> g
Breakpoint 0 hit
Trap19_DoubleFree!explicit_double_delete+0x2f:
00007ff6`518b173f 33d2            xor     edx,edx
0:000> $$ ===== explicit_double_delete safe: one unique_ptr owns the int =====
0:000> dv /t /v
00000003`57fff6f8 class std::unique_ptr<int,std::default_delete<int> > p = unique_ptr 7
0:000> dqs @@c++(&p) L1
00000003`57fff6f8  00000190`73517100
0:000> ?? *(int*)@@masm(poi(@@c++(&p)))
int 0n7
0:000> g
Breakpoint 1 hit
Trap19_DoubleFree!explicit_double_delete+0x3b:
00007ff6`518b174b 488d155ea40000  lea     rdx,[Trap19_DoubleFree!__xt_z+0x110 (00007ff6`518bbbb0)]
0:000> $$ ===== explicit_double_delete safe: after reset the owner is empty =====
0:000> dv /t /v
00000003`57fff6f8 class std::unique_ptr<int,std::default_delete<int> > p = empty
0:000> dqs @@c++(&p) L1
00000003`57fff6f8  00000000`00000000
0:000> g
Breakpoint 2 hit
Trap19_DoubleFree!duplicated_ownership+0x3f:
00007ff6`518b17cf 488d15faa30000  lea     rdx,[Trap19_DoubleFree!__xt_z+0x130 (00007ff6`518bbbd0)]
0:000> $$ ===== duplicated_ownership safe: ownership was moved, not copied =====
0:000> dv /t /v
00000003`57fff6e8 class std::unique_ptr<int,std::default_delete<int> > second = unique_ptr 7
00000003`57fff6c8 class std::unique_ptr<int,std::default_delete<int> > first = empty
0:000> dqs @@c++(&first) L1
00000003`57fff6c8  00000000`00000000
0:000> dqs @@c++(&second) L1
00000003`57fff6e8  00000190`73517100
0:000> ?? *(int*)@@masm(poi(@@c++(&second)))
int 0n7
0:000> g
Breakpoint 3 hit
Trap19_DoubleFree!shallow_copy_double_free+0x32:
00007ff6`518b1882 488d15d7a30000  lea     rdx,[Trap19_DoubleFree!__xt_z+0x1c0 (00007ff6`518bbc60)]
0:000> $$ ===== shallow_copy_double_free safe: deep copy gives two buffers =====
0:000> dv /t /v
00000003`57fff6e8 struct Buffer b = struct Buffer
00000003`57fff6c8 struct Buffer a = struct Buffer
0:000> ?? a.data
int * 0x00000190`73517100
0:000> ?? b.data
int * 0x00000190`7350f980
0:000> ?? *a.data
int 0n7
0:000> ?? *b.data
int 0n7
0:000> g
explicit: single release
ownership: 7
copy: 7
ntdll!NtTerminateProcess+0x14:
00007ffc`a0500904 c3              ret
```

The safe target has one owner at a time: an empty `unique_ptr` after reset, a moved-from `first`, and two different `Buffer::data` addresses after the deep copy.

---

## 5. The unsafe target

Later variants were captured in separate sessions and skipped earlier crashing variants by setting `@rip` at `main`.

### Variant 1: same raw pointer released twice

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:8`
0:000> g
Breakpoint 0 hit
Trap19_DoubleFree_unsafe!explicit_double_delete+0x61:
00007ff7`76411701 488b442420      mov     rax,qword ptr [rsp+20h] ss:0000005d`51b9f710=000001f39a962090
0:000> $$ ===== explicit_double_delete unsafe: after first delete, before second =====
0:000> dv /t /v
0000005d`51b9f710 int * p = 0x000001f3`9a962090
0:000> .printf "p = %p\n", @@c++(p)
p = 000001f39a962090
0:000> ?? *p
int 0n-17891602
0:000> db @@c++(p) L10
000001f3`9a962090  ee fe ee fe ee fe ee fe-ee fe ee fe ee fe ee fe  ................
```

`0xFEEEFEEE` is the Debug CRT/Win32 freed-memory pattern, not a language guarantee. The important fact is that `p` still holds a number after its ownership was spent.

### Variant 2: two `unique_ptr` owners

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:47`
0:000> bp `main.cpp:26`
0:000> g
Breakpoint 0 hit
Trap19_DoubleFree_unsafe!main+0x6:
00007ff7`76411886 e82cfaffff      call    Trap19_DoubleFree_unsafe!ILT+690(?explicit_double_deleteYAXXZ) (00007ff7`764112b7)
0:000> $$ ===== skip explicit_double_delete in this capture =====
0:000> r @rip = Trap19_DoubleFree_unsafe!main+0xb
0:000> g
Breakpoint 1 hit
Trap19_DoubleFree_unsafe!duplicated_ownership+0x79:
00007ff7`764117a9 488d4c2458      lea     rcx,[rsp+58h]
0:000> $$ ===== duplicated_ownership unsafe: two unique_ptr objects own one address =====
0:000> dv /t /v
000000d4`becff760 int * raw = 0x00000252`2f4b0c70
000000d4`becff798 class std::unique_ptr<int,std::default_delete<int> > second = unique_ptr 7
000000d4`becff778 class std::unique_ptr<int,std::default_delete<int> > first = unique_ptr 7
0:000> ?? raw
int * 0x00000252`2f4b0c70
0:000> dqs @@c++(&first) L1
000000d4`becff778  00000252`2f4b0c70
0:000> dqs @@c++(&second) L1
000000d4`becff798  00000252`2f4b0c70
0:000> ?? *(int*)@@masm(poi(@@c++(&first)))
int 0n7
```

The two `unique_ptr` objects are not sharing; they are both exclusive owners of the same address. That is the bug before any destructor runs.

### Variant 3: shallow copy of an owner

```
0:000> .symopt-100
0:000> .lines -e
0:000> bp `main.cpp:47`
0:000> bp `main.cpp:43`
0:000> g
Breakpoint 0 hit
Trap19_DoubleFree_unsafe!main+0x6:
00007ff7`76411886 e82cfaffff      call    Trap19_DoubleFree_unsafe!ILT+690(?explicit_double_deleteYAXXZ) (00007ff7`764112b7)
0:000> $$ ===== skip earlier variants in this capture =====
0:000> r @rip = Trap19_DoubleFree_unsafe!main+0x10
0:000> g
Breakpoint 1 hit
Trap19_DoubleFree_unsafe!shallow_copy_double_free+0x2c:
00007ff7`7641180c 488d157da40000  lea     rdx,[Trap19_DoubleFree_unsafe!__xt_z+0x1f0 (00007ff7`7641bc90)]
0:000> $$ ===== shallow_copy_double_free unsafe: implicit copy duplicated the owner =====
0:000> dv /t /v
000000b5`550ff768 struct Buffer b = struct Buffer
000000b5`550ff748 struct Buffer a = struct Buffer
0:000> ?? a.data
int * 0x0000029e`91219930
0:000> ?? b.data
int * 0x0000029e`91219930
0:000> ?? *a.data
int 0n7
0:000> ?? *b.data
int 0n7
```

---

## 6. What the measurements prove

| Variant | Safe measurement | Unsafe measurement |
|---|---|---|
| explicit release | owner becomes null after `reset()` | `p` still names freed memory filled with `ee fe ee fe` |
| duplicated ownership | `first` is null, `second` owns | `first` and `second` contain the same address |
| shallow copy | `a.data` and `b.data` differ | `a.data` and `b.data` are identical |

Double free is an ownership error, not merely an allocator error. The allocator may detect the second release, but the debugger can show the bad state earlier.

---

## 7. Command reference used here

| Command | Purpose |
|---|---|
| `.symopt-100`, `.lines -e` | make local names and line breakpoints usable |
| `` bp `main.cpp:8` `` | stop after the first raw `delete` and before the second |
| `r @rip = module!main+offset` | reach later variants without executing earlier unsafe code |
| `dv /t /v` | list locals with types |
| `dqs @@c++(&first) L1` | dump the raw pointer stored inside a `unique_ptr` |
| `?? a.data` | evaluate a raw owner member |
| `db p L10` | show Debug CRT freed-memory fill |

---

## 8. Left to you

1. Let the unsafe explicit variant continue. Where does the Debug CRT stop, and how does the message differ from the earlier byte evidence?
2. Put a hardware breakpoint on the address in `first`. Which destructor reads it first?
3. Replace the raw `Buffer::data` with `std::unique_ptr<int>`. Which special member becomes deleted, and how does compilation prevent the trap?
4. Run the ASan unsafe target. Which variant is reported first, and why might that hide later variants?
5. In the safe deep-copy variant, add an assignment operation. Does the existing assignment policy still prevent duplicated ownership?

---

## 9. Tool limits

| Tool | Reports this trap | Limitation |
|---|---|---|
| CDB | yes, shows duplicate owner addresses | cannot prove all future paths are safe |
| AddressSanitizer | yes, for many double frees | stops at first failure and changes allocator behavior |
| Debug CRT fills | yes, when memory is freed | Debug-only courtesy; Release may not show it |
| Compiler warnings | partial | raw ownership protocols are often invisible to the compiler |
