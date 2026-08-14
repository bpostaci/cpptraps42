#include <iostream>
// Real ODR UB requires inconsistent definitions across translation units. This valid sample
// shows the prevention rule: one canonical definition, included everywhere, same build flags.
struct Packet { int id; };
int main(){ std::cout<<sizeof(Packet)<<'\n'; // BP: compare layout reports across every module.
}

