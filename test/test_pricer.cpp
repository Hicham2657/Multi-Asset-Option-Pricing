#include "Option.hpp"
#include "Basket.hpp"
#include "AsianOption.hpp"
#include "MonteCarlo.hpp"
#include "BlackScholesModel.hpp"

#include <pnl/pnl_finance.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    constexpr double SPOT = 100.0;
    constexpr double STRIKE = 100.0;
    constexpr double MATURITY = 1.0;
    constexpr double RATE = 0.05;
    constexpr double VOLATILITY = 0.2;
    constexpr std::size_t SAMPLES = 1000000;

    int failures = 0;

    /// Check that a Monte Carlo estimate is within its 95% confidence interval
    /// of the reference value.
    void check(const std::string& name, double estimate, double reference, double std_dev)
    {
        const bool ok = std::abs(estimate - reference) < 1.96 * std_dev;
        if (!ok) ++failures;
        std::cout << name << " : " << estimate
                  << " +/- " << 1.96 * std_dev
                  << (ok ? "  OK" : "  FAILED")
                  << "\n";
    }

    void check_true(const std::string& name, bool condition)
    {
        if (!condition) ++failures;
        std::cout << name << (condition ? " : OK" : " : FAILED") << "\n";
    }

    /// A basket with a single asset and unit weight degenerates into a vanilla
    /// call, so the closed-form Black-Scholes price is the reference.
    void test_vanilla_call(double reference)
    {
        PnlVect* weights = pnl_vect_create_from_scalar(1, 1.0);
        PnlVect* volatilities = pnl_vect_create_from_scalar(1, VOLATILITY);
        PnlMat* past = pnl_mat_create_from_scalar(1, 1, SPOT);

        Basket call{weights, 1, STRIKE};
        BlackScholesModel model{RATE, volatilities, MATURITY, 0.0};
        MonteCarlo engine{model, call, SAMPLES};

        for (int run = 1; run <= 5; ++run)
        {
            const PriceAndStdDev result = engine.price(past, 0.0);
            check("vanilla call, run " + std::to_string(run),
                  result.price, reference, result.std_dev);
        }

        pnl_mat_free(&past);
        pnl_vect_free(&weights);
        pnl_vect_free(&volatilities);
    }

    /// Two perfectly correlated identical assets with weights 1/2 behave like a
    /// single asset. This is the only test that exercises the Cholesky factor:
    /// with one asset the correlation matrix is the identity.
    void test_cholesky(double reference)
    {
        PnlVect* weights = pnl_vect_create_from_scalar(2, 0.5);
        PnlVect* volatilities = pnl_vect_create_from_scalar(2, VOLATILITY);
        PnlMat* past = pnl_mat_create_from_scalar(1, 2, SPOT);

        Basket basket{weights, 1, STRIKE};
        BlackScholesModel model{RATE, volatilities, MATURITY, 0.999999};
        MonteCarlo engine{model, basket, SAMPLES};

        const PriceAndStdDev result = engine.price(past, 0.0);
        check("two correlated assets", result.price, reference, result.std_dev);

        pnl_mat_free(&past);
        pnl_vect_free(&weights);
        pnl_vect_free(&volatilities);
    }

    /// A basket payoff only reads the terminal spot, so refining the fixing
    /// grid must leave the price unchanged. This catches errors in the time
    /// step or in the drift term, which the single-step case hides.
    void test_time_step_invariance(double reference)
    {
        PnlVect* weights = pnl_vect_create_from_scalar(1, 1.0);
        PnlVect* volatilities = pnl_vect_create_from_scalar(1, VOLATILITY);
        PnlMat* past = pnl_mat_create_from_scalar(1, 1, SPOT);

        BlackScholesModel model{RATE, volatilities, MATURITY, 0.0};

        for (std::size_t steps : {1u, 12u, 365u})
        {
            Basket basket{weights, steps, STRIKE};
            MonteCarlo engine{model, basket, SAMPLES};
            const PriceAndStdDev result = engine.price(past, 0.0);
            check("basket with N = " + std::to_string(steps),
                  result.price, reference, result.std_dev);
        }

        pnl_mat_free(&past);
        pnl_vect_free(&weights);
        pnl_vect_free(&volatilities);
    }

    /// The correlation matrix is only positive definite for
    /// -1 / (D - 1) < rho < 1, and the model must reject anything else.
    void test_invalid_correlation()
    {
        PnlVect* volatilities = pnl_vect_create_from_scalar(2, VOLATILITY);
        bool threw = false;

        try
        {
            BlackScholesModel invalid{RATE, volatilities, MATURITY, -1.5};
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }

        check_true("invalid correlation is rejected", threw);
        pnl_vect_free(&volatilities);
    }
}

int main()
{
    const double reference = pnl_bs_call(SPOT, STRIKE, MATURITY, RATE, 0.0, VOLATILITY);
    std::cout << "Black-Scholes reference price : " << reference << "\n\n";

    test_vanilla_call(reference);
    std::cout << "\n";
    test_cholesky(reference);
    test_time_step_invariance(reference);
    std::cout << "\n";
    test_invalid_correlation();

    std::cout << "\n" << (failures == 0 ? "All tests passed" : "Some tests failed") << "\n";
    return failures == 0 ? 0 : 1;
}