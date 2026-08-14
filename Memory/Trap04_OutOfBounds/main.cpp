#include <array>
#include <iostream>

// Type 1: unchecked subscript - operator[] never validates the index.
void unchecked_subscript() {
    std::array<int, 4> values{10,20,30,40};
    const std::size_t index = 4;
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "subscript: " << values[index] << '\n'; // BP: data()+size() is one-past, not readable.
#else
    if (index < values.size()) std::cout << "subscript: " << values[index] << '\n';
    else std::cout << "subscript: rejected index " << index << '\n';
#endif
}

// Type 2: off-by-one loop - '<=' walks one element past the end.
void off_by_one_loop() {
    std::array<int, 4> values{10,20,30,40};
    int sum = 0;
#if defined(RUN_UNSAFE_EXAMPLE)
    for (std::size_t i = 0; i <= values.size(); ++i) sum += values[i]; // BP: last iteration is out of bounds.
#else
    for (std::size_t i = 0; i < values.size(); ++i) sum += values[i];
#endif
    std::cout << "loop: sum=" << sum << '\n';
}

// Type 3: wrong element count - sizeof on a decayed pointer measures the pointer, not the array.
void decayed_array_size(const int* data) {
#if defined(RUN_UNSAFE_EXAMPLE)
    const std::size_t count = sizeof(data) / sizeof(data[0]); // BP: sizeof(pointer), not the array.
    int sum = 0;
    for (std::size_t i = 0; i < count; ++i) sum += data[i];
    std::cout << "decay: count=" << count << " sum=" << sum << '\n';
#else
    (void)data;
    std::cout << "decay: pass the size explicitly or use std::span/std::array\n";
#endif
}

int main() {
    unchecked_subscript();
    off_by_one_loop();
    const int raw[4]{10,20,30,40};
    decayed_array_size(raw);
}

