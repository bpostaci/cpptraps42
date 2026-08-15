#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

// Type 1: unsigned subtraction below zero wraps to a huge value instead of going negative.
void size_minus_one_on_empty() {
	const std::vector<int> empty;
	const std::size_t last = empty.size() - 1; // BP: 0u - 1 wraps to SIZE_MAX; well defined, still wrong.
	std::cout << "empty.size()-1 = " << last << '\n';

	if (!empty.empty()) {                       // Guard the container instead of trusting arithmetic.
		std::cout << "last element: " << empty[empty.size() - 1] << '\n';
	} else {
		std::cout << "empty: no last element\n";
	}
}

// Type 2: a reverse loop with an unsigned index never terminates on its own.
void reverse_loop_never_ends() {
	const std::vector<int> v{10, 20, 30};
	// for (std::size_t i = v.size() - 1; i >= 0; --i) {} // BP: i >= 0 is always true for unsigned.

	for (std::size_t i = v.size(); i-- > 0;) {  // Post-decrement in the condition stops at zero.
		std::cout << "down " << v[i] << '\n';
	}
	for (auto it = v.rbegin(); it != v.rend(); ++it) { // Reverse iterators avoid indices altogether.
		std::cout << "rit  " << *it << '\n';
	}
}

// Type 3: mixing signed and unsigned converts the signed operand, flipping the comparison.
void signed_unsigned_comparison() {
	const int offset = -1;
	const std::size_t count = 3;
	std::cout << "(-1 < 3u) = " << std::boolalpha
			  << (offset < static_cast<int>(count))          // Correct: compare in the signed domain.
			  << "  raw mixed compare would be: " << (static_cast<std::size_t>(offset) < count)
			  << '\n'; // BP: -1 converts to SIZE_MAX, so the mixed comparison is false.

	const std::vector<int> v{10, 20, 30};
	std::cout << "ssize based loop:";
	for (std::ptrdiff_t i = std::ssize(v) - 1; i >= 0; --i) { // std::ssize gives a signed size.
		std::cout << ' ' << v[static_cast<std::size_t>(i)];
	}
	std::cout << '\n';
}

int main() {
	size_minus_one_on_empty();
	reverse_loop_never_ends();
	signed_unsigned_comparison();
}
