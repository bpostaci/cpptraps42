#include <iostream>
#include <memory>

// Type 1: two shared_ptr members pointing at each other keep the reference counts above zero forever.
struct CyclicNode {
	std::shared_ptr<CyclicNode> peer;
	int id;
	explicit CyclicNode(int value) : id(value) { std::cout << "ctor  " << id << '\n'; }
	~CyclicNode() { std::cout << "dtor  " << id << '\n'; } // BP: never reached in the cyclic case.
};

void shared_ptr_cycle_leaks() {
	auto a = std::make_shared<CyclicNode>(1);
	auto b = std::make_shared<CyclicNode>(2);
	a->peer = b;
	b->peer = a; // BP: use_count of both is now 2; scope exit only drops it back to 1.
	std::cout << "cycle use_count: " << a.use_count() << '\n';
} // BP: no destructor output here - the two nodes are leaked.

// Type 2: making the back edge non-owning breaks the cycle.
struct WeakNode {
	std::shared_ptr<WeakNode> next;
	std::weak_ptr<WeakNode> prev; // The back edge observes without owning.
	int id;
	explicit WeakNode(int value) : id(value) {}
	~WeakNode() { std::cout << "dtor  weak" << id << '\n'; }
};

void weak_ptr_breaks_cycle() {
	auto a = std::make_shared<WeakNode>(1);
	auto b = std::make_shared<WeakNode>(2);
	a->next = b;
	b->prev = a; // Owning count of 'a' stays 1, so scope exit destroys both.
	std::cout << "weak use_count: " << a.use_count() << '\n';
}

// Type 3: a weak_ptr must be locked before use; expiry is a normal, checkable state.
void locking_a_weak_ptr() {
	std::weak_ptr<WeakNode> observer;
	{
		auto owner = std::make_shared<WeakNode>(3);
		observer = owner;
		if (auto locked = observer.lock()) { // lock() yields a temporary owner while in use.
			std::cout << "locked: " << locked->id << '\n';
		}
	} // The only owner dies here.
	std::cout << "expired: " << std::boolalpha << observer.expired() << '\n';
}

int main() {
	shared_ptr_cycle_leaks();
	weak_ptr_breaks_cycle();
	locking_a_weak_ptr();
}
