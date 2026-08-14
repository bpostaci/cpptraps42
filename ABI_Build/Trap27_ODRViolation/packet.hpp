#pragma once
#include <cstddef>
struct Packet {
    int id;
#ifdef EXTRA_FIELD
    void* payload;
#endif
};
std::size_t provider_packet_size();
