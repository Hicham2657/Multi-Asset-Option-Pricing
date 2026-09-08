#include "BlackScholesModel.hpp"
#include <cmath>
#include <stdexcept>

BlackScholesModel::BlackScholesModel(double riskFreeRate, const PnlVect* sigmas, double timeHorizon,double correlation)
    : _riskFreeRate(riskFreeRate), _sigmas(pnl_vect_copy(sigmas)),_timeHorizon(timeHorizon), _correlation(correlation) {
    _cholesky = pnl_mat_create_from_scalar(getD(), getD(), correlation);
    pnl_mat_set_diag(_cholesky, 1.0, 0);
    if (pnl_mat_chol(_cholesky) != 0) {
        pnl_vect_free(&_sigmas);
        pnl_mat_free(&_cholesky);
        throw std::invalid_argument("correlation matrix is not positive definite");
    }
    _G = pnl_vect_new();
}

BlackScholesModel::~BlackScholesModel() {
    pnl_vect_free(&_sigmas);
    pnl_mat_free(&_cholesky);
    pnl_vect_free(&_G);
}

void BlackScholesModel::asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng){
    int N = path->m - 1;
    int D = path->n;
    double dt = _timeHorizon/static_cast<double>(N);
    double sqrt_dt = std::sqrt(dt);

    PnlVect S_t = pnl_vect_wrap_mat_row(past, past->m - 1);
    pnl_mat_set_row(path, &S_t, 0);

    int start = 0;
    for (int i = start+1; i<N+1; ++i){
        pnl_vect_rng_normal_d(_G, D, rng);
        for (int d = 0; d<D; ++d){
            PnlVect L_d = pnl_vect_wrap_mat_row(_cholesky, d);
            double sigma_d = GET(_sigmas, d);
            double drift = (_riskFreeRate - sigma_d*sigma_d/2)* dt;
            double diffusion = sigma_d*sqrt_dt* pnl_vect_scalar_prod(&L_d, _G);
            double S_i_d = MGET(path, i-1, d) * std::exp(drift + diffusion);
            MLET(path, i, d) = S_i_d;
        }
    }
}

std::size_t BlackScholesModel::getD() const{return _sigmas->size;}
double BlackScholesModel::getRiskFreeRate() const{return _riskFreeRate;}
double BlackScholesModel::getTimeHorizon() const {return _timeHorizon;}
