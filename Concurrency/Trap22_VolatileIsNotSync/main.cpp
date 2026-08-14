#include <atomic>
#include <iostream>
#include <thread>
int main(){ int payload=0; std::atomic<bool> ready{false};
    std::jthread writer([&]{payload=42; ready.store(true,std::memory_order_release);}); // BP: release.
    std::jthread reader([&]{while(!ready.load(std::memory_order_acquire)){} // BP: acquire.
                            std::cout<<payload<<'\n';}); }

