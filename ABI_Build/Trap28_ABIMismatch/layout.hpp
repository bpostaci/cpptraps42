#pragma once
#include <cstddef>
struct Layout { std::size_t size; std::size_t alignment; std::size_t id_offset; };
Layout producer_layout();
Layout consumer_layout();
