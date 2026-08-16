# Trap Reference

Every trap directory represents one canonical book topic; selected directories contain several named variants of that topic, and advanced build traps may contain multiple translation units. Comments explain the questionable operation beside the relevant statement and identify the corrective pattern. When a source contains a guarded `RUN_UNSAFE_EXAMPLE` branch, CMake creates a separately named `_unsafe` target. Traps without such a branch have only their normal target. `_unsafe` is a teaching label: a branch can be undefined behavior, a lifetime violation, an unspecified-but-valid state, or defined yet dangerous logic.

This page is the index. Each row summarises one trap in a single line; the `README.md` inside that trap's folder expands the same trap into the full rule, the corrective pattern, and the tools that do and do not detect it.
| Trap | Category | Typical symptom | Why it fails | Detection | Correct direction |
|---:|---|---|---|---|---|
| 01 | Memory | delayed crash/corruption | access after dynamic lifetime | ASan | RAII ownership |
| 02 | Lifetime | plausible stale pointer | non-null proves no lifetime fact | ASan + ownership trace | weak/shared snapshot |
| 03 | Memory | Debug/Release difference | indeterminate read | warnings/MSan | initialize at declaration |
| 04 | Memory | adjacent corruption | dereference outside array | ASan | bounds check/span/at |
| 05 | Memory | optimized logic vanishes | signed overflow is UB | UBSan | pre-check/checked math |
| 06 | Lifetime | stale stack data | scope ended object lifetime | ASan | keep use inside owner scope |
| 07 | Memory | iterator fails after mutation | reallocation invalidates handles | ASan/debug iterators | reacquire/reserve with care |
| 08 | Lifetime | corrupt view text | view does not own characters | ASan | retain owner/return string |
| 09 | Lifetime | callback reads old stack | reference capture outlives referent | ASan | explicit value/owner capture |
| 10 | ObjectModel | base override selected | derived part not active | debugger call stack | avoid virtual initialization |
| 11 | ObjectModel | derived cleanup skipped | deletion contract not polymorphic | warnings/leak tools | virtual or protected destructor |
| 12 | ObjectModel | derived behavior disappears | copying creates standalone Base | debugger dynamic type | reference/pointer polymorphism |
| 13 | ObjectModel | optimized-only wrong value | illegal unrelated typed access | optimizer comparison | bit_cast/memcpy |
| 14 | ObjectModel | architecture-specific fault | address violates alignof(T) | UBSan | aligned storage/allocation |
| 15 | ObjectModel | numeric address looks right | provenance/lifetime lost | ownership history | retain original valid pointer |
| 16 | Memory | class invariant corruption | byte writes are not class operations | ASan | constructors/assignment |
| 17 | Memory | allocator/destructor failure | allocation/deallocation forms differ | ASan | container/unique_ptr<T[]> |
| 18 | ABI_Build | foreign-heap corruption | incompatible allocator contract | heap diagnostics | matching destroy API |
| 19 | Memory | allocator failure later | resource released twice | ASan | unique ownership |
| 20 | Concurrency | nondeterminism | conflicting unsynchronized access | TSan | atomic/mutex |
| 21 | Concurrency | object dies after check | invariant not protected as a unit | TSan + schedule trace | locked ownership snapshot |
| 22 | Concurrency | stale payload | volatile creates no synchronization | TSan | release/acquire or mutex |
| 23 | Lifetime | reused value changes by type | moved-from state unspecified | contract review | reset before semantic reuse |
| 24 | Lifetime | dangling c_str | temporary dies at full-expression | ASan | keep owner alive |
| 25 | ABI_Build | leaked lock/resource | exception bypasses manual cleanup | exception breakpoints | RAII |
| 26 | ABI_Build | startup depends on link order | cross-TU dynamic init order | constructor breakpoints | construct on first use/constinit |
| 27 | ABI_Build | modules disagree on type | inconsistent definitions violate ODR | preprocessed/layout comparison | canonical header/config |
| 28 | ABI_Build | shifted fields/calls | binary contracts disagree | layout/symbol inspection | opaque/versioned ABI |
| 29 | ObjectModel | raw bytes treated as class | storage is not necessarily a live object | lifetime trace | construct_at/destroy_at |
| 30 | ObjectModel | expression varies by compiler | unsequenced scalar modifications | warnings/UBSan | separate sequenced statements |
| 31 | Memory | double free on scope exit | implicit copy duplicates a raw owner | ASan | Rule of Three/Zero |
| 32 | Lifetime | destructor never runs | owning cycle keeps use_count above zero | leak tools/use_count | `weak_ptr` back edge |
| 33 | Memory | container grows while reading | `operator[]` is a mutating lookup | size assertions | `find`/`at`/`contains` |
| 34 | ObjectModel | `bool&` will not bind | bit-packed specialization returns a proxy | compile errors | `bitset`/`array`/`vector<char>` |
| 35 | Lifetime | writes hit a hidden copy | `auto` drops reference and const | debugger type inspection | `auto&`/`const auto&`/`decltype(auto)` |
| 36 | ObjectModel | base overload not found | derived name hides the base set | overload resolution trace | `using Base::f` |
| 37 | ObjectModel | wrong default through base | default argument binds statically | static vs dynamic type check | non-virtual interface |
| 38 | ObjectModel | member access will not compile | declaration parsed instead of definition | C4930/-Wvexing-parse | brace initialization |
| 39 | ObjectModel | member built from garbage | declaration order beats init-list order | warnings/MSan | order declarations or assign in body |
| 40 | Memory | equality test never true | decimals are not representable in binary | value inspection | scaled tolerance/`isnan` |
| 41 | Memory | huge index or endless loop | unsigned subtraction wraps | warnings/ASan | guard emptiness/`ssize`/reverse iterators |
| 42 | Concurrency | abrupt `std::terminate` | joinable thread destroyed | exception breakpoints | `jthread`/RAII join |

## Expanded lab coverage

Targets 03, 04, 05, 06, 07, 08, 09, 12, 17, 19, 21, 23, and every target from 31 to 42 each contain three short variant functions called from `main()`. This preserves the searchable per-trap structure while allowing precise breakpoints on common forms of the same bug. The PDF explains the canonical rule; the repository supplies additional practice variants, so variant functions do not require separate PDF pages.

## Set boundaries

Traps 01-30 are the original canonical list, weighted towards undefined behavior, object model, and build/ABI contracts. Traps 31-42 cover the standard-library and class-design mistakes that appear most often in day-to-day code review. Several of them are not undefined behavior at all: 33, 34, 35, 36, 37, 38, 40, and 41 are fully defined yet routinely produce wrong programs, so sanitizer silence is the expected outcome and the debugger plus the type/contract review is the correct tool.
