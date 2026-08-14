#include <iostream>
auto bad() { int local=42; return [&local]{ return local; }; }
auto good() { int local=42; return [local]{ return local; }; }
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto callback=bad(); std::cout << callback() << '\n'; // BP: dead stack referent.
#else
    auto callback=good(); std::cout << callback() << '\n'; // Value lives in closure.
#endif
}

