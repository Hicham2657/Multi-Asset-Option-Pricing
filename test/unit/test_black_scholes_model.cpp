#include "BlackScholesModel.hpp"

#include "pnl_test_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
double roundoffTolerance(double expected)
{
    return 32.0 * std::numeric_limits<double>::epsilon()
           * std::max(1.0, std::abs(expected));
}
}

TEST(BlackScholesModelTest, StoresParametersAndCopiesVolatilityDimension)
{
    auto volatilities = makeVector({0.2, 0.3});
    BlackScholesModel model{0.05, volatilities.get(), 2.0, 0.25};

    pnl_vect_resize_from_scalar(volatilities.get(), 3, 0.9);

    EXPECT_EQ(model.getD(), 2);
    EXPECT_DOUBLE_EQ(model.getRiskFreeRate(), 0.05);
    EXPECT_DOUBLE_EQ(model.getTimeHorizon(), 2.0);
}

TEST(BlackScholesModelTest, RejectsSingularAndIndefiniteCorrelations)
{
    auto volatilities = makeVector({0.2, 0.3});

    EXPECT_THROW((BlackScholesModel{0.05, volatilities.get(), 1.0, 1.0}),
                 std::invalid_argument);
    EXPECT_THROW((BlackScholesModel{0.05, volatilities.get(), 1.0, -1.0}),
                 std::invalid_argument);
    EXPECT_THROW((BlackScholesModel{0.05, volatilities.get(), 1.0, 1.01}),
                 std::invalid_argument);
    EXPECT_THROW((BlackScholesModel{0.05, volatilities.get(), 1.0, -1.01}),
                 std::invalid_argument);
}

TEST(BlackScholesModelTest, BuildsKnownDeterministicPathFromNonGridTime)
{
    constexpr double rate = 0.10;
    constexpr double current_time = 0.35;
    constexpr double current_spot = 103.0;
    auto volatilities = makeVector({0.0});
    auto past = makeMatrix(3, 1, {100.0, 102.0, current_spot});
    PnlMatPtr path{pnl_mat_create_from_scalar(5, 1, -1.0)};
    auto rng = makeRng(17);
    BlackScholesModel model{rate, volatilities.get(), 1.0, 0.0};

    model.asset(past.get(), current_time, path.get(), rng.get());

    EXPECT_DOUBLE_EQ(pnl_mat_get(path.get(), 0, 0), 100.0);
    EXPECT_DOUBLE_EQ(pnl_mat_get(path.get(), 1, 0), 102.0);
    for (int fixing = 2; fixing <= 4; ++fixing)
    {
        const double fixing_time = fixing * 0.25;
        const double expected = current_spot * std::exp(rate * (fixing_time - current_time));
        EXPECT_NEAR(pnl_mat_get(path.get(), fixing, 0), expected,
                    roundoffTolerance(expected));
    }
}

TEST(BlackScholesModelTest, AtMaturityCopiesTheObservedPathWithoutSimulation)
{
    auto volatilities = makeVector({0.2, 0.3});
    auto past = makeMatrix(4, 2, {100.0, 90.0,
                                  101.0, 92.0,
                                  103.0, 91.0,
                                  105.0, 95.0});
    PnlMatPtr path{pnl_mat_create_from_scalar(4, 2, -1.0)};
    auto rng = makeRng(4);
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.4};

    model.asset(past.get(), 1.0, path.get(), rng.get());

    for (int row = 0; row < path->m; ++row)
    {
        for (int column = 0; column < path->n; ++column)
        {
            EXPECT_DOUBLE_EQ(pnl_mat_get(path.get(), row, column),
                             pnl_mat_get(past.get(), row, column));
        }
    }
}

TEST(BlackScholesModelTest, SimulatedPricesRemainFiniteAndStrictlyPositive)
{
    auto volatilities = makeVector({0.3, 0.5, 0.7});
    auto past = makeMatrix(1, 3, {100.0, 80.0, 120.0});
    PnlMatPtr path{pnl_mat_create(13, 3)};
    auto rng = makeRng(1234);
    BlackScholesModel model{0.02, volatilities.get(), 1.0, 0.2};

    model.asset(past.get(), 0.0, path.get(), rng.get());

    for (int row = 0; row < path->m; ++row)
    {
        for (int column = 0; column < path->n; ++column)
        {
            const double spot = pnl_mat_get(path.get(), row, column);
            EXPECT_TRUE(std::isfinite(spot));
            EXPECT_GT(spot, 0.0);
        }
    }
}

TEST(BlackScholesModelTest, ShiftChangesOnlyFutureRowsOfSelectedAsset)
{
    auto volatilities = makeVector({0.2, 0.3});
    auto original = makeMatrix(5, 2, {10.0, 20.0,
                                      11.0, 21.0,
                                      12.0, 22.0,
                                      13.0, 23.0,
                                      14.0, 24.0});
    PnlMatPtr upward{pnl_mat_copy(original.get())};
    PnlMatPtr downward{pnl_mat_copy(original.get())};
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.0};

    model.shift_asset(0.5, upward.get(), downward.get(), 0.1, 1);

    for (int row = 0; row <= 2; ++row)
    {
        EXPECT_DOUBLE_EQ(pnl_mat_get(upward.get(), row, 1),
                         pnl_mat_get(original.get(), row, 1));
        EXPECT_DOUBLE_EQ(pnl_mat_get(downward.get(), row, 1),
                         pnl_mat_get(original.get(), row, 1));
    }
    for (int row = 3; row <= 4; ++row)
    {
        EXPECT_NEAR(pnl_mat_get(upward.get(), row, 1),
                    1.1 * pnl_mat_get(original.get(), row, 1), 1e-12);
        EXPECT_NEAR(pnl_mat_get(downward.get(), row, 1),
                    0.9 * pnl_mat_get(original.get(), row, 1), 1e-12);
    }
    for (int row = 0; row <= 4; ++row)
    {
        EXPECT_DOUBLE_EQ(pnl_mat_get(upward.get(), row, 0),
                         pnl_mat_get(original.get(), row, 0));
        EXPECT_DOUBLE_EQ(pnl_mat_get(downward.get(), row, 0),
                         pnl_mat_get(original.get(), row, 0));
    }
}

TEST(BlackScholesModelTest, UnshiftRestoresTheOriginalPath)
{
    auto volatilities = makeVector({0.2});
    auto original = makeMatrix(4, 1, {100.0, 101.0, 102.0, 103.0});
    PnlMatPtr upward{pnl_mat_copy(original.get())};
    PnlMatPtr downward{pnl_mat_copy(original.get())};
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.0};

    model.shift_asset(0.25, upward.get(), downward.get(), 0.01, 0);
    model.unshift_asset(0.25, upward.get(), downward.get(), 0.01, 0);

    for (int row = 0; row < original->m; ++row)
    {
        const double expected = pnl_mat_get(original.get(), row, 0);
        EXPECT_NEAR(pnl_mat_get(upward.get(), row, 0), expected,
                    roundoffTolerance(expected));
        EXPECT_NEAR(pnl_mat_get(downward.get(), row, 0), expected,
                    roundoffTolerance(expected));
    }
}

TEST(BlackScholesModelTest, ShiftAtMaturityLeavesPathUnchanged)
{
    auto volatilities = makeVector({0.2});
    auto original = makeMatrix(3, 1, {100.0, 101.0, 102.0});
    PnlMatPtr upward{pnl_mat_copy(original.get())};
    PnlMatPtr downward{pnl_mat_copy(original.get())};
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.0};

    model.shift_asset(1.0, upward.get(), downward.get(), 0.1, 0);

    for (int row = 0; row < original->m; ++row)
    {
        EXPECT_DOUBLE_EQ(pnl_mat_get(upward.get(), row, 0),
                         pnl_mat_get(original.get(), row, 0));
        EXPECT_DOUBLE_EQ(pnl_mat_get(downward.get(), row, 0),
                         pnl_mat_get(original.get(), row, 0));
    }
}
