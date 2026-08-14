#include <iostream>
#include <memory>
int main(){ auto owner=std::make_unique<int>(42); int* original=owner.get();
    // Keep the original pointer association instead of converting to an integer and inventing it again.
    int* observer=original; // BP: trace allocation origin, bounds, lifetime and ownership.
    std::cout<<*observer<<'\n'; }

