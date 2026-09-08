#pragma once
#include <pnl/pnl_random.h>
#include <pnl/pnl_matvect.h>
#include <cstddef>

class BlackScholesModel;
class Option;

struct PriceAndCI
{
    double price;
    double ci;
};

class MonteCarlo{
    private:
        BlackScholesModel& _model;
        const Option& _option;
        std::size_t _num_iterations;
        PnlRng* _rng;
        PnlMat* _path;
    public:
        MonteCarlo(BlackScholesModel& model, const Option& option, std::size_t num_iterations);
        ~MonteCarlo();
        PriceAndCI price(const PnlMat* past, double t);
};