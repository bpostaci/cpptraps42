#include "state.hpp"
// ANTI-PATTERN: reads a dynamically initialized object from another translation unit.
// Whether global_configuration has already run its constructor is not a portable dependency.
int copied_during_static_initialization = global_configuration.value;
