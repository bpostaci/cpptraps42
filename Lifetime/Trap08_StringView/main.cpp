#include <iostream>
#include <string>
#include <string_view>

std::string make_owner() { return "temporary owner"; }

// Type 1: returning a view to a local string - the owner dies at return.
std::string_view returned_view() {
    std::string s = "temporary owner";
    return s; // BP: view outlives the string it points into.
}

void dangling_return() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto view = returned_view();
    std::cout << "return: " << view << '\n'; // BP: view owns neither pointer target nor lifetime.
#else
    std::string owner = "stable owner";
    std::string_view view = owner;
    std::cout << "return: " << view << '\n';
#endif
}

// Type 2: binding a view to a temporary - the temporary dies at the end of the full expression.
void temporary_binding() {
#if defined(RUN_UNSAFE_EXAMPLE)
    std::string_view view = make_owner(); // BP: temporary destroyed after this statement.
    std::cout << "temporary: " << view << '\n'; // BP: reads released string storage.
#else
    std::string owner = make_owner(); // Keep the owner alive in a named variable.
    std::string_view view = owner;
    std::cout << "temporary: " << view << '\n';
#endif
}

// Type 3: mutating the owner - reassignment or growth moves the buffer under the view.
void mutated_owner() {
    std::string owner = "short";
    std::string_view view = owner;
    owner = "a much longer replacement string that forces reallocation"; // BP: view now stale.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "mutation: " << view << '\n'; // BP: view points into the old buffer.
#else
    view = owner; // Re-seat the view after every owner mutation.
    std::cout << "mutation: " << view << '\n';
#endif
}

int main() {
    dangling_return();
    temporary_binding();
    mutated_owner();
}

