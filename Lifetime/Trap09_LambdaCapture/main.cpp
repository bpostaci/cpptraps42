#include <functional>
#include <iostream>
#include <memory>

// Type 1: capturing a local by reference and returning the closure.
auto bad_by_reference() { int local=42; return [&local]{ return local; }; }
auto good_by_value() { int local=42; return [local]{ return local; }; }

void escaping_reference_capture() {
#if defined(RUN_UNSAFE_EXAMPLE)
    auto callback = bad_by_reference();
    std::cout << "reference: " << callback() << '\n'; // BP: dead stack referent.
#else
    auto callback = good_by_value();
    std::cout << "reference: " << callback() << '\n'; // Value lives in closure.
#endif
}

// Type 2: [this] captures the object pointer, not the members - the closure outlives the object.
struct Session {
    int id{7};
    std::function<int()> make_reader() {
#if defined(RUN_UNSAFE_EXAMPLE)
        return [this]{ return id; }; // BP: closure keeps a raw pointer to *this.
#else
        return [copy = id]{ return copy; }; // Capture the needed state by value.
#endif
    }
};

void this_capture() {
    std::function<int()> reader;
    { Session session; reader = session.make_reader(); } // BP: session dies here.
    std::cout << "this: " << reader() << '\n';
}

// Type 3: capturing a member of an owned object without extending its lifetime.
void owned_member_capture() {
    std::function<int()> reader;
    {
        auto owner = std::make_shared<Session>();
#if defined(RUN_UNSAFE_EXAMPLE)
        Session& ref = *owner;
        reader = [&ref]{ return ref.id; }; // BP: reference outlives the shared owner.
#else
        reader = [owner]{ return owner->id; }; // Closure shares ownership.
#endif
    }
    std::cout << "owned: " << reader() << '\n';
}

int main() {
    escaping_reference_capture();
    this_capture();
    owned_member_capture();
}

