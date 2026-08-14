#include <iostream>

int main() {
    int value = 1;
#if defined(RUN_UNSAFE_EXAMPLE)
    // ANTI-PATTERN: multiple unsequenced modifications of the same scalar object.
    value = value++ + ++value; // BP: undefined behavior; do not reason about an output value.
#else
    // CORRECT: express each state transition as a separate sequenced statement.
    const int old = value;
    ++value;
    const int after_first_increment = value;
    ++value;
    value = old + after_first_increment;
#endif
    std::cout << value << '\n';
}
