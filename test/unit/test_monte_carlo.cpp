#include "Basket.hpp"
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"

#include "pnl_test_utils.hpp"

#include <gtest/gtest.h>
#include <pnl/pnl_finance.h>

#include <cmath>

namespace
{
constexpr double SPOT = 100.0;
constexpr double STRIKE = 100.0;
constexpr double MATURITY = 1.0;
constexpr double RATE = 0.05;
constexpr double VOLATILITY = 0.2;
}

TEST(MonteCarloTest, VanillaCallAgreesWithBlackScholesWithinFourStandardErrors)
{
    constexpr std::size_t samples = 200000;
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({VOLATILITY});
    auto past = makeMatrix(1, 1, {SPOT});
    Basket call{weights.get(), 1, STRIKE};
    BlackScholesModel model{RATE, volatilities.get(), MATURITY, 0.0};
    MonteCarlo engine{model, call, samples};
    const double reference = pnl_bs_call(
        SPOT, STRIKE, MATURITY, RATE, 0.0, VOLATILITY);

    const PriceAndStdDev result = engine.price(past.get(), 0.0);

    EXPECT_GT(result.std_dev, 0.0);
    EXPECT_LE(std::abs(result.price - reference), 4.0 * result.std_dev)
        << "The tolerance is four reported Monte Carlo standard errors.";
}

TEST(MonteCarloTest, PriceAtMaturityEqualsKnownPayoffWithZeroUncertainty)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({VOLATILITY});
    auto past = makeMatrix(2, 1, {SPOT, 120.0});
    Basket call{weights.get(), 1, STRIKE};
    BlackScholesModel model{RATE, volatilities.get(), MATURITY, 0.0};
    MonteCarlo engine{model, call, 16};

    const PriceAndStdDev result = engine.price(past.get(), MATURITY);

    EXPECT_DOUBLE_EQ(result.price, 20.0);
    EXPECT_DOUBLE_EQ(result.std_dev, 0.0);
}

TEST(MonteCarloTest, FreshEnginesReproduceResultsFromTheFixedSeed)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({VOLATILITY});
    auto past = makeMatrix(1, 1, {SPOT});
    Basket call{weights.get(), 4, STRIKE};
    BlackScholesModel first_model{RATE, volatilities.get(), MATURITY, 0.0};
    BlackScholesModel second_model{RATE, volatilities.get(), MATURITY, 0.0};
    MonteCarlo first_engine{first_model, call, 20000};
    MonteCarlo second_engine{second_model, call, 20000};

    const PriceAndStdDev first = first_engine.price(past.get(), 0.0);
    const PriceAndStdDev second = second_engine.price(past.get(), 0.0);

    EXPECT_DOUBLE_EQ(first.price, second.price);
    EXPECT_DOUBLE_EQ(first.std_dev, second.std_dev);
}

TEST(MonteCarloTest, CallPriceIsNonnegativeAndDecreasesWithStrike)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({VOLATILITY});
    auto past = makeMatrix(1, 1, {SPOT});
    Basket low_strike_call{weights.get(), 1, 90.0};
    Basket high_strike_call{weights.get(), 1, 110.0};
    BlackScholesModel low_strike_model{RATE, volatilities.get(), MATURITY, 0.0};
    BlackScholesModel high_strike_model{RATE, volatilities.get(), MATURITY, 0.0};
    MonteCarlo low_strike_engine{low_strike_model, low_strike_call, 50000};
    MonteCarlo high_strike_engine{high_strike_model, high_strike_call, 50000};

    const PriceAndStdDev low_strike = low_strike_engine.price(past.get(), 0.0);
    const PriceAndStdDev high_strike = high_strike_engine.price(past.get(), 0.0);

    EXPECT_GE(low_strike.price, 0.0);
    EXPECT_GE(high_strike.price, 0.0);
    EXPECT_GE(low_strike.price, high_strike.price);
}

TEST(MonteCarloTest, DeterministicInTheMoneyBasketDeltaEqualsItsWeights)
{
    auto weights = makeVector({0.25, 0.75});
    auto volatilities = makeVector({0.0, 0.0});
    auto past = makeMatrix(1, 2, {100.0, 100.0});
    Basket call{weights.get(), 1, 50.0};
    BlackScholesModel model{0.0, volatilities.get(), MATURITY, 0.25};
    MonteCarlo engine{model, call, 32};
    PnlVectPtr deltas{pnl_vect_new()};
    PnlVectPtr deltaStdDev{pnl_vect_new()};

    engine.delta(past.get(), 0.0, 0.01, deltas.get(), deltaStdDev.get());

    ASSERT_EQ(deltas->size, 2);
    EXPECT_NEAR(pnl_vect_get(deltas.get(), 0), 0.25, 1e-12);
    EXPECT_NEAR(pnl_vect_get(deltas.get(), 1), 0.75, 1e-12);
    // Le payoff est déterministe (volatilités nulles) : l'estimateur du delta
    // n'a aucune variance.
    ASSERT_EQ(deltaStdDev->size, 2);
    EXPECT_NEAR(pnl_vect_get(deltaStdDev.get(), 0), 0.0, 1e-12);
    EXPECT_NEAR(pnl_vect_get(deltaStdDev.get(), 1), 0.0, 1e-12);
}

TEST(MonteCarloTest, PriceAndDeltasAgreesWithSeparatePriceAndDeltaCalls)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({VOLATILITY});
    auto past = makeMatrix(1, 1, {SPOT});
    Basket call{weights.get(), 1, STRIKE};
    BlackScholesModel model{RATE, volatilities.get(), MATURITY, 0.0};
    MonteCarlo engine{model, call, 5000};

    // MonteCarlo est réinitialisé à la même graine (0) à chaque construction,
    // donc deux moteurs frais consomment le même flux de tirages : appeler
    // price() puis delta() sur l'un doit donner exactement ce que renvoie
    // PriceAndDeltas() sur l'autre.
    const PriceAndStdDev separate_price = engine.price(past.get(), 0.0);
    PnlVectPtr deltas{pnl_vect_new()};
    PnlVectPtr deltaStdDev{pnl_vect_new()};
    engine.delta(past.get(), 0.0, 0.1, deltas.get(), deltaStdDev.get());

    BlackScholesModel combined_model{RATE, volatilities.get(), MATURITY, 0.0};
    MonteCarlo combined_engine{combined_model, call, 5000};
    const PricingResults combined =
        combined_engine.PriceAndDeltas(past.get(), 0.0, 0.1);

    EXPECT_DOUBLE_EQ(combined.price, separate_price.price);
    EXPECT_DOUBLE_EQ(combined.priceStdDev, separate_price.std_dev);
    ASSERT_EQ(combined.delta->size, deltas->size);
    for (int i = 0; i < deltas->size; ++i)
    {
        EXPECT_DOUBLE_EQ(pnl_vect_get(combined.delta, i),
                         pnl_vect_get(deltas.get(), i));
        EXPECT_DOUBLE_EQ(pnl_vect_get(combined.deltaStdDev, i),
                         pnl_vect_get(deltaStdDev.get(), i));
    }

    // PriceAndDeltas() alloue delta/deltaStdDev en interne (pnl_vect_new())
    // et PricingResults ne les possède pas : c'est à l'appelant de les
    // libérer, comme dans price0.cpp.
    PnlVect* combined_delta = const_cast<PnlVect*>(combined.delta);
    PnlVect* combined_delta_std_dev = const_cast<PnlVect*>(combined.deltaStdDev);
    pnl_vect_free(&combined_delta);
    pnl_vect_free(&combined_delta_std_dev);
}
