#include <iostream>
int main() {
    int* observer = nullptr;
    { int local = 42; observer = &local; } // BP: local lifetime ends at brace.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << *observer << '\n'; // BP: mapped stack address, but no live int.
#else
    std::cout << "Observer intentionally not dereferenced\n";
#endif
}

