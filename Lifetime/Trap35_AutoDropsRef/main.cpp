#include <iostream>
#include <string>
#include <vector>

struct Heavy {
	std::string payload;
	int hits = 0;
};

static Heavy& shared_instance() {
	static Heavy instance{"large-payload", 0};
	return instance;
}

// Type 1: plain 'auto' strips reference and const, so the write lands on a copy.
void auto_copies_a_reference() {
	auto copy = shared_instance(); // BP: deduced as Heavy, not Heavy& - a full copy is made.
	copy.hits = 42;                // BP: mutates the copy; the shared object is untouched.
	std::cout << "copy hits=" << copy.hits << " shared hits=" << shared_instance().hits << '\n';

	auto& reference = shared_instance(); // 'auto&' keeps the binding to the original.
	reference.hits = 42;
	std::cout << "after auto&: shared hits=" << shared_instance().hits << '\n';
}

// Type 2: the same deduction rule silently copies every element of a range-for loop.
void range_for_copies_elements() {
	std::vector<Heavy> items(3, Heavy{"element", 0});
	for (auto item : items) { item.hits = 1; } // BP: each iteration copies and discards.
	std::cout << "by value: items[0].hits=" << items[0].hits << '\n';

	for (auto& item : items) { item.hits = 1; } // Mutating loop: bind by reference.
	std::cout << "by ref  : items[0].hits=" << items[0].hits << '\n';

	for (const auto& item : items) { (void)item.payload; } // Read-only loop: const reference.
	std::cout << "const ref loop made no copies\n";
}

// Type 3: proxy-returning APIs need decltype(auto) or an explicit type to keep the semantics.
void keeping_the_exact_type() {
	Heavy& source = shared_instance();
	decltype(auto) exact = shared_instance(); // Deduces Heavy&, preserving the reference.
	exact.hits = 7;
	std::cout << "decltype(auto) shared hits=" << source.hits << '\n';

	const auto& observed = shared_instance(); // No copy, and mutation is compile-checked away.
	std::cout << "observed payload=" << observed.payload << '\n';
}

int main() {
	auto_copies_a_reference();
	range_for_copies_elements();
	keeping_the_exact_type();
}
