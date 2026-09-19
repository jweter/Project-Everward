#include <gtest/gtest.h>
#include <vector>
#include "src/simulation/include/everward/simulation/utils.hpp"

namespace everward::simulation {

TEST(UtilsTest, CalculateAverage) {
    std::vector<double> numbers = {1.0, 2.0, 3.0, 4.0, 5.0};
    double average = calculate_average(numbers);
    EXPECT_DOUBLE_EQ(average, 3.0);
}

TEST(UtilsTest, CalculateAverageEmpty) {
    std::vector<double> numbers = {};
    double average = calculate_average(numbers);
    EXPECT_DOUBLE_EQ(average, 0.0);
}

}  // namespace everward::simulation
