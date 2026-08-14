#include <iostream>

// Type 1: an automatic scalar has no value until one is established.
void uninitialized_local() {
#if defined(RUN_UNSAFE_EXAMPLE)
    int count; // No value has been established.
    std::cout << "local: " << count << '\n'; // BP: invalid read; use warnings/MemorySanitizer.
#else
    int count{}; // Value initialization establishes zero.
    std::cout << "local: " << count << '\n';
#endif
}

// Type 2: new T default-initializes a POD; only new T{} zero-initializes it.
struct Point { int x; int y; };

void default_vs_value_new() {
#if defined(RUN_UNSAFE_EXAMPLE)
    Point* p = new Point; // BP: members are default-initialized, i.e. indeterminate.
    std::cout << "new: " << p->x << ',' << p->y << '\n'; // BP: invalid read.
    delete p;
#else
    Point* p = new Point{}; // Value initialization zeroes both members.
    std::cout << "new: " << p->x << ',' << p->y << '\n';
    delete p;
#endif
}

// Type 3: a partial aggregate initializer only looks complete.
struct Config { int retries; int timeout; int backoff; };

void partial_aggregate() {
#if defined(RUN_UNSAFE_EXAMPLE)
    Config c; // BP: no initializer at all; every member is indeterminate.
    std::cout << "aggregate: " << c.retries << ',' << c.timeout << ',' << c.backoff << '\n';
#else
    Config c{3}; // Remaining members are value-initialized to zero by the aggregate rules.
    std::cout << "aggregate: " << c.retries << ',' << c.timeout << ',' << c.backoff << '\n';
#endif
}

int main() {
    uninitialized_local();
    default_vs_value_new();
    partial_aggregate();
}

