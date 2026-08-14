#include <memory>
struct Widget { ~Widget() = default; };
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto* p = new Widget[3];
    delete p; // BP: allocation used new[]; deallocation must use delete[].
#else
    auto p = std::make_unique<Widget[]>(3); // Owner remembers delete[].
#endif
}

