#include <iostream>
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    int count; // No value has been established.
    std::cout << count << '\n'; // BP: invalid read; use warnings/MemorySanitizer.
#else
    int count{}; // Value initialization establishes zero.
    std::cout << count << '\n';
#endif
}

