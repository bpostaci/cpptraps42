#include <iostream>
#include <string>
#include <vector>

struct Timer {
	int ticks = 7;
	Timer() = default;
	explicit Timer(int value) : ticks(value) {}
};

// Type 1: anything that can be parsed as a declaration IS parsed as a declaration.
void empty_parens_declare_a_function() {
	// The compiler does warn here: MSVC C4930, GCC/Clang -Wvexing-parse. Silenced locally so the
	// rest of the project can still build with warnings-as-errors enabled.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4930) // 'Timer t(void)': prototyped function not called.
#endif
	Timer t(); // BP: this declares a function 't' returning Timer - no object is created.
	// std::cout << t.ticks;  // Would not compile: 't' is a function, not a Timer.
	// sizeof(t);             // Would not compile either: a function type has no size.
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

	Timer braced{};   // Braces cannot start a function declaration: a real default-constructed Timer.
	Timer plain;      // No parentheses at all: also a real object.
	std::cout << "vexing: braced=" << braced.ticks << " plain=" << plain.ticks << '\n';
}

// Type 2: the same rule bites when the argument is itself a constructor call.
void named_argument_becomes_a_parameter() {
	// std::vector<int> v(std::istream_iterator<int>(std::cin), std::istream_iterator<int>());
	//   ^ declares a function taking two iterators, not a vector.

	std::string source{"1 2 3"};
	std::vector<char> from_iterators(source.begin(), source.end()); // Named values are unambiguous.
	std::vector<char> braced_form{source.begin(), source.end()};    // Braces remove all doubt.
	std::cout << "iterators size=" << from_iterators.size()
			  << " braced size=" << braced_form.size() << '\n';
}

// Type 3: braces are not a blanket replacement - initializer_list overloads win over them.
void braces_have_their_own_rule() {
	std::vector<int> parens(3, 0); // Three elements, all zero.
	std::vector<int> braces{3, 0}; // Two elements: 3 and 0 - initializer_list takes priority.
	std::cout << "parens size=" << parens.size() << " braces size=" << braces.size() << '\n';

	Timer sized{3}; // For types without an initializer_list constructor, braces are equivalent.
	std::cout << "sized ticks=" << sized.ticks << '\n';
}

int main() {
	empty_parens_declare_a_function();
	named_argument_becomes_a_parameter();
	braces_have_their_own_rule();
}
