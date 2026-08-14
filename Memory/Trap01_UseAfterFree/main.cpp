#include <iostream>
#include <memory>
struct Widget { int value{42}; };
int main() {
    auto owner = std::make_unique<Widget>();
    Widget* observer = owner.get();
    std::cout << observer->value << '\n'; // Safe: owner still controls a live Widget.
    owner.reset(); // BP: lifetime ends and storage is returned to the allocator.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << observer->value << '\n'; // BP: heap-use-after-free; enable ASan.
#endif
}

