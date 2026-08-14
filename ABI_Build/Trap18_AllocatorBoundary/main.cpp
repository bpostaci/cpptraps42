#include <cstdlib>
#include <iostream>
struct FreeDeleter { void operator()(void* p) const { std::free(p); } };
#include <memory>
int main(){ std::unique_ptr<void,FreeDeleter> buffer(std::malloc(256)); // BP: matching allocator/deallocator.
    std::cout<<(buffer?"allocated":"failed")<<'\n';
    // Across a DLL, export a matching destroy function or caller-owned buffer contract.
}

