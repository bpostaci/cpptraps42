#include <memory>
struct Base { virtual ~Base()=default; }; // Virtual because deletion through Base is supported.
struct Derived final: Base { std::unique_ptr<int> resource=std::make_unique<int>(42); };
int main(){ std::unique_ptr<Base> p=std::make_unique<Derived>(); // BP: destruction dispatches correctly.
}

