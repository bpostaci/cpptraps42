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

// Type 1: addition past INT_MAX is undefined, not wrapping.
void additive_overflow() {
#if defined(RUN_UNSAFE_EXAMPLE)
    volatile int maximum = std::numeric_limits<int>::max();
    std::cout << "add: " << maximum + 1 << '\n'; // BP: signed overflow occurs here; use UBSan.
#else
    std::cout << "add: rejected by safe_add\n";
#endif
}

// Type 2: multiplication overflows long before the operands look suspicious.
void multiplicative_overflow() {
    volatile int factor = 100000;
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "mul: " << factor * factor << '\n'; // BP: 10^10 does not fit in int.
#else
    const long long widened = static_cast<long long>(factor) * factor; // Widen before multiplying.
    std::cout << "mul: " << widened << '\n';
#endif
}

// Type 3: negating INT_MIN has no representable result.
void negation_overflow() {
    volatile int minimum = std::numeric_limits<int>::min();
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "neg: " << -minimum << '\n'; // BP: |INT_MIN| exceeds INT_MAX.
#else
    const long long widened = -static_cast<long long>(minimum); // Widen before negating.
    std::cout << "neg: " << widened << '\n';
#endif
}

int main() {
    if (auto sum = safe_add(19, 23)) {
        std::cout << "sum=" << *sum;
    }
    unsigned wrapping = std::numeric_limits<unsigned>::max();
    ++wrapping; // Defined modulo 2^N, unlike signed overflow.
    std::cout << "; unsigned wrap=" << wrapping << '\n';
    additive_overflow();
    multiplicative_overflow();
    negation_overflow();
}
