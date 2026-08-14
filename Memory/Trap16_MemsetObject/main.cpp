#include <cstring>
#include <iostream>
#include <string>
int main() {
    std::string text = "owned characters";
#if defined(RUN_UNSAFE_EXAMPLE)
    std::memset(&text, 0, sizeof text); // BP: overwrites invariants; not construction.
#else
    text.clear(); // Type-aware operation maintains the string invariant.
#endif
    std::cout << text.size() << '\n';
}

