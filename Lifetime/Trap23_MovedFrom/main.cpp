#include <iostream>
#include <string>
int main() {
    std::string source="payload";
    std::string destination=std::move(source); // BP: source valid, state unspecified.
    source.clear(); // Establish a known state before semantic reuse.
    std::cout << destination << ", source.size=" << source.size() << '\n';
}

