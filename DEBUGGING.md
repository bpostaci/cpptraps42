# Visual Studio Debugging Map

Every project has one `main.cpp`. Search that file for `BP:` and set a breakpoint on each marked line. Start with safe mode; enable `RUN_UNSAFE_EXAMPLE` only for the project currently being studied.

## Memory

- `Trap01_UseAfterFree`: stop before `owner.reset()`, record `owner.get()` and `observer`, step over reset, then observe that the address remains but ownership/lifetime is gone. Enable AddressSanitizer before the unsafe dereference.
- `Trap03_UninitializedMemory`: break on the read and inspect the uninitialized local. Compiler warning C4700 is primary evidence on MSVC; debug fill bytes are not valid values.
- `Trap04_OutOfBounds`: compare `index`, `size()`, `data()`, and `data()+index`. One-past may be formed but never dereferenced.
- `Trap07_IteratorInvalidation`: record `data()` and `capacity()` before and after `push_back`. A changed `data()` proves relocation.
- `Trap16_MemsetObject`: inspect the string's logical fields before memset. Do not continue without ASan; the raw write destroys invariants.
- `Trap17_NewDeleteMismatch` and `Trap19_DoubleFree`: use AddressSanitizer and study both allocation and deallocation stacks.

## Lifetime and object model

- For `string_view`, lambda capture, and temporary lifetime, record the owner/captured local address before scope exit and compare it at the later use.
- In `Trap10_VirtualInConstructor`, break in both `speak()` implementations. Step through `Base::Base`; dispatch to Base is defined behavior.
- In `Trap29_RawBytes`, inspect alignment and raw bytes before `construct_at`; then step over construction and expand the live `Record`. After `destroy_at`, bytes remain but lifetime has ended.
- In `Trap14_Alignment`, inspect `(uintptr_t)p % alignof(uint64_t)`. UBSan/alignment sanitizer is more useful than relying on x64 hardware behavior.

## Concurrency

- Use ThreadSanitizer for `Trap20_DataRace`; normal breakpoints serialize threads and can hide the schedule.
- In `Trap22_VolatileIsNotSync`, break on release and acquire. The payload is non-atomic but safe because the successful acquire synchronizes with release. Replacing the atomic with volatile removes that relation.

## ABI and build

- At `Trap28_ABIMismatch`, compare `sizeof`, `alignof`, and offsets in every participating module. Also compare architecture, runtime library, packing, calling convention, ownership, and byte order.
- For ODR bugs, compare preprocessed output and compiler definitions across translation units. A linker success does not prove definitions agree.

## Sanitizer choice

| Symptom | First tool |
|---|---|
| use-after-free, bounds, double free | AddressSanitizer |
| signed overflow, misalignment | UndefinedBehaviorSanitizer (Clang) |
| data race | ThreadSanitizer (Clang/Linux or WSL) |
| uninitialized value | compiler warnings / MemorySanitizer where supported |

