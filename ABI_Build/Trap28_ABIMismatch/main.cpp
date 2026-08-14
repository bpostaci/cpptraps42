#include <cstddef>
#include <cstdint>
#include <iostream>
struct WireMessage { std::uint32_t version; std::uint32_t size; std::uint64_t request_id; };
int main(){ WireMessage m{1,16,9001}; // BP: compare size/alignment/offsets on both ABI sides.
    std::cout<<sizeof(m)<<' '<<alignof(WireMessage)<<' '<<offsetof(WireMessage,request_id)<<'\n';
    // A true wire format must additionally specify byte order and serialization.
}

