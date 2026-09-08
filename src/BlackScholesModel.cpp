#include "BlackScholesModel.hpp"
#include <cmath>

BlackScholesModel::BlackScholesModel(double riskFreeRate, PnlVect* sigmas, double timeHorizon,double correlation)
    : _riskFreeRate(riskFreeRate), _sigmas(sigmas),_timeHorizon(timeHorizon), _correlation(correlation) {
    _cholesky = pnl_mat_create_from_scalar(getD(), getD(), correlation);
    pnl_mat_set_diag(_cholesky, 1.0, 0);
    pnl_mat_chol(_cholesky);
    _G = pnl_vect_new();
}

BlackScholesModel::~BlackScholesModel() {
    pnl_vect_free(&_sigmas);
    pnl_mat_free(&_cholesky);
    pnl_vect_free(&_G);
}

double my_exp(double x)
{
    return std::exp(x);
};


PnlMat* BlackScholesModel::generateTildePath( PnlRng *rng,int K, int D, int N,double dt){
    PnlMat* stildas = pnl_mat_create_from_zero(K, D);
    for (int i =0 ; i < K; i++) {
        pnl_vect_rng_normal_d(_G, D, rng);

        PnlVect* diffusion = pnl_vect_new();
        pnl_mat_mult_vect_inplace(diffusion, _cholesky, _G);
        
        pnl_vect_mult_vect_term(diffusion, _sigmas);
        pnl_vect_mult_scalar(diffusion,sqrt(dt));
        
        
        PnlVect* drift = pnl_vect_copy(_sigmas);

        // drift = sigma^2
        pnl_vect_mult_vect_term(drift, _sigmas);

        // drift = - sigma^2 / 2
        pnl_vect_mult_scalar(drift, -0.5);

        // drift = r - sigma^2 / 2
        pnl_vect_plus_scalar(drift, _riskFreeRate);

        // drift = (r - sigma^2 / 2) * dt
        pnl_vect_mult_scalar(drift, dt);

        PnlVect* s_tilda=pnl_vect_copy(drift);
        pnl_vect_plus_vect(s_tilda, diffusion);

        pnl_vect_map_inplace(s_tilda, my_exp);

        pnl_mat_set_row(stildas, s_tilda, N-K+i);
        pnl_vect_free(&diffusion);
        pnl_vect_free(&drift);
        pnl_vect_free(&s_tilda);
    }
    return stildas;
};
    


void BlackScholesModel::buildPath(PnlMat* stildas, PnlMat* path, int K, int D, int N){
    for (int i = 0; i < K; i++) {
        for (int d = 0; d < D; d++) {
            double s_tilda = MGET(stildas, i, d);
            double s_prev = MGET(path, N-K+i-1, d);
            MLET(path, N-K+i, d) = s_prev * s_tilda;
        }
    }
}


void BlackScholesModel::asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng){
    int N = path->m - 1;
    int D = path->n;
    double dt = _timeHorizon/static_cast<double>(N);

    for (int i = 0; i < past->m; i++){
        PnlVect* row = pnl_vect_new();
        pnl_mat_get_row(row, past, i);
        pnl_mat_set_row(path, row, i);
        pnl_vect_free(&row);
    }

    PnlMat* stildas = generateTildePath(rng, N, D, N, dt);
    buildPath(stildas, path, N, D, N);
    pnl_mat_free(&stildas);
}


double BlackScholesModel::getRiskFreeRate() const{return _riskFreeRate;}
double BlackScholesModel::getTimeHorizon() const {return _timeHorizon;}
