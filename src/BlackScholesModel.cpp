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
    _sTilde = pnl_mat_new();
}

BlackScholesModel::~BlackScholesModel() {
    pnl_vect_free(&_sigmas);
    pnl_mat_free(&_cholesky);
    pnl_vect_free(&_G);
    pnl_mat_free(&_sTilde);
}

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

// first step = (t_{i+1} - t) if t not among t_i's
void BlackScholesModel::simulateSTilde(int K, int N, double first_step, PnlRng* rng){
    int D = getD();
    pnl_mat_resize(_sTilde, K, D);
    double dt = _timeHorizon/static_cast<double>(N);

    for (int i = 0; i<K; ++i){
        pnl_vect_rng_normal_d(_G, D, rng);
        double step = (i==0) ? first_step : dt;
        double sqrt_step = std::sqrt(step);
        for (int d = 0; d<D; ++d){
            PnlVect L_d = pnl_vect_wrap_mat_row(_cholesky, d);
            double sigma_d = GET(_sigmas, d);
            double drift = (_riskFreeRate - sigma_d*sigma_d/2)* step;
            double diffusion = sigma_d*sqrt_step* pnl_vect_scalar_prod(&L_d, _G);
            MLET(_sTilde, i, d) = std::exp(drift + diffusion);
        }
    }
}

void BlackScholesModel::buildPathFromSTilde(PnlMat* path, int K, PnlVect* s_t){
    int N = path->m - 1;
    int D = getD();

    for (int d = 0; d<D; ++d){
        MLET(path, N-K+1 ,d) =  GET(s_t, d)* MGET(_sTilde, 0, d);
    }

    for (int i = 1; i<K; ++i){
        for (int d = 0; d <D; ++d){
            MLET(path, N-K+i+1, d) = MGET(path, N-K+i, d) * MGET(_sTilde, i, d); 
        }
    }
}

void BlackScholesModel::asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng){
    int N = path->m - 1;

    int i = compute_last_index(t, _timeHorizon, N);
    double dt = _timeHorizon/N;

    for (int l = 0; l <= i; l++){
        PnlVect past_ith_row = pnl_vect_wrap_mat_row(past, l);
        pnl_mat_set_row(path, &past_ith_row, l);
    }
    
    PnlVect S_t = pnl_vect_wrap_mat_row(past, past->m - 1);
    double first_step = (std::abs(t - i*dt) < EPS) ? dt : (i+1)*dt - t;


    int K = N - i; //why not N+1-i
    simulateSTilde(K, N, first_step, rng);
    buildPathFromSTilde(path, K, &S_t);
}

int BlackScholesModel::getD() const{return _sigmas->size;}
double BlackScholesModel::getRiskFreeRate() const{return _riskFreeRate;}
double BlackScholesModel::getTimeHorizon() const {return _timeHorizon;}
