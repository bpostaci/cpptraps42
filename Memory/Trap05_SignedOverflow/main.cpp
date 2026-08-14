#include <iostream>
#include <limits>
#include <optional>
#include <vector>

std::optional<int> safe_add(int a, int b) {
    if ((b > 0 && a > std::numeric_limits<int>::max() - b) ||
        (b < 0 && a < std::numeric_limits<int>::min() - b)) {
        return std::nullopt; // Reject before evaluating the overflowing expression.
    }
    return a + b;
}

// Type 1: signed overflow is undefined, not wrapping.
void signed_overflow() {
#if defined(RUN_UNSAFE_EXAMPLE)
    volatile int maximum = std::numeric_limits<int>::max();
    std::cout << "overflow: " << maximum + 1 << '\n'; // BP: signed overflow occurs here; use UBSan.
#else
    std::cout << "overflow: rejected by safe_add\n";
#endif
}

// Type 2: comparing signed with unsigned converts the signed operand.
void mixed_sign_comparison() {
    const int index = -1;
    const std::vector<int> values{10, 20, 30};
#if defined(RUN_UNSAFE_EXAMPLE)
    const bool appears_in_range = index < values.size(); // BP: index becomes a huge size_t; the test is false.
    std::cout << "compare: index < size is " << std::boolalpha << appears_in_range
              << " after the usual arithmetic conversions\n";
#else
    if (index >= 0 && static_cast<std::size_t>(index) < values.size())
        std::cout << "compare: " << values[static_cast<std::size_t>(index)] << '\n';
    else std::cout << "compare: rejected negative index\n";
#endif
}

// Type 3: implicit narrowing silently discards the high bits.
void narrowing_conversion() {
    const int wide = 300;
#if defined(RUN_UNSAFE_EXAMPLE)
    unsigned char narrow = wide; // BP: defined modulo conversion, but usually unintended data loss.
    std::cout << "narrow: " << static_cast<int>(narrow) << '\n';
#else
    if (wide >= 0 && wide <= std::numeric_limits<unsigned char>::max())
        std::cout << "narrow: " << wide << '\n';
    else std::cout << "narrow: rejected out-of-range conversion\n";
#endif
}

int main() {
    if (auto sum = safe_add(19, 23)) {
        std::cout << "sum=" << *sum;
    }
    unsigned wrapping = std::numeric_limits<unsigned>::max();
    ++wrapping; // Defined modulo 2^N, unlike signed overflow.
    std::cout << "; unsigned wrap=" << wrapping << '\n';
    signed_overflow();
    mixed_sign_comparison();
    narrowing_conversion();
}
