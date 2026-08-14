#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>

struct Widget { int value{1}; ~Widget() = default; };

// Type 1: new[] paired with scalar delete.
void array_form_mismatch() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto* p = new Widget[3];
    delete p; // BP: allocation used new[]; deallocation must use delete[].
#else
    auto p = std::make_unique<Widget[]>(3); // Owner remembers delete[].
    std::cout << "array: " << p[0].value << '\n';
#endif
}

// Type 2: malloc paired with delete - different allocator, and no constructor ever ran.
void allocator_family_mismatch() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto* p = static_cast<Widget*>(std::malloc(sizeof(Widget)));
    delete p; // BP: malloc storage must be released with free, and was never constructed.
#else
    auto p = std::make_unique<Widget>(); // One family: new/delete via the owner.
    std::cout << "family: " << p->value << '\n';
#endif
}

// Type 3: placement new paired with delete - the caller owns the buffer, not operator delete.
void placement_new_mismatch() {
    alignas(Widget) unsigned char buffer[sizeof(Widget)];
    Widget* p = new (buffer) Widget{}; // BP: no allocation happened; only construction.
#if defined(RUN_UNSAFE_EXAMPLE)
    delete p; // BP: frees stack storage that operator new never returned.
#else
    std::cout << "placement: " << p->value << '\n';
    p->~Widget(); // Destroy explicitly; the buffer is released by its own scope.
#endif
}

int main() {
    array_form_mismatch();
    allocator_family_mismatch();
    placement_new_mismatch();
}

