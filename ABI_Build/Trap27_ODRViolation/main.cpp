#include "packet.hpp"
#include <iostream>
int main() {
    std::cout << "consumer sizeof(Packet)=" << sizeof(Packet)
              << ", provider sizeof(Packet)=" << provider_packet_size() << '\n'; // BP: compare TU assumptions.
}
