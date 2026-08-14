#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

// Type 1: check and act must happen under one lock, not two.
void check_then_act_lock() {
    std::mutex m;
    auto shared = std::make_shared<int>(42);
    std::shared_ptr<int> snapshot;
    { std::scoped_lock lock(m); snapshot = shared; } // BP: check and lifetime handoff share one protocol.
    if (snapshot) std::cout << "lock: " << *snapshot << '\n';
}

// Type 2: empty() then pop() - the queue can be drained between the two calls.
void empty_then_pop() {
    std::mutex m;
    std::vector<int> queue{1,2,3};
#if defined(RUN_UNSAFE_EXAMPLE)
    bool has_item;
    { std::scoped_lock lock(m); has_item = !queue.empty(); } // BP: lock released before acting.
    if (has_item) { std::scoped_lock lock(m); std::cout << "pop: " << queue.back() << '\n'; queue.pop_back(); }
#else
    std::scoped_lock lock(m); // One critical section covers both the check and the action.
    if (!queue.empty()) { std::cout << "pop: " << queue.back() << '\n'; queue.pop_back(); }
#endif
}

// Type 3: file-system TOCTOU - exists() then open() is two separate observations.
void exists_then_open() {
    const std::filesystem::path path = "trap21_sample.txt";
    { std::ofstream create(path); create << "data\n"; }
#if defined(RUN_UNSAFE_EXAMPLE)
    if (std::filesystem::exists(path)) { // BP: the file may vanish before the open below.
        std::ifstream in(path);
        std::string line; std::getline(in, line);
        std::cout << "file: " << line << '\n'; // BP: assumes the earlier check still holds.
    }
#else
    std::ifstream in(path); // Open first, then check the resulting handle - one atomic step.
    if (in) { std::string line; std::getline(in, line); std::cout << "file: " << line << '\n'; }
    else std::cout << "file: open failed\n";
#endif
    std::error_code ec; std::filesystem::remove(path, ec);
}

int main() {
    check_then_act_lock();
    empty_then_pop();
    exists_then_open();
}

