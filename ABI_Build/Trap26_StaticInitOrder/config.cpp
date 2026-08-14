#include "state.hpp"
Configuration::Configuration() : value(42) {}
Configuration global_configuration; // Dynamic initialization in this translation unit.
const Configuration& safe_configuration() {
    static const Configuration value; // Construct-on-first-use establishes dependency order.
    return value;
}
