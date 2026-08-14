#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{1,2,3}; v.shrink_to_fit();
    auto old = v.begin();
    const int* old_buffer = v.data();
    v.push_back(4); // BP: reallocation can invalidate all handles into the vector.
    std::cout << "old buffer=" << old_buffer << " new buffer=" << v.data() << '\n';
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << *old << '\n'; // BP: stale iterator; use ASan/debug iterators.
#else
    std::cout << *v.begin() << '\n'; // Reacquire after mutation.
#endif
}

