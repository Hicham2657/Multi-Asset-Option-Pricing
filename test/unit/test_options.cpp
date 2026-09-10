#include "AsianOption.hpp"
#include "Basket.hpp"

#include "pnl_test_utils.hpp"

#include <gtest/gtest.h>

namespace
{
constexpr double ARITHMETIC_TOLERANCE = 1e-12;
}

TEST(OptionMetadataTest, ConstructorCopiesWeightsAndStoresNumberOfSteps)
{
    auto weights = makeVector({0.25, 0.75});
    Basket option{weights.get(), 12, 100.0};

    pnl_vect_set(weights.get(), 0, 10.0);

    ASSERT_EQ(option.GetWeights()->size, 2);
    EXPECT_NEAR(pnl_vect_get(option.GetWeights(), 0), 0.25, ARITHMETIC_TOLERANCE);
    EXPECT_NEAR(pnl_vect_get(option.GetWeights(), 1), 0.75, ARITHMETIC_TOLERANCE);
    EXPECT_EQ(option.GetNumSteps(), 12u);
}

TEST(BasketOptionTest, ComputesWeightedTerminalPayoffAboveStrike)
{
    auto weights = makeVector({0.25, 0.75});
    auto path = makeMatrix(2, 2, {90.0, 110.0,
                                  120.0, 140.0});
    Basket option{weights.get(), 1, 125.0};

    const double payoff = option.ComputePayoff(path.get());

    EXPECT_NEAR(payoff, 10.0, ARITHMETIC_TOLERANCE);
}

TEST(BasketOptionTest, PayoffIsZeroBelowStrike)
{
    auto weights = makeVector({0.5, 0.5});
    auto path = makeMatrix(1, 2, {80.0, 100.0});
    Basket option{weights.get(), 1, 100.0};

    EXPECT_NEAR(option.ComputePayoff(path.get()), 0.0, ARITHMETIC_TOLERANCE);
}

TEST(BasketOptionTest, PayoffIsZeroAtTheMoney)
{
    auto weights = makeVector({0.4, 0.6});
    auto path = makeMatrix(1, 2, {70.0, 120.0});
    Basket option{weights.get(), 1, 100.0};

    EXPECT_NEAR(option.ComputePayoff(path.get()), 0.0, ARITHMETIC_TOLERANCE);
}

TEST(BasketOptionTest, EarlierFixingsDoNotAffectPayoff)
{
    auto weights = makeVector({0.5, 0.5});
    auto first_path = makeMatrix(3, 2, {50.0, 60.0,
                                        70.0, 80.0,
                                        120.0, 140.0});
    auto second_path = makeMatrix(3, 2, {500.0, 600.0,
                                         700.0, 800.0,
                                         120.0, 140.0});
    Basket option{weights.get(), 2, 100.0};

    EXPECT_NEAR(option.ComputePayoff(first_path.get()),
                option.ComputePayoff(second_path.get()), ARITHMETIC_TOLERANCE);
}

TEST(BasketOptionTest, SupportsSingleAssetAndSingleObservation)
{
    auto weights = makeVector({1.0});
    auto path = makeMatrix(1, 1, {125.0});
    Basket option{weights.get(), 0, 100.0};

    EXPECT_NEAR(option.ComputePayoff(path.get()), 25.0, ARITHMETIC_TOLERANCE);
}

TEST(AsianOptionTest, ComputesWeightedArithmeticAveragePayoff)
{
    auto weights = makeVector({0.25, 0.75});
    auto path = makeMatrix(3, 2, {100.0, 80.0,
                                  120.0, 100.0,
                                  140.0, 120.0});
    AsianOption option{weights.get(), 2, 100.0};

    const double payoff = option.ComputePayoff(path.get());

    EXPECT_NEAR(payoff, 5.0, ARITHMETIC_TOLERANCE);
}

TEST(AsianOptionTest, PayoffIsZeroBelowStrike)
{
    auto weights = makeVector({1.0});
    auto path = makeMatrix(3, 1, {80.0, 90.0, 100.0});
    AsianOption option{weights.get(), 2, 100.0};

    EXPECT_NEAR(option.ComputePayoff(path.get()), 0.0, ARITHMETIC_TOLERANCE);
}

TEST(AsianOptionTest, PayoffIsZeroAtTheMoney)
{
    auto weights = makeVector({0.5, 0.5});
    auto path = makeMatrix(2, 2, {80.0, 120.0,
                                  120.0, 80.0});
    AsianOption option{weights.get(), 1, 100.0};

    EXPECT_NEAR(option.ComputePayoff(path.get()), 0.0, ARITHMETIC_TOLERANCE);
}

TEST(AsianOptionTest, IsInvariantUnderPermutationOfObservationTimes)
{
    auto weights = makeVector({0.3, 0.7});
    auto first_path = makeMatrix(3, 2, {90.0, 110.0,
                                        120.0, 80.0,
                                        150.0, 140.0});
    auto permuted_path = makeMatrix(3, 2, {150.0, 140.0,
                                           90.0, 110.0,
                                           120.0, 80.0});
    AsianOption option{weights.get(), 2, 90.0};

    EXPECT_NEAR(option.ComputePayoff(first_path.get()),
                option.ComputePayoff(permuted_path.get()), ARITHMETIC_TOLERANCE);
}

TEST(AsianOptionTest, SupportsAZeroStrikeAtOneObservation)
{
    auto weights = makeVector({0.2, 0.8});
    auto path = makeMatrix(1, 2, {50.0, 100.0});
    AsianOption option{weights.get(), 0, 0.0};

    EXPECT_NEAR(option.ComputePayoff(path.get()), 90.0, ARITHMETIC_TOLERANCE);
}
