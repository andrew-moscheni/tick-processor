#include <iostream>
#include "tick.h"

int main() {
	std::string filepath = "../data/BTCUSDT-trades-2024-01-15.csv";

	std::cout << "Loading tick data from: " << filepath << std::endl;

	auto ticks = parse_csv(filepath);

	if (ticks.empty()) {
		std::cerr << "No ticks loaded. Check the file path." << std::endl;
		return 1;
	}

	std::cout << "First tick  — time: " << ticks.front().timestamp_ms
              << "  price: $" << ticks.front().price
              << "  qty: "    << ticks.front().quantity << std::endl;

    	std::cout << "Last tick   — time: " << ticks.back().timestamp_ms
              << "  price: $" << ticks.back().price
              << "  qty: "    << ticks.back().quantity << std::endl;

    	std::cout << "Total ticks: " << ticks.size() << std::endl;
	return 0;
}
