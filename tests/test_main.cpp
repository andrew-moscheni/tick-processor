#include <gtest/gtest.h>
#include "tick.h"
#include <fstream>

// Make temp file to ensure parsing is correct
TEST(ParserTest, ParsesValidRows) {
	std::ofstream tmp("/tmp/test_ticks.csv");
	tmp << "1,42580.01,0.00500000,212.90,1705276800017,true,true\n";
	tmp << "2,42600.00,0.01200000,511.20,1705276800035,false,true\n";
	tmp << "3,42550.50,0.00800000,340.40,1705276800051,true,false\n";
	tmp.close();

	auto ticks = parse_csv("/tmp/test_ticks.csv");

	EXPECT_EQ(ticks.size(), 3);
	EXPECT_DOUBLE_EQ(ticks[0].price, 42580.01);
	EXPECT_DOUBLE_EQ(ticks[1].quantity, 0.012);
	EXPECT_EQ(ticks[2].timestamp_ms, 1705276800051);
}

TEST(ParserTest, SkipsMalformedRows) {
	std::ofstream tmp("/tmp/test_bad.csv");
	tmp << "1,42580.01,0.005,212.90,1705276800017,true,true\n";
	tmp << "this,is,bad\n";
	tmp << "3,42600.00,0.012,511.20,170527680035,false,true\n";
	tmp.close();

	auto ticks = parse_csv("/tmp/test_bad.csv");

	EXPECT_EQ(ticks.size(), 2);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
