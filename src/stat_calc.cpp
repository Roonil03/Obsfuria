#include "stat_calc.hpp"
#include <random>
#include <algorithm>

int StatCalc::generateStat(int median, int minVal, int maxVal) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(-50, 50);

    int result = median + dist(gen);
    return std::clamp(result, minVal, maxVal);
}
