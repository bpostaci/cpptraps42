#include <bit>
#include <cstdint>
#include <iostream>
int main() {
    float value = 1.0F;
#if defined(RUN_UNSAFE_EXAMPLE)
    // ANTI-PATTERN: the cast changes the pointer type, not the accessible object type.
    auto bits = *reinterpret_cast<std::uint32_t*>(&value); // BP: strict-aliasing violation.
#else
    // CORRECT: copy the value representation between equal-size trivially copyable types.
    auto bits = std::bit_cast<std::uint32_t>(value); // BP: legal representation copy.
#endif
    std::cout << std::hex << bits << '\n';
}
