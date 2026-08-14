# Trap Reference

Every trap directory represents one canonical book topic; selected directories contain several named variants of that topic, and advanced build traps may contain multiple translation units. Comments explain the questionable operation beside the relevant statement and identify the corrective pattern. When a source contains a guarded `RUN_UNSAFE_EXAMPLE` branch, CMake creates a separately named `_unsafe` target. Traps without such a branch have only their normal target. `_unsafe` is a teaching label: a branch can be undefined behavior, a lifetime violation, an unspecified-but-valid state, or defined yet dangerous logic.

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

## Expanded lab coverage

Targets 03, 04, 05, 06, 07, 08, 09, 12, 17, 19, 21, and 23 each contain three short variant functions called from `main()`. This preserves the searchable 30-trap structure while allowing precise breakpoints on common forms of the same bug. The PDF explains the canonical rule; the repository supplies additional practice variants, so variant functions do not require separate PDF pages.
