#include <iostream>
#include <limits>
#include <optional>

std::optional<int> safe_add(int a, int b) {
    if ((b > 0 && a > std::numeric_limits<int>::max() - b) ||
        (b < 0 && a < std::numeric_limits<int>::min() - b)) {
        return std::nullopt; // Reject before evaluating the overflowing expression.
    }
    return a + b;
}

int main() {
    if (auto sum = safe_add(19, 23)) {
        std::cout << "sum=" << *sum;
    }
    unsigned wrapping = std::numeric_limits<unsigned>::max();
    ++wrapping; // Defined modulo 2^N, unlike signed overflow.
    std::cout << "; unsigned wrap=" << wrapping << '\n';
#if defined(RUN_UNSAFE_EXAMPLE)
    volatile int maximum = std::numeric_limits<int>::max();
    std::cout << maximum + 1 << '\n'; // BP: signed overflow occurs here; use UBSan.
#endif
}
