# Self-Test and Debugging Exercises

Answer these before opening the key. Several questions deliberately distinguish undefined behavior from defined-but-dangerous behavior.

## Questions

1. Why can a non-null pointer still be invalid?
2. What is the difference between default-initializing `new Point` and value-initializing `new Point{}` when `Point` has no user-provided constructor?
3. May a one-past pointer be formed, and may it be dereferenced?
4. Why does passing an array as `T*` make bounds checking harder?
5. Why is checking for signed overflow after evaluating `a + b` unreliable?
6. For `int index = -1`, why is `index < vector.size()` normally false rather than true?
7. Is converting `300` to an 8-bit `unsigned char` undefined behavior?
8. Which vector event invalidates all iterators, references, and pointers to its elements?
9. Does `erase` invalidate the same positions for every standard container?
10. What does `std::string_view` own?
11. Name three events that can dangle a `string_view`.
12. Why can `[&local]` and `[this]` both create lifetime bugs in deferred callbacks?
13. Is object slicing undefined behavior? What information is lost?
14. Why must `new[]` pair with `delete[]`, and placement `new` pair with explicit destruction?
15. How does a shallow copy of a raw owning pointer lead to double deletion?
16. After moving from `std::string`, may the source be destroyed or assigned to? May its old text be assumed?
17. What state does a successfully moved-from `std::unique_ptr` have?
18. Why is “check under a lock, unlock, then use” not one atomic invariant?
19. Why can a filesystem existence check followed by open be a TOCTOU bug?
20. Why can ordinary debugger breakpoints hide a data race?
21. Does `memory_order_relaxed` publish unrelated payload data?
22. Why does virtual dispatch select `Base` inside `Base::Base()`?
23. What is missing from `address + sizeof(T)` before the storage can be accessed as a live `T`?
24. Is matching `sizeof` enough to prove ABI compatibility?
25. If a class declares a destructor but no copy operations, what does the compiler still generate, and why is that dangerous for a raw owning pointer?
26. Two objects hold `shared_ptr` members pointing at each other. What are their use counts at scope exit, and which destructor runs?
27. Does reading `map["missing"]` change the container?
28. Why can you not bind `bool&` to an element of `std::vector<bool>`?
29. What does `auto x = f();` deduce when `f` returns `T&`, and what is the runtime cost?
30. A derived class declares `void log(double)`. Why does `derived.log("text")` fail to compile even though the base has a string overload?
31. A virtual function is overridden with a different default argument. Which default applies when called through a base reference?
32. What does `Timer t();` declare inside a function body?
33. In which order are members initialized, and what determines it?
34. Why is `0.1 + 0.2 == 0.3` false, and why is a fixed absolute epsilon insufficient?
35. `container.size() - 1` on an empty container yields what, and is it undefined behavior?
36. What happens when a joinable `std::thread` reaches its destructor?

## Practical exercises

1. Run `Trap05_SignedOverflow_unsafe`, break in each of its three functions, and classify each result as UB or defined behavior. Explain why sanitizer output is not expected for all three.
2. In `Trap07_IteratorInvalidation`, record vector `data()` and `capacity()` before and after growth. Reacquire the iterator and explain why the numeric address is evidence, not the language rule itself.
3. In `Trap08_StringView`, draw two boxes: the view and its owner. Step through each variant and mark the exact event that breaks the relationship.
4. In `Trap12_ObjectSlicing`, compare the dynamic type before and after a by-value copy. Confirm that no memory error is required for polymorphism to disappear.
5. In `Trap19_DoubleFree_unsafe`, use AddressSanitizer and identify the first ownership mistake, the first release, and the later reported failure. They may be different locations.
6. In `Trap21_CheckThenAct`, write the smallest possible competing-thread/process timeline between the check and use. Do not rely on single-step execution to reproduce it.
7. In `Trap29_RawBytes`, stop before `construct_at`, after construction, and after `destroy_at`. At each stop, state whether storage exists and whether a `Record` object is alive.
8. In `Trap31_ShallowCopy_unsafe`, run under AddressSanitizer and identify which of the two destructors reports the failure. Then explain why the real defect is the missing copy constructor, not the second destructor.
9. In `Trap32_SharedPtrCycle`, log `use_count()` at three points and explain why the cyclic case never reaches zero. Confirm that changing one edge to `weak_ptr` restores both destructor calls.
10. In `Trap35_AutoDropsRef`, print the address of the source object and of each deduced variable. Classify each of `auto`, `auto&`, `const auto&`, and `decltype(auto)` as copy or alias.
11. In `Trap39_MemberInitOrder_unsafe`, break in the constructor and record the value of `count` at the moment `doubled` is computed. Explain why reordering the init-list would not fix it.
12. In `Trap41_UnsignedUnderflow`, evaluate `v.size() - 1` for an empty container in the watch window. Explain why this is defined behavior yet still a bug, and why no sanitizer reports it.

## Answer key

1. Pointer value alone proves neither lifetime, bounds, alignment, permitted type access/provenance, nor synchronization.
2. `new Point` default-initializes; scalar members may remain indeterminate. `new Point{}` value-initializes and zero-initializes those members in this case.
3. It may be formed for comparison/arithmetic within the allowed domain, but not dereferenced.
4. Array-to-pointer decay discards the extent; pass a range/container, `std::span`, or an explicit validated count.
5. The overflowing signed operation is already UB, so the optimizer may assume it never happened. Check before evaluating it or use checked arithmetic.
6. The usual arithmetic conversions convert `-1` to a large unsigned `size_t`; that value is not less than the small size. The result is defined but often contrary to the intended range test.
7. No. Conversion to an unsigned destination is defined modulo one more than its maximum value. It can still be unwanted information loss.
8. Reallocation to a new backing allocation.
9. No. Invalidation rules are container- and operation-specific; learn and check the relevant contract.
10. Nothing. It is a borrowed pointer-and-length view.
11. Owner destruction/scope exit, destruction of a temporary owner, and owner mutation that reallocates or otherwise invalidates references.
12. The closure stores a reference or object pointer, not an owned snapshot; invocation can occur after the referent/object lifetime ends.
13. No. Copying into a `Base` object is defined; the derived subobject/state and derived dynamic type are not part of the copied destination.
14. The allocation/deallocation and lifetime protocols must match. A mismatch violates the required contract; placement construction does not arrange automatic destruction.
15. Both copies believe they own the same allocation, so their cleanup paths release the same resource twice. Use value semantics or a single explicit RAII owner.
16. Yes, it remains valid and can be destroyed or assigned to; its particular value is generally unspecified, so the old text cannot be assumed.
17. It is empty/null after a successful move.
18. Another actor can change or destroy the checked state after unlock and before use. Protect the complete check-and-use sequence or acquire an owning snapshot.
19. Another process can replace or change the path between operations. Prefer an atomic/open-and-validate API and operate on the acquired handle.
20. Pausing threads changes their interleaving and may accidentally serialize the conflicting accesses. Use ThreadSanitizer or deliberate schedule instrumentation.
21. No. It makes that atomic access atomic but does not by itself publish unrelated payload; use a release/acquire protocol or mutex.
22. During base construction the derived subobject is not yet active, so virtual dispatch is limited to the currently constructed class.
23. Suitable alignment, storage duration, a begun `T` lifetime, valid provenance, permitted typed access, and satisfied object invariants.
24. No. Alignment, offsets, packing, calling convention, runtime/allocator, ownership, exception rules, and representation must also agree.
25. The copy constructor and copy assignment are still generated and copy the pointer value, so two objects claim one allocation and both release it. Declare copy and move operations, or hold the resource in a member that owns it correctly.
26. Both counts remain at 1 after the scope-local owners are destroyed, so neither destructor runs and both objects leak. Exactly one edge of the cycle must be a `weak_ptr`.
27. Yes. `operator[]` default-constructs and inserts a value for a missing key. Use `find`, `at`, or `contains` when the intent is to read.
28. It is a bit-packed specialization with no addressable `bool` elements. `operator[]` returns a proxy object that writes through to the packed bits.
29. It deduces `T`, stripping the reference and top-level const, so a full copy is made and later writes do not reach the original.
30. Declaring any member named `log` hides every base overload of that name. Add `using Base::log;` to bring them back into the overload set.
31. The base default applies. The body is selected by the dynamic type, but default arguments are substituted from the static type; keep defaults out of virtual functions.
32. A function named `t` taking no parameters and returning `Timer`. Use `Timer t;` or `Timer t{};` to define an object.
33. In the order the members are declared in the class, after all base subobjects. The order written in the member-initializer list is irrelevant, so a member must never be initialized from a member declared after it.
34. Neither operand is exactly representable in binary floating point, so the sum differs from the literal `0.3` by a rounding error. A fixed epsilon fails for large magnitudes, where the smallest representable gap already exceeds it; scale the tolerance to the operands.
35. It yields the maximum value of the unsigned type. This is defined modular arithmetic, not undefined behavior, which is exactly why no sanitizer flags it. Guard with `empty()` or use signed sizes.
36. Its destructor calls `std::terminate` and the process aborts. Join or detach on every path, or use `std::jthread`, which joins in its destructor.

### Practical exercise checkpoints

- Exercise 1: signed overflow is UB; mixed-sign comparison and unsigned narrowing are defined, though potentially buggy.
- Exercise 2: a changed `data()` demonstrates that relocation occurred; the standard invalidation rule is what makes old handles invalid.
- Exercise 3: the view never extends owner lifetime, and reallocation can invalidate it before owner destruction.
- Exercise 4: slicing is a semantic/type-model bug, so ASan silence is expected.
- Exercise 5: repair ownership at its origin, not merely the second `delete` site.
- Exercise 6: the bug is the unprotected interval; a deterministic crash is not required.
- Exercise 7: raw storage exists at all three stops, but a live `Record` exists only between construction and destruction.
- Exercise 8: ASan reports the second release, while the defect originates in the implicitly generated copy.
- Exercise 9: the cyclic pair holds each other alive at count 1; the `weak_ptr` edge does not contribute to ownership.
- Exercise 10: only `auto` copies; the other three alias the original object.
- Exercise 11: `count` is still indeterminate, because declaration order fixed the sequence before the init-list was written.
- Exercise 12: the wrap is defined behavior, so the bug is visible only through the invariant, not through a tool.
