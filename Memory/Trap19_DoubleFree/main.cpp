#include <memory>
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    int* p = new int(7); delete p; // BP: first release.
    delete p; // BP: second release may corrupt allocator metadata.
#else
    auto p = std::make_unique<int>(7);
    p.reset(); // Exactly one owning object and one release event.
#endif
}

