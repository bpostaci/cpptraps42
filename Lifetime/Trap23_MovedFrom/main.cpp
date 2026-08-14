#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Type 1: a moved-from standard container is valid but its value is unspecified.
void moved_from_container() {
    std::string source = "payload";
    std::string destination = std::move(source); // BP: source valid, state unspecified.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "container: assuming old content: " << source << '\n'; // BP: unspecified value.
#else
    source.clear(); // Establish a known state before semantic reuse.
    std::cout << "container: " << destination << ", source.size=" << source.size() << '\n';
#endif
}

// Type 2: a moved-from unique_ptr is guaranteed null - dereferencing it is undefined.
void moved_from_owner() {
    auto source = std::make_unique<int>(7);
    auto destination = std::move(source); // BP: source is now null by contract.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "owner: " << *source << '\n'; // BP: null dereference.
#else
    std::cout << "owner: " << *destination << ", source empty=" << (source == nullptr) << '\n';
#endif
}

// Type 3: self-move leaves the object in a valid but unspecified state.
void self_move() {
    std::vector<int> values{1,2,3};
#if defined(RUN_UNSAFE_EXAMPLE)
    values = std::move(values); // BP: self-move; contents become unspecified.
    std::cout << "self: size=" << values.size() << '\n'; // BP: relies on an unspecified state.
#else
    std::cout << "self: size=" << values.size() << " (self-move avoided)\n";
#endif
}

int main() {
    moved_from_container();
    moved_from_owner();
    self_move();
}

