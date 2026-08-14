#include <iostream>
const int& config(){ static const int value=42; return value; } // BP: first-use initialization is thread-safe.
int main(){ std::cout<<config()<<'\n'; }

