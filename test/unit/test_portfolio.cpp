#include "Basket.hpp"
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "json_helper.hpp"
#include "portfolio.hpp"

#include "pnl_test_utils.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(PositionTest, SerializesEveryPublicValue)
{
    auto deltas = makeVector({0.4, 0.6});
    auto delta_std_devs = makeVector({0.01, 0.02});
    Position position{7, 14.5, 0.1, deltas.get(), delta_std_devs.get(), 1014.5};

    const nlohmann::json json = position;

    EXPECT_EQ(json.at("date").get<int>(), 7);
    EXPECT_DOUBLE_EQ(json.at("value").get<double>(), 1014.5);
    EXPECT_DOUBLE_EQ(json.at("price").get<double>(), 14.5);
    EXPECT_DOUBLE_EQ(json.at("priceStdDev").get<double>(), 0.1);
    EXPECT_EQ(json.at("deltas"), nlohmann::json({0.4, 0.6}));
    EXPECT_EQ(json.at("deltasStdDev"), nlohmann::json({0.01, 0.02}));
}

TEST(PositionTest, PrintWritesValidJson)
{
    auto deltas = makeVector({0.5});
    auto delta_std_devs = makeVector({0.03});
    Position position{2, 9.0, 0.2, deltas.get(), delta_std_devs.get(), 500.0};
    testing::internal::CaptureStdout();

    position.print();

    const std::string output = testing::internal::GetCapturedStdout();
    const nlohmann::json json = nlohmann::json::parse(output);
    EXPECT_EQ(json.at("date").get<int>(), 2);
    EXPECT_DOUBLE_EQ(json.at("value").get<double>(), 500.0);
}

TEST(PortfolioTest, StartsEmptyAndKeepsMonteCarloReference)
{
    auto weights = makeVector({1.0});
    auto volatilities = makeVector({0.2});
    Basket option{weights.get(), 1, 100.0};
    BlackScholesModel model{0.05, volatilities.get(), 1.0, 0.0};
    MonteCarlo engine{model, option, 10};
    nlohmann::json parameters = nlohmann::json::object();

    Portfolio portfolio{parameters, engine};

    EXPECT_EQ(&portfolio.monteCarlo, &engine);
    EXPECT_TRUE(portfolio.positions.empty());
}
