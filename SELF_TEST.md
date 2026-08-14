# Quick Self-Test

1. Why can a non-null pointer still be invalid?
2. What event invalidates every vector iterator during growth?
3. What does `string_view` own?
4. Why does virtual dispatch select `Base` inside `Base::Base()`?
5. What is missing from `address + sizeof(T)` before it can be used as a `T`?
6. Why is post-operation signed-overflow checking unreliable?
7. Does `memory_order_relaxed` publish unrelated payload data?
8. Which sanitizer should be used first for a data race?
9. Why can a double free crash later instead of at the second delete?
10. Is matching `sizeof` enough to prove ABI compatibility?

## Answers

1. Lifetime, bounds, alignment, provenance/type access, and synchronization can still be wrong.
2. Reallocation to a different backing buffer.
3. Nothing; it is a borrowed pointer and length.
4. The derived subobject is not active yet.
5. Correct alignment, a begun lifetime, valid provenance, permitted typed access, and established invariants.
6. The overflowing operation is already UB, allowing optimizer assumptions.
7. No. It makes only that atomic operation indivisible; release/acquire or another protocol publishes data.
8. ThreadSanitizer.
9. The second release can corrupt allocator metadata that is consulted later.
10. No; offsets, alignment, calling convention, runtime, ownership, exceptions, and representation must also agree.

