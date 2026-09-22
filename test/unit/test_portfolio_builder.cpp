#include "Basket.hpp"
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "PortfolioBuilder.hpp"

#include "pnl_test_utils.hpp"

#include <gtest/gtest.h>

TEST(PortfolioBuilderTest, PortfolioValueIsScalarProductOfPricesAndDeltas)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({0.2});
    Basket call{weights.get(), 1, 100.0};
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.0};
    MonteCarlo engine{model, call, 4};
    PortfolioBuilder builder{&engine};

    auto path = makeMatrix(1, 2, {100.0, 50.0});
    auto deltas = makeVector({0.5, 2.0});

    EXPECT_DOUBLE_EQ(builder.PortfolioValue(path.get(), deltas.get(), 0),
                     100.0 * 0.5 + 50.0 * 2.0);
}

TEST(PortfolioBuilderTest, StartsWithoutPositions)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({0.2});
    Basket call{weights.get(), 1, 100.0};
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.0};
    MonteCarlo engine{model, call, 4};
    PortfolioBuilder builder{&engine};

    EXPECT_TRUE(builder.getPositions().empty());
}

// Volatilité nulle : le modèle et les deltas sont déterministes, comme dans
// MonteCarloTest.PriceAndDeltasDeltaIsDeterministicAndEqualsWeightsForZeroVolatility.
// (Ce test a démasqué un bug de pnl_mat_create(0, n) dans buildPortfolio(),
// corrigé depuis — voir l'historique git de PortfolioBuilder.cpp.)
TEST(PortfolioBuilderTest, BuildPortfolioProducesOnePositionPerHedgingDate)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({0.0});
    Basket call{weights.get(), 1, 90.0};   // strike profond dans la monnaie à S=100
    BlackScholesModel model{0.0, volatilities.get(), 1.0, 0.0};
    MonteCarlo engine{model, call, 4};
    PortfolioBuilder builder{&engine};

    // H = 2 pas de rebalancement -> 3 lignes de marché (S_t0, S_t1, S_t2).
    // fixingDatesNb = 1 (comme le "1" passé à Basket ci-dessus) : une seule
    // date de constatation, à maturité.
    auto marketPath = makeMatrix(3, 1, {100.0, 100.0, 100.0});

    builder.buildPortfolio(marketPath.get(), 0.01, 2, 0.0, 1.0, 1);

    const auto& positions = builder.getPositions();
    ASSERT_EQ(positions.size(), 3u);

    auto it = positions.begin();
    EXPECT_EQ(it->date, 0);
    ++it;
    EXPECT_EQ(it->date, 1);
    ++it;
    EXPECT_EQ(it->date, 2);

    // Call profondément dans la monnaie, volatilité nulle -> delta = 1 (le
    // payoff est toujours exercé, la sensibilité au spot vaut 1) sur toutes
    // les positions.
    for (const auto& position : positions)
    {
        ASSERT_EQ(position.deltas->size, 1);
        EXPECT_NEAR(pnl_vect_get(position.deltas, 0), 1.0, 1e-9);
    }
}

// Vérifie que buildPortfolio() interroge la BONNE ligne du chemin de marché
// pour la date de constatation de l'option (ici une seule, à maturité), même
// quand la grille de REBALANCEMENT (H) est plus fine. Avant correction, à la
// date de rebalancement H=4 (= la maturité), le prix était calculé à partir
// de la 2e ligne du chemin de marché (l'observation du 1er pas de
// rebalancement, ici 100) au lieu de la dernière (la vraie valeur à
// maturité, ici 130) — cf. le diagnostic dans PortfolioBuilder.cpp.
TEST(PortfolioBuilderTest, BuildPortfolioUsesTheTrueFixingValueNotTheEarlyHedgingRow)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({0.0});
    Basket call{weights.get(), 1, 100.0};   // une seule constatation, à maturité
    BlackScholesModel model{0.0, volatilities.get(), 1.0, 0.0};
    MonteCarlo engine{model, call, 4};
    PortfolioBuilder builder{&engine};

    // H = 4 pas de rebalancement (5 lignes), mais 1 seule date de
    // constatation : le marché est plat jusqu'à la toute dernière ligne, où
    // il saute à 130 (la vraie valeur à maturité).
    auto marketPath = makeMatrix(5, 1, {100.0, 100.0, 100.0, 100.0, 130.0});

    builder.buildPortfolio(marketPath.get(), 0.01, 4, 0.0, 1.0, 1);

    const auto& positions = builder.getPositions();
    ASSERT_EQ(positions.size(), 5u);

    const auto& last = positions.back();
    EXPECT_EQ(last.date, 4);
    // prix à maturité = payoff = max(S_T - K, 0) = max(130 - 100, 0) = 30
    EXPECT_NEAR(last.price, 30.0, 1e-9);
}
