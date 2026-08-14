#include <cstdint>
#include <iostream>
#include <memory>

int main() {
    auto owner = std::make_unique<int>(42);
    int* original = owner.get();
#if defined(RUN_UNSAFE_EXAMPLE)
    // ANTI-PATTERN: numeric equality alone is treated as proof of typed-access validity.
    const auto address = reinterpret_cast<std::uintptr_t>(original);
    int* reconstructed = reinterpret_cast<int*>(address); // Platform-dependent boundary operation.
    owner.reset();
    std::cout << *reconstructed << '\n'; // BP: lifetime/provenance lost; address may look unchanged.
#else
    // CORRECT: retain the original association and keep its owner alive through the access.
    int* observer = original;
    std::cout << *observer << '\n'; // BP: inspect allocation, bounds, lifetime, and ownership.
#endif
}
