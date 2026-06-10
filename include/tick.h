#pragma once

#include <string>
#include <vector>
#include <cstdint>

// single trade
struct Tick {
	int64_t timestamp_ms;
	double price;
	double quantity;
};

// runs CSV and returns vector of Ticks
std::vector<Tick> parse_csv(const std::string& filepath);
