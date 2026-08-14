#include <atomic>
#include <iostream>
#include <thread>

int main() {
    int payload = 0;
#if defined(RUN_UNSAFE_EXAMPLE)
    volatile bool ready = false; // ANTI-PATTERN: volatile creates no happens-before edge.
    std::jthread writer([&] { payload = 42; ready = true; });
    std::jthread reader([&] {
        while (!ready) { std::this_thread::yield(); }
        std::cout << payload << '\n'; // BP: data race on payload; use TSan.
    });
#else
    std::atomic<bool> ready{false};
    std::jthread writer([&] {
        payload = 42;
        ready.store(true, std::memory_order_release); // BP: release publishes prior write.
    });
    std::jthread reader([&] {
        while (!ready.load(std::memory_order_acquire)) { std::this_thread::yield(); }
        std::cout << payload << '\n'; // Only this thread writes to cout.
    });
#endif
}
