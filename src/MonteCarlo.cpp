#include "MonteCarlo.hpp"
#include "BlackScholesModel.hpp"
#include "Option.hpp"
#include <cmath>
#include "utils.hpp"

MonteCarlo::MonteCarlo(BlackScholesModel& model, const Option& option, std::size_t num_iterations)
    : _model(model), _option(option), _num_iterations(num_iterations){
        _path = pnl_mat_create(_option.GetNumSteps() + 1, _model.getD());
        _rng = pnl_rng_create(PNL_RNG_MERSENNE);
        pnl_rng_sseed(_rng, 0);
    }

MonteCarlo::~MonteCarlo(){  
    pnl_rng_free(&_rng);
    pnl_mat_free(&_path);
}

PriceAndStdDev MonteCarlo::price(const PnlMat* past, double t){
    double sum = 0.0;
    double s_sum = 0.0;
    double T = _model.getTimeHorizon();
    double M = static_cast<double>(_num_iterations);
    double discount_factor = std::exp(-_model.getRiskFreeRate()*(T-t));
    
    for (std::size_t i =0;  i < _num_iterations; ++i){
        _model.asset(past, t, _path, _rng);
        double payoff = _option.ComputePayoff(_path);
        sum += payoff;
        s_sum += payoff*payoff;
    }

    PriceAndStdDev result;
    result.price = discount_factor * sum/M;
    double variance =  discount_factor * discount_factor * (s_sum/ M - (sum/M)*(sum/M));
    result.std_dev = std::sqrt(variance / M);
    return result;
}


void MonteCarlo::delta(const PnlMat* past,double t,double fdstep, PnlVect* deltas, int M){
    pnl_vect_resize(deltas, _model.getD());
    pnl_vect_set_zero(deltas);
    double T = _model.getTimeHorizon();
    PnlMat* path1 = pnl_mat_new();
    PnlMat* path2 = pnl_mat_new();
    for(int j = 0; j < M; j++){
        _model.asset(past, t, _path, _rng);
        pnl_mat_clone(path1, _path);
        pnl_mat_clone(path2, _path);
        for(int i = 0; i < _model.getD(); i++){
            _model.shift_asset(t, path1, path2, fdstep, i);
            double payoff1 = _option.ComputePayoff(path1);
            double payoff2 = _option.ComputePayoff(path2);
            LET(deltas, i) += payoff1 - payoff2;
            _model.unshift_asset(t, path1, path2, fdstep, i);
    }
    }
    double discount = std::exp(-_model.getRiskFreeRate() * (T - t));
    double multcoeff;
    for(int i = 0; i < _model.getD(); i++){
        multcoeff = discount/(2.0 * fdstep * pnl_mat_get(past, past->m - 1, i)*M);
        pnl_vect_set(deltas, i, multcoeff*pnl_vect_get(deltas,i));
    }
    pnl_mat_free(&path1);
    pnl_mat_free(&path2);
}

