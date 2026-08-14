#include <iostream>
#include <vector>

// Type 1: automatic storage - the pointee dies at the end of its block.
void scope_exit_dangle() {
    int* observer = nullptr;
    { int local = 42; observer = &local; } // BP: local lifetime ends at brace.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "scope: " << *observer << '\n'; // BP: mapped stack address, but no live int.
#else
    (void)observer;
    std::cout << "scope: observer intentionally not dereferenced\n";
#endif
}

// Type 2: dynamic storage - the pointee dies at delete, the pointer keeps the address.
void deleted_heap_dangle() {
    int* observer = new int{42};
    delete observer; // BP: storage returned to the allocator; observer is now stale.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "heap: " << *observer << '\n'; // BP: heap-use-after-free; enable ASan.
#else
    std::cout << "heap: pointer cleared instead of reused\n";
    observer = nullptr;
    (void)observer;
#endif
}

// Type 3: container reallocation - growth moves the elements and invalidates old pointers.
void reallocation_dangle() {
    std::vector<int> values{1, 2, 3};
    int* observer = &values.front();
    values.reserve(values.capacity() + 1); // BP: buffer moves; observer points at freed storage.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "vector: " << *observer << '\n'; // BP: reads the old, released buffer.
#else
    (void)observer;
    std::cout << "vector: re-read by index instead: " << values.front() << '\n';
#endif
}

int main() {
    scope_exit_dangle();
    deleted_heap_dangle();
    reallocation_dangle();
}

