#include <array>
#include <bitset>
#include <iostream>
#include <vector>

// Type 1: vector<bool> is a bit-packed specialisation, so operator[] returns a proxy, not bool&.
void proxy_instead_of_reference() {
	std::vector<bool> bits{true, false, true};
	// bool& ref = bits[0];      // Would not compile: operator[] yields std::vector<bool>::reference.
	auto proxy = bits[0];        // BP: 'auto' deduces the proxy, which still aliases the container.
	proxy = false;               // BP: this writes into 'bits', although it looks like a local copy.
	std::cout << "proxy write changed container: " << std::boolalpha << bits[0] << '\n';

	bool copy = bits[2];         // Naming the type explicitly forces a real, detached copy.
	copy = false;
	std::cout << "explicit bool copy is detached: " << bits[2] << '\n';
}

// Type 2: there is no contiguous bool array underneath, so data() and pointers are unavailable.
void no_contiguous_storage() {
	std::vector<bool> bits(8, true);
	// const bool* raw = bits.data();     // Would not compile: no data() returning bool*.
	// for (bool& b : bits) { b = false; } // Would not compile: cannot bind bool& to the proxy.
	for (auto&& b : bits) { b = false; }   // Binding to the proxy by forwarding reference works.
	std::cout << "packed size=" << bits.size() << " front=" << bits.front() << '\n';

	std::vector<char> flags(8, 1);          // A real contiguous buffer when interop is needed.
	std::cout << "char buffer is contiguous: " << static_cast<const void*>(flags.data()) << '\n';
}

// Type 3: pick the container that matches the intent instead of fighting the specialisation.
void better_alternatives() {
	std::bitset<8> fixed_flags;             // Fixed-size flag set with bit operations.
	fixed_flags.set(1);
	std::cout << "bitset=" << fixed_flags << " count=" << fixed_flags.count() << '\n';

	std::array<bool, 3> small{true, false, true}; // Real bools, real references, fixed size.
	for (bool& b : small) { b = !b; }
	std::cout << "array front=" << small.front() << '\n';

	std::vector<char> dynamic_flags{1, 0, 1};     // Real bools with dynamic size.
	dynamic_flags.push_back(0);
	std::cout << "vector<char> size=" << dynamic_flags.size() << '\n';
}

int main() {
	proxy_instead_of_reference();
	no_contiguous_storage();
	better_alternatives();
}
