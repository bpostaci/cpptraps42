#include <iostream>
#include <map>
#include <string>

static void dump(const char* label, const std::map<std::string, int>& m) {
	std::cout << label << " size=" << m.size() << " keys=[";
	for (const auto& [key, value] : m) { std::cout << key << ':' << value << ' '; }
	std::cout << "]\n";
}

// Type 1: operator[] is a mutating call - a lookup miss default-constructs and inserts.
void bracket_inserts_silently() {
	std::map<std::string, int> scores{{"ada", 10}};
	if (scores["bob"] == 0) { // BP: "bob" did not exist; it is created with value 0 right here.
		std::cout << "bob looked absent, but...\n";
	}
	dump("bracket ", scores); // Size is 2, not 1.
}

// Type 2: read-only lookups must use find/at/contains, which never insert.
void non_mutating_lookups() {
	std::map<std::string, int> scores{{"ada", 10}};
	if (auto it = scores.find("bob"); it != scores.end()) {
		std::cout << "found bob\n";
	} else {
		std::cout << "bob absent, nothing inserted\n";
	}
	std::cout << "contains(ada)=" << std::boolalpha << scores.contains("ada")
			  << " at(ada)=" << scores.at("ada") << '\n';
	dump("lookup  ", scores); // Still size 1.
}

// Type 3: a const map has no operator[], so the trap turns into a compile error - use that.
void const_map_forbids_bracket() {
	const std::map<std::string, int> scores{{"ada", 10}};
	// scores["bob"]; // Would not compile: operator[] is non-const by design.
	std::cout << "const at(ada)=" << scores.at("ada") << '\n';

	// Counting words: operator[] is the right tool when insertion IS the intent.
	std::map<char, int> histogram;
	for (char c : std::string{"abracadabra"}) { ++histogram[c]; } // Intentional insert-or-update.
	std::cout << "histogram a=" << histogram['a'] << " b=" << histogram['b'] << '\n';
}

int main() {
	bracket_inserts_silently();
	non_mutating_lookups();
	const_map_forbids_bracket();
}
