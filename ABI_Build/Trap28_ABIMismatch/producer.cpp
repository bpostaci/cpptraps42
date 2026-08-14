#include "layout.hpp"
#include <cstdint>
#pragma pack(push, 1)
struct ProducerMessage { std::uint32_t version; std::uint64_t request_id; };
#pragma pack(pop)
Layout producer_layout() {
    return {sizeof(ProducerMessage), alignof(ProducerMessage), offsetof(ProducerMessage, request_id)};
}
