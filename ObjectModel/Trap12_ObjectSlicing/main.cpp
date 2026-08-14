#include <iostream>
#include <memory>
#include <vector>

struct Base { virtual ~Base()=default; virtual void type() const {std::cout<<"Base\n";} };
struct Derived: Base { void type() const override {std::cout<<"Derived\n";} };

// Type 1: copy-initializing a Base from a Derived copies only the Base subobject.
void copy_slicing() {
    Derived d;
    Base sliced = d; // BP: only Base subobject is copied.
    std::cout << "copy: "; sliced.type();
    const Base& polymorphic = d;
    std::cout << "reference: "; polymorphic.type();
}

// Type 2: a container of Base slices every element on insertion.
void container_slicing() {
#if defined(RUN_UNSAFE_EXAMPLE)
    std::vector<Base> values;
    values.push_back(Derived{}); // BP: the Derived part is discarded on copy.
    std::cout << "container: "; values.front().type();
#else
    std::vector<std::unique_ptr<Base>> values;
    values.push_back(std::make_unique<Derived>()); // Store handles, not values.
    std::cout << "container: "; values.front()->type();
#endif
}

// Type 3: a by-value parameter slices at the call boundary.
void print_by_value(Base b) { std::cout << "by value: "; b.type(); }
void print_by_reference(const Base& b) { std::cout << "by reference: "; b.type(); }

void parameter_slicing() {
    Derived d;
#if defined(RUN_UNSAFE_EXAMPLE)
    print_by_value(d); // BP: the parameter copy is a plain Base.
#else
    print_by_reference(d); // Reference parameters preserve the dynamic type.
#endif
}

int main() {
    copy_slicing();
    container_slicing();
    parameter_slicing();
}

