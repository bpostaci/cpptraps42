#include <atomic>
#include <iostream>
#include <thread>
int main(){
#if defined(RUN_UNSAFE_EXAMPLE)
    int counter=0; auto work=[&]{for(int i=0;i<100000;++i) ++counter;}; // BP: data race; use TSan.
#else
    std::atomic<int> counter{0}; auto work=[&]{for(int i=0;i<100000;++i) counter.fetch_add(1,std::memory_order_relaxed);};
#endif
    std::jthread a(work),b(work); a.join(); b.join(); std::cout<<counter<<'\n'; }

