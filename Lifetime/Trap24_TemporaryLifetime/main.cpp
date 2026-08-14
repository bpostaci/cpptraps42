#include <iostream>
#include <string>
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    const char* p=std::string("hello").c_str(); // BP: owner dies at semicolon.
    std::cout << p << '\n'; // BP: dangling borrowed pointer.
#else
    std::string owner="hello"; std::cout << owner.c_str() << '\n';
#endif
}

