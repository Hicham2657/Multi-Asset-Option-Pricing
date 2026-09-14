#pragma once
#include <pnl/pnl_random.h>
#include <pnl/pnl_matvect.h>
#include <cstddef>
#include "pricing_results.hpp"   // PriceAndDeltas() renvoie un PricingResults par valeur

class BlackScholesModel;
class Option;

struct PriceAndStdDev { double price; double std_dev; };

class MonteCarlo{
    private:
        BlackScholesModel& _model;
        const Option& _option;
        std::size_t _num_iterations;
        PnlRng* _rng;

        PnlMat* _path;
        PnlMat* _shifted;
        PnlVect* _sum;
        PnlVect* _sum_sq;

    public:
        MonteCarlo(BlackScholesModel& model, const Option& option, std::size_t num_iterations);
        ~MonteCarlo();

        MonteCarlo(const MonteCarlo&) = delete;
        MonteCarlo& operator=(const MonteCarlo&) = delete;

        PriceAndStdDev price(const PnlMat* past, double t);
        void delta(const PnlMat* past,double t,double fdstep, PnlVect* deltas, PnlVect* delta_std_dev);
        PricingResults PriceAndDeltas(const PnlMat* past, double t, double fd_step);

};  