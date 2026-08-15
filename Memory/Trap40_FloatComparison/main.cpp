#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

// Type 1: decimal literals are not representable in binary, so exact equality fails.
void exact_equality_fails() {
	const double sum = 0.1 + 0.2;
	std::cout << std::setprecision(20) << "0.1+0.2 = " << sum << '\n';
	std::cout << "sum == 0.3 : " << std::boolalpha << (sum == 0.3) << '\n'; // BP: false.
	std::cout << "difference : " << std::setprecision(6) << (sum - 0.3) << '\n';
}

// Type 2: compare with a tolerance that scales with the magnitude of the operands.
static bool nearly_equal(double a, double b, double relative = 1e-9, double absolute = 1e-12) {
	if (a == b) { return true; } // Handles equal finite values and same-sign infinities.
	if (!std::isfinite(a) || !std::isfinite(b)) { return false; }
	const double diff = std::fabs(a - b);
	const double scale = std::max(std::fabs(a), std::fabs(b));
	return diff <= std::max(absolute, relative * scale); // Hybrid absolute/relative tolerance.
}

void tolerant_comparison() {
	std::cout << "nearly_equal(0.1+0.2, 0.3) : " << std::boolalpha
			  << nearly_equal(0.1 + 0.2, 0.3) << '\n';
	std::cout << "nearly_equal(1e9+1, 1e9)   : "
			  << nearly_equal(1e9 + 1.0, 1e9) << '\n'; // Absolute epsilon alone would be wrong here.
}

// Type 3: accumulation and non-finite values break loop conditions built on equality.
void accumulation_and_nan() {
	double running = 0.0;
	for (int i = 0; i < 10; ++i) { running += 0.1; } // Error compounds every iteration.
	std::cout << "sum of ten 0.1 == 1.0 : " << std::boolalpha << (running == 1.0) << '\n';

	for (int i = 0; i < 10; ++i) {         // Integer loop counter: exact, terminates predictably.
		const double step = i * 0.1;
		(void)step;
	}

	const double nan_value = std::nan("");
	std::cout << "nan == nan : " << (nan_value == nan_value)             // BP: always false.
			  << "  isnan    : " << std::isnan(nan_value) << '\n';       // The correct test.
}

int main() {
	exact_equality_fails();
	tolerant_comparison();
	accumulation_and_nan();
}
