#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){ alignas(std::uint64_t) std::byte storage[sizeof(std::uint64_t)+1];
#if defined(RUN_UNSAFE_EXAMPLE)
    auto* p=reinterpret_cast<std::uint64_t*>(storage+1); *p=7; // BP: misaligned typed access.
#else
    auto* p=reinterpret_cast<std::uint64_t*>(storage); *p=7; std::cout<<*p<<'\n';
#endif
}

