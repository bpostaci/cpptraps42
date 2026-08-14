#include <bit>
#include <cstdint>
#include <iostream>
int main(){ float value=1.0F;
    auto bits=std::bit_cast<std::uint32_t>(value); // BP: copies representation without illegal lvalue access.
    std::cout<<std::hex<<bits<<'\n'; }

