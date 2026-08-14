#include "state.hpp"
#include <iostream>
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "cross-TU copied value=" << copied_during_static_initialization << '\n'; // BP: link order may matter.
#else
    std::cout << "first-use value=" << safe_configuration().value << '\n';
#endif
}
