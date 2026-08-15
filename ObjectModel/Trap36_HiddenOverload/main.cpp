#include <iostream>
#include <string>

// Type 1: any declaration of a name in the derived class hides every base overload of that name.
struct Base {
	void log(int value) { std::cout << "Base::log(int) " << value << '\n'; }
	void log(const std::string& text) { std::cout << "Base::log(string) " << text << '\n'; }
};

struct Hiding : Base {
	void log(double value) { std::cout << "Hiding::log(double) " << value << '\n'; }
	// BP: this single declaration hides Base::log(int) and Base::log(const string&).
};

void name_hiding_changes_overload_resolution() {
	Hiding h;
	h.log(1); // BP: int converts to double and calls Hiding::log(double), not Base::log(int).
	// h.log("text"); // Would not compile: the string overload is hidden, not overloaded.
	h.Base::log(1); // Qualification reaches the hidden base overload explicitly.
}

// Type 2: a using-declaration pulls the base overloads back into the derived overload set.
struct Exposing : Base {
	using Base::log; // Re-introduces both base overloads alongside the new one.
	void log(double value) { std::cout << "Exposing::log(double) " << value << '\n'; }
};

void using_declaration_restores_the_set() {
	Exposing e;
	e.log(1);        // Exact match wins: Base::log(int).
	e.log(1.5);      // Exposing::log(double).
	e.log("text");   // Base::log(const string&) is visible again.
}

// Type 3: hiding also breaks virtual dispatch when the signature does not match exactly.
struct Shape {
	virtual ~Shape() = default;
	virtual void draw(int layer) const { std::cout << "Shape::draw layer=" << layer << '\n'; }
};

struct Circle : Shape {
	// void draw(long layer) const override {} // Would not compile: 'override' catches the mismatch.
	void draw(int layer) const override { std::cout << "Circle::draw layer=" << layer << '\n'; }
};

void override_keyword_catches_mismatch() {
	const Circle c;
	const Shape& s = c;
	s.draw(3); // Dispatches to Circle::draw because the signature matches exactly.
}

int main() {
	name_hiding_changes_overload_resolution();
	using_declaration_restores_the_set();
	override_keyword_catches_mismatch();
}
