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
    _S_tildas = pnl_mat_new();
}

BlackScholesModel::~BlackScholesModel() {
    pnl_vect_free(&_sigmas);
    pnl_mat_free(&_cholesky);
    pnl_vect_free(&_G);
    pnl_mat_free(&_S_tildas);
}

double my_exp(double x)
{
    return std::exp(x);
};

double EPS = 1E-10;
// helper donné par les profs
int compute_last_index(double t, double T, int N) {
    double dt = T / N;
    int nearest_index = std::round(t / dt);
    if (std::fabs(nearest_index * dt - t) < EPS) {
        return nearest_index;
    } else {
        return int(t / dt);
    }
}
    
void BlackScholesModel::generateTildePath(int K, int N, PnlRng* rng){
    int D = getD();
    pnl_mat_resize(_S_tildas, K, D);
    double dt = _timeHorizon/static_cast<double>(N);
    double sqrt_dt = std::sqrt(dt);
    for (int i = 0; i<K; ++i){
        pnl_vect_rng_normal_d(_G, D, rng);
        for (int d = 0; d<D; ++d){
            PnlVect L_d = pnl_vect_wrap_mat_row(_cholesky, d);
            double sigma_d = GET(_sigmas, d);
            double drift = (_riskFreeRate - sigma_d*sigma_d/2)* dt;
            double diffusion = sigma_d*sqrt_dt* pnl_vect_scalar_prod(&L_d, _G);
            MLET(_S_tildas, i, d) = std::exp(drift + diffusion);
        }
    }
}

void BlackScholesModel::buildPath(PnlMat* path, int K){
    int N = path->m - 1;
    int D = getD();
    for (int i = 0; i<K; ++i){
        for (int d = 0; d <D; ++d){
            MLET(path, N-K+i+1, d) = MGET(path, N-K+i, d) * MGET(_S_tildas, i, d); 
        }
    }
}

void BlackScholesModel::asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng){
    int N = path->m - 1;
    for (int i = 0; i < past->m; i++){
        PnlVect past_ith_row = pnl_vect_wrap_mat_row(past, i);
        pnl_mat_set_row(path, &past_ith_row, i);
    }
    int K = N + 1 - past->m;
    generateTildePath(K, N, rng);
    buildPath(path, K);
}

int BlackScholesModel::getD() const{return _sigmas->size;}
double BlackScholesModel::getRiskFreeRate() const{return _riskFreeRate;}
double BlackScholesModel::getTimeHorizon() const {return _timeHorizon;}
