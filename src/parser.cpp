#include "tick.h"

#include <fstream> // reading files
#include <sstream> // splitting strings
#include <stdexcept> //error-throwing
#include <iostream> // warnings

// String split into vector of strings
static std::vector<std::string> split(const std::string& line, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream stream(line);

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

// turn CSV into vector of Ticks
std::vector<Tick> parse_csv(const std::string& filepath) {
	std::ifstream file(filepath);

	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + filepath);
	}

	std::vector<Tick> ticks;
	std::string line;
	int line_number = 0;
	int skipped = 0;

	while (std::getline(file, line)) {
		line_number++;

		if (line.empty()) continue;

		std::vector<std::string> cols = split(line, ',');

		if (cols.size() < 7) {
			skipped++;
			continue;
		}

		try {
			Tick tick;

			tick.price = std::stod(cols[1]);
			tick.quantity = std::stod(cols[2]);
			tick.timestamp_ms = std::stoll(cols[4]);

			ticks.push_back(tick);

		} catch (const std::exception& e) {
			skipped++;
		}
	}

	std::cout << "Parsed " << ticks.size() << " ticks (" << skipped << " rows skipped)" << std::endl;

	return ticks;
}
