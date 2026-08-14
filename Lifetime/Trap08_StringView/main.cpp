#include <iostream>
#include <string>
#include <string_view>
std::string_view bad_view() { std::string s="temporary owner"; return s; }
int main() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto view = bad_view();
    std::cout << view << '\n'; // BP: view owns neither pointer target nor lifetime.
#else
    std::string owner = "stable owner";
    std::string_view view = owner;
    std::cout << view << '\n';
#endif
}

