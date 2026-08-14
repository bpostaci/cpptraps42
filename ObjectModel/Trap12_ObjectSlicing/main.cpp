#include <iostream>
struct Base { virtual ~Base()=default; virtual void type(){std::cout<<"Base\n";} };
struct Derived: Base { void type() override {std::cout<<"Derived\n";} };
int main(){ Derived d; Base sliced=d; // BP: only Base subobject is copied.
            sliced.type(); Base& polymorphic=d; polymorphic.type(); }

