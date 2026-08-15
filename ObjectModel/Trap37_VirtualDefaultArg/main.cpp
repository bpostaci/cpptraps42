#include <iostream>

// Type 1: the function body is chosen dynamically, the default argument statically.
struct Base {
	virtual ~Base() = default;
	virtual void render(int scale = 1) const { std::cout << "Base::render scale=" << scale << '\n'; }
};

struct Derived : Base {
	void render(int scale = 100) const override { // BP: a different default on an override.
		std::cout << "Derived::render scale=" << scale << '\n';
	}
};

void default_argument_comes_from_static_type() {
	Derived d;
	const Base& as_base = d;
	d.render();       // Derived::render scale=100 - static type is Derived.
	as_base.render(); // BP: Derived::render scale=1 - body from Derived, default from Base.
}

// Type 2: keep the virtual interface free of defaults; a non-virtual wrapper owns them.
struct Interface {
	virtual ~Interface() = default;
	void render(int scale = 1) const { do_render(scale); } // Non-virtual, single default.
private:
	virtual void do_render(int scale) const = 0; // Virtual, no default argument at all.
};

struct Impl : Interface {
private:
	void do_render(int scale) const override { std::cout << "Impl::do_render scale=" << scale << '\n'; }
};

void non_virtual_interface_has_one_default() {
	Impl i;
	const Interface& as_interface = i;
	i.render();
	as_interface.render(); // Same default through either static type.
}

// Type 3: overloading instead of defaulting removes the ambiguity entirely.
struct Explicit {
	virtual ~Explicit() = default;
	virtual void render(int scale) const { std::cout << "Explicit::render scale=" << scale << '\n'; }
	void render() const { render(1); } // The "default" becomes a real, inherited overload.
};

void overloads_instead_of_defaults() {
	Explicit e;
	const Explicit& as_ref = e;
	as_ref.render();
	as_ref.render(5);
}

int main() {
	default_argument_comes_from_static_type();
	non_virtual_interface_has_one_default();
	overloads_instead_of_defaults();
}
