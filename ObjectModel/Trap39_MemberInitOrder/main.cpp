#include <iostream>
#include <vector>

// Type 1: members are initialised in declaration order, whatever the init-list order says.
#if defined(RUN_UNSAFE_EXAMPLE)
struct Reordered {
	int doubled;    // Declared first, so it is initialised first.
	int count;      // Declared second, even though the init-list writes count first.
	explicit Reordered(int value)
		: count(value)       // Textual order does not control construction order.
		, doubled(count * 2) // BP: runs before count(value) and reads an indeterminate int -> UB.
	{}
};
#endif

void init_list_order_is_a_lie() {
#if defined(RUN_UNSAFE_EXAMPLE)
	Reordered r{21};
	std::cout << "reordered: count=" << r.count << " doubled=" << r.doubled << '\n';
	// BP: 'doubled' was computed from an uninitialised 'count'.
#else
	std::cout << "reordered: skipped (reads an uninitialised member -> UB)\n";
#endif
}

// Type 2: derive dependent members inside the body, or order the declarations to match.
struct Ordered {
	int count;
	int doubled;
	explicit Ordered(int value) : count(value), doubled(count * 2) {} // Declaration order respected.
};

struct BodyComputed {
	int count;
	int doubled;
	explicit BodyComputed(int value) : count(value), doubled(0) {
		doubled = count * 2; // Assignment in the body has no ordering surprise at all.
	}
};

void safe_dependent_members() {
	Ordered o{21};
	BodyComputed b{21};
	std::cout << "ordered: " << o.count << '/' << o.doubled
			  << "  body: " << b.count << '/' << b.doubled << '\n';
}

// Type 3: the same rule governs base classes - bases first, then members, in declaration order.
struct Buffer {
	std::vector<int> storage;
	explicit Buffer(std::size_t n) : storage(n, 0) { std::cout << "Buffer ctor n=" << n << '\n'; }
};

struct View : Buffer {
	std::size_t size; // Members are initialised after the base subobject, always.
	explicit View(std::size_t n) : Buffer(n), size(storage.size()) {
		std::cout << "View ctor size=" << size << '\n'; // Base is fully alive by now.
	}
};

void bases_precede_members() {
	View v{4};
	std::cout << "view storage=" << v.storage.size() << '\n';
}

int main() {
	init_list_order_is_a_lie();
	safe_dependent_members();
	bases_precede_members();
}
