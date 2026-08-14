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

## Practical exercises

1. Run `Trap05_SignedOverflow_unsafe`, break in each of its three functions, and classify each result as UB or defined behavior. Explain why sanitizer output is not expected for all three.
2. In `Trap07_IteratorInvalidation`, record vector `data()` and `capacity()` before and after growth. Reacquire the iterator and explain why the numeric address is evidence, not the language rule itself.
3. In `Trap08_StringView`, draw two boxes: the view and its owner. Step through each variant and mark the exact event that breaks the relationship.
4. In `Trap12_ObjectSlicing`, compare the dynamic type before and after a by-value copy. Confirm that no memory error is required for polymorphism to disappear.
5. In `Trap19_DoubleFree_unsafe`, use AddressSanitizer and identify the first ownership mistake, the first release, and the later reported failure. They may be different locations.
6. In `Trap21_CheckThenAct`, write the smallest possible competing-thread/process timeline between the check and use. Do not rely on single-step execution to reproduce it.
7. In `Trap29_RawBytes`, stop before `construct_at`, after construction, and after `destroy_at`. At each stop, state whether storage exists and whether a `Record` object is alive.

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

### Practical exercise checkpoints

- Exercise 1: signed overflow is UB; mixed-sign comparison and unsigned narrowing are defined, though potentially buggy.
- Exercise 2: a changed `data()` demonstrates that relocation occurred; the standard invalidation rule is what makes old handles invalid.
- Exercise 3: the view never extends owner lifetime, and reallocation can invalidate it before owner destruction.
- Exercise 4: slicing is a semantic/type-model bug, so ASan silence is expected.
- Exercise 5: repair ownership at its origin, not merely the second `delete` site.
- Exercise 6: the bug is the unprotected interval; a deterministic crash is not required.
- Exercise 7: raw storage exists at all three stops, but a live `Record` exists only between construction and destruction.
