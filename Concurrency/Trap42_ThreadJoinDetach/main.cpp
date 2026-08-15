#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

static void worker(int id) {
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::cout << "worker " << id << " done\n";
}

// Type 1: a joinable thread destroyed without join() or detach() calls std::terminate.
void destroyed_while_joinable() {
#if defined(RUN_UNSAFE_EXAMPLE)
	std::thread t{worker, 1}; // BP: no join, no detach.
	// BP: ~thread() sees joinable() == true and calls std::terminate - the process dies here.
#else
	std::cout << "joinable-destroy: skipped (~thread on a joinable thread calls std::terminate)\n";
	std::thread t{worker, 1};
	t.join();
#endif
}

// Type 2: an early return or exception skips a manual join on the happy path only.
void exception_skips_manual_join() {
	try {
		std::jthread guarded{worker, 2}; // jthread joins in its destructor, even while unwinding.
		throw std::runtime_error{"failure after the thread started"};
	} catch (const std::exception& e) {
		std::cout << "caught: " << e.what() << " (jthread still joined)\n";
	}
}

// Type 3: detach outlives the data it captured; ownership must outlive the thread.
void detach_outlives_its_data() {
	{
		int local = 7;
		std::thread t{[&local] {                 // BP: capturing by reference plus detach is a race
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			(void)local;                         //     against the enclosing scope ending.
		}};
		t.join(); // Joining before the scope ends keeps 'local' alive for the whole thread.
	}

	std::vector<std::jthread> pool;
	for (int i = 3; i < 6; ++i) {
		pool.emplace_back([i] { worker(i); });   // Capture by value; jthread joins on destruction.
	}
	std::cout << "pool size=" << pool.size() << " (all joined at scope exit)\n";
}

int main() {
	destroyed_while_joinable();
	exception_skips_manual_join();
	detach_outlives_its_data();
}
