#include <iostream>
#include <limits>
int main() {
    int value = std::numeric_limits<int>::max();
    // Check before addition. Performing overflow and checking afterward is already too late.
    if (value == std::numeric_limits<int>::max()) { // BP: inspect boundary.
        std::cout << "Increment rejected before signed overflow\n";
        return 0;
    }
    ++value;
}

