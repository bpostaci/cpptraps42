#include <iostream>
#include <memory>
#include <mutex>
int main(){ std::mutex m; auto shared=std::make_shared<int>(42); std::shared_ptr<int> snapshot;
    { std::scoped_lock lock(m); snapshot=shared; } // BP: check and lifetime handoff share one protocol.
    if(snapshot) std::cout<<*snapshot<<'\n'; }

