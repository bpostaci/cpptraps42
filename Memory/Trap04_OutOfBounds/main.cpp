#include <array>
#include <iostream>
int main() {
    std::array<int, 4> values{10,20,30,40};
    const std::size_t index = 4;
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << values[index] << '\n'; // BP: data()+size() is one-past, not readable.
#else
    if (index < values.size()) std::cout << values[index] << '\n';
    else std::cout << "Rejected index " << index << '\n';
#endif
}

