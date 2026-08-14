#include <iostream>
#include <memory>
int main() {
    auto owner = std::make_shared<int>(42);
    std::weak_ptr<int> observer = owner;
    owner.reset(); // BP: the observer remains, but the int lifetime ends here.
    if (auto snapshot = observer.lock()) std::cout << *snapshot << '\n';
    else std::cout << "No live object - numeric non-null checks would be insufficient\n";
}
