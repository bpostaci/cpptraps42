#include <iostream>
#include <map>
#include <vector>

// Type 1: growth reallocates the buffer and invalidates iterators, pointers and references alike.
void reallocation_invalidation() {
    std::vector<int> v{1,2,3}; v.shrink_to_fit();
    auto old = v.begin();
    int& old_reference = v.front(); // A reference is invalidated by exactly the same event.
    const int* old_buffer = v.data();
    v.push_back(4); // BP: reallocation can invalidate all handles into the vector.
    std::cout << "old buffer=" << old_buffer << " new buffer=" << v.data() << '\n';
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "realloc: " << *old << " / " << old_reference << '\n'; // BP: stale iterator and reference.
#else
    (void)old; (void)old_reference;
    std::cout << "realloc: " << *v.begin() << " / " << v.front() << '\n'; // Reacquire after mutation.
#endif
}

// Type 2: erase invalidates the erased iterator; it must be replaced by the return value.
void erase_invalidation() {
    std::vector<int> v{1,2,3,4};
    auto it = v.begin() + 1;
    auto next = v.erase(it); // BP: 'it' is dead from here on; 'next' is the valid successor.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "erase: " << *it << '\n'; // BP: use of an erased iterator.
#else
    (void)it;
    std::cout << "erase: " << *next << '\n'; // Continue from the returned iterator.
#endif
}

// Type 3: node-based containers keep other iterators valid, but not the erased node.
void node_invalidation() {
    std::map<int, int> m{{1,10},{2,20},{3,30}};
    auto erased = m.find(2);
    auto survivor = m.find(3);
    m.erase(erased); // BP: only 'erased' dies; 'survivor' stays valid in a node-based map.
#if defined(RUN_UNSAFE_EXAMPLE)
    std::cout << "node: " << erased->second << '\n'; // BP: the node is already destroyed.
#else
    (void)erased;
    std::cout << "node: survivor still valid: " << survivor->second << '\n';
#endif
}

int main() {
    reallocation_invalidation();
    erase_invalidation();
    node_invalidation();
}

