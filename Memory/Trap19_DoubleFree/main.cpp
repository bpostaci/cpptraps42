#include <iostream>
#include <memory>

// Type 1: the same raw pointer is released twice.
void explicit_double_delete() {
#if defined(RUN_UNSAFE_EXAMPLE)
    int* p = new int(7); delete p; // BP: first release.
    delete p; // BP: second release may corrupt allocator metadata.
#else
    auto p = std::make_unique<int>(7);
    p.reset(); // Exactly one owning object and one release event.
    std::cout << "explicit: single release\n";
#endif
}

// Type 2: two owners adopt the same raw pointer, so both destructors free it.
void duplicated_ownership() {
#if defined(RUN_UNSAFE_EXAMPLE)
    int* raw = new int(7);
    std::unique_ptr<int> first(raw);
    std::unique_ptr<int> second(raw); // BP: two owners for one allocation.
#else
    auto first = std::make_unique<int>(7);
    auto second = std::move(first); // Ownership is transferred, never duplicated.
    std::cout << "ownership: " << *second << '\n';
#endif
}

// Type 3: a shallow copy - the implicit copy constructor duplicates the pointer, not the buffer.
struct Buffer {
    int* data;
    Buffer() : data(new int(7)) {}
    ~Buffer() { delete data; }
#if !defined(RUN_UNSAFE_EXAMPLE)
    Buffer(const Buffer& other) : data(new int(*other.data)) {} // Deep copy: Rule of Three.
    Buffer& operator=(const Buffer&) = delete;
#endif
};

void shallow_copy_double_free() {
    Buffer a;
    Buffer b = a; // BP: unsafe build copies the pointer; both destructors delete it.
    std::cout << "copy: " << *b.data << '\n';
}

int main() {
    explicit_double_delete();
    duplicated_ownership();
    shallow_copy_double_free();
}

