#include <iostream>
#include <mutex>
#include <stdexcept>
int main(){ std::mutex m; try { std::scoped_lock lock(m); // BP: resource tied to scope.
        throw std::runtime_error("failure"); // Destructor unlocks during unwinding.
    } catch(...) { std::cout<<"recovered and unlocked\n"; } }

