#include <iostream>
struct Base { Base(){ speak(); } virtual ~Base()=default; virtual void speak(){std::cout<<"Base phase\n";} };
struct Derived final: Base { int ready{99}; void speak() override {std::cout<<"Derived "<<ready<<'\n';} };
int main(){ Derived d; // BP: Base constructor calls Base::speak; Derived part is not active yet.
            d.speak(); }

