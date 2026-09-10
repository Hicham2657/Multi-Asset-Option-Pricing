#include "MonteCarlo.hpp"
#include "BlackScholesModel.hpp"
#include "Option.hpp"
#include <cmath>
#include "utils.hpp"
#include "pricing_results.hpp"

MonteCarlo::MonteCarlo(BlackScholesModel& model, const Option& option, std::size_t num_iterations)
    : _model(model), _option(option), _num_iterations(num_iterations){
        _path = pnl_mat_create(_option.GetNumSteps() + 1, _model.getD());
        _shifted = pnl_mat_create(_option.GetNumSteps() + 1, _model.getD());
        _sum = pnl_vect_create(_model.getD());
        _sum_sq = pnl_vect_create(_model.getD());

        _rng = pnl_rng_create(PNL_RNG_MERSENNE);
        pnl_rng_sseed(_rng, 0);
    }

MonteCarlo::~MonteCarlo(){
    pnl_rng_free(&_rng);
    pnl_mat_free(&_path);
    pnl_mat_free(&_shifted);
    pnl_vect_free(&_sum);
    pnl_vect_free(&_sum_sq);
}

PriceAndStdDev MonteCarlo::price(const PnlMat* past, double t){
    double sum = 0.0;
    double s_sum = 0.0;
    double T = _model.getTimeHorizon();
    double M = static_cast<double>(_num_iterations);
    double discount_factor = std::exp(-_model.getRiskFreeRate()*(T-t));
    
    for (std::size_t i =0;  i < _num_iterations; ++i){
        _model.asset(past, t, _path, _rng);
        const double payoff = _option.ComputePayoff(_path);
        sum += payoff;
        s_sum += payoff*payoff;
    }

    PriceAndStdDev result;
    result.price = discount_factor * sum/M;
    double variance =  discount_factor * discount_factor * (s_sum/ M - (sum/M)*(sum/M));
    result.std_dev = std::sqrt(variance / M);
    return result;
}

void MonteCarlo::delta(const PnlMat* past, double t, double fd_step, PnlVect* deltas, PnlVect* delta_std_dev){
    const int D = _model.getD();
    const double M = static_cast<double>(_num_iterations);
    const double T = _model.getTimeHorizon();

    pnl_vect_resize(deltas, D);
    pnl_vect_resize(delta_std_dev, D);
    pnl_vect_set_zero(deltas);
    pnl_vect_set_zero(delta_std_dev);

    PnlMat* path_up = pnl_mat_new();
    PnlMat* path_down = pnl_mat_new();

    for (std::size_t j = 0; j < _num_iterations; ++j) {
        _model.asset(past, t, _path, _rng);

        for (int d = 0; d < D; ++d) {
            pnl_mat_clone(path_up, _path);
            pnl_mat_clone(path_down, _path);
            _model.shift_asset(t, path_up, path_down, fd_step, d);

            const double diff = _option.ComputePayoff(path_up) - _option.ComputePayoff(path_down);
            LET(deltas, d)        += diff;
            LET(delta_std_dev, d) += diff * diff;
        }
    }

    const double discount = std::exp(-_model.getRiskFreeRate() * (T - t));
    for (int d = 0; d < D; ++d) {
        const double coeff = discount / (2.0 * fd_step * MGET(past, past->m - 1, d));
        const double mean = GET(deltas, d) / M;
        const double var = GET(delta_std_dev, d) / M - mean * mean;
        LET(deltas, d) = coeff * mean;
        LET(delta_std_dev, d) = std::abs(coeff) * std::sqrt(var / M);
    }
    pnl_mat_free(&path_up);
    pnl_mat_free(&path_down);
}


PricingResults MonteCarlo::PriceAndDeltas(const PnlMat* past, double t, double fd_step){

    PriceAndStdDev res = price(past, t);
    PnlVect* deltas = pnl_vect_new();
    PnlVect* deltasStdDev = pnl_vect_new();

    delta(past, t, fd_step, deltas, deltasStdDev);
    
    return PricingResults(res.price, res.std_dev, deltas, deltasStdDev); 

}