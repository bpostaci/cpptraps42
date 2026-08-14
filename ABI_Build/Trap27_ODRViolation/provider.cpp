#if defined(RUN_UNSAFE_EXAMPLE)
#define EXTRA_FIELD // Only this translation unit sees the extra member: intentional ODR violation.
#endif
#include "packet.hpp"
std::size_t provider_packet_size() { return sizeof(Packet); }
