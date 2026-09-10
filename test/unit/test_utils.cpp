#include "utils.hpp"

#include <gtest/gtest.h>

#include <cmath>

TEST(ComputeLastIndexTest, ReturnsGridPointIndex)
{
    EXPECT_EQ(compute_last_index(0.0, 1.0, 100), 0);
    EXPECT_EQ(compute_last_index(0.5, 1.0, 100), 50);
    EXPECT_EQ(compute_last_index(1.0, 1.0, 100), 100);
}

TEST(ComputeLastIndexTest, ReturnsPreviousIndexBetweenGridPoints)
{
    EXPECT_EQ(compute_last_index(0.234, 1.0, 10), 2);
    EXPECT_EQ(compute_last_index(0.999, 1.0, 10), 9);
}

TEST(ComputeLastIndexTest, SnapsOnlyInsideFloatingPointTolerance)
{
    const double lower_boundary = 0.5 - EPS;
    const double just_inside = std::nextafter(lower_boundary, 0.5);
    const double just_outside = std::nextafter(lower_boundary, 0.0);

    EXPECT_EQ(compute_last_index(just_inside, 1.0, 100), 50);
    EXPECT_EQ(compute_last_index(just_outside, 1.0, 100), 49);
}

TEST(ComputeLastIndexTest, IsMonotoneOverValidTimes)
{
    int previous = compute_last_index(0.0, 2.0, 24);
    for (int sample = 1; sample <= 1000; ++sample)
    {
        const double time = 2.0 * sample / 1000.0;
        const int current = compute_last_index(time, 2.0, 24);
        EXPECT_GE(current, previous);
        EXPECT_GE(current, 0);
        EXPECT_LE(current, 24);
        previous = current;
    }
}
