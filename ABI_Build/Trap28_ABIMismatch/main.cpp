#include "layout.hpp"
#include <cstdint>
#include <iostream>
struct ConsumerMessage { std::uint32_t version; std::uint64_t request_id; };
Layout consumer_layout() {
    return {sizeof(ConsumerMessage), alignof(ConsumerMessage), offsetof(ConsumerMessage, request_id)};
}
int main() {
    const Layout producer = producer_layout();
    const Layout consumer = consumer_layout();
    std::cout << "producer: size=" << producer.size << " align=" << producer.alignment
              << " id-offset=" << producer.id_offset << '\n';
    std::cout << "consumer: size=" << consumer.size << " align=" << consumer.alignment
              << " id-offset=" << consumer.id_offset << '\n'; // BP: layouts disagree across boundary.
}
