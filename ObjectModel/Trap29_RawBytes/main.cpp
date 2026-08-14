#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
struct Record { explicit Record(int n):name("record"),number(n){} std::string name; int number; };
int main(){ alignas(Record) std::byte storage[sizeof(Record)]; // Storage, not yet a Record.
    auto* candidate=reinterpret_cast<Record*>(storage);
    Record* object=std::construct_at(candidate,7); // BP: begins lifetime and establishes invariants.
    std::cout<<object->name<<':'<<object->number<<'\n'; // Valid typed access.
    std::destroy_at(object); // BP: lifetime ends although bytes remain.
}

