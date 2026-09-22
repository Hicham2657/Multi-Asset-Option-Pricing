#include "MonteCarlo.hpp"
#include "BlackScholesModel.hpp"
#include "Option.hpp"
#include <cmath>
#include "utils.hpp"

MonteCarlo::MonteCarlo(BlackScholesModel& model, const Option& option, std::size_t num_iterations)
    : _model(model), _option(option), _num_iterations(num_iterations){
        _path = pnl_mat_create(_option.GetNumSteps() + 1, _model.getD());
        _pathUp = pnl_mat_new();
        _pathDown = pnl_mat_new();

        _rng = pnl_rng_create(PNL_RNG_MERSENNE);
        pnl_rng_sseed(_rng, 0);
    }

MonteCarlo::~MonteCarlo(){
    pnl_rng_free(&_rng);
    pnl_mat_free(&_path);
    pnl_mat_free(&_pathUp);
    pnl_mat_free(&_pathDown);
}

void MonteCarlo::simulateAndAccumulate(const PnlMat* past, double t, double fd_step,
                                       double* sum, double* s_sum,
                                       PnlVect* deltaSum, PnlVect* deltaSumSq)
{
    const int D = _model.getD();

    for (std::size_t j = 0; j < _num_iterations; ++j) {
        _model.asset(past, t, _path, _rng);   // une seule trajectoire par itération,
                                               // réutilisée pour le prix ET les deltas

        if (sum != nullptr) {
            const double payoff = _option.ComputePayoff(_path);
            *sum += payoff;
            *s_sum += payoff * payoff;
        }

        if (deltaSum != nullptr) {
            for (int d = 0; d < D; ++d) {
                // _pathUp/_pathDown sont des membres reutilises a chaque
                // iteration (et entre tous les appels a simulateAndAccumulate) :
                // pnl_mat_clone ne realloue que si la taille change, donc une
                // fois stabilisee (des la 1re iteration), plus aucune
                // allocation ici.
                pnl_mat_clone(_pathUp, _path);
                pnl_mat_clone(_pathDown, _path);
                _model.shift_asset(t, _pathUp, _pathDown, fd_step, d);

                const double diff = _option.ComputePayoff(_pathUp) - _option.ComputePayoff(_pathDown);
                LET(deltaSum, d)   += diff;
                LET(deltaSumSq, d) += diff * diff;
            }
        }
    }
}

void MonteCarlo::finalizeDeltas(const PnlMat* past, double t, double fd_step,
                                PnlVect* deltas, PnlVect* deltaStdDev) const
{
    const int D = _model.getD();
    const double M = static_cast<double>(_num_iterations);
    const double discount = std::exp(-_model.getRiskFreeRate() * (_model.getTimeHorizon() - t));

    for (int d = 0; d < D; ++d) {
        const double coeff = discount / (2.0 * fd_step * MGET(past, past->m - 1, d));
        const double mean = GET(deltas, d) / M;
        // GET(deltaStdDev, d)/M - mean*mean est une variance empirique : elle
        // ne peut pas être négative mathématiquement, mais les arrondis
        // flottants peuvent la rendre légèrement < 0 quand la vraie variance
        // est ~0 (payoff quasi déterministe, typiquement près de la
        // maturité). std::sqrt d'un négatif -> NaN -> "null" en JSON. On la
        // clippe à 0.
        const double var = std::max(0.0, GET(deltaStdDev, d) / M - mean * mean);
        LET(deltas, d)      = coeff * mean;
        LET(deltaStdDev, d) = std::abs(coeff) * std::sqrt(var / M);
    }
}

PriceAndStdDev MonteCarlo::price(const PnlMat* past, double t){
    double sum = 0.0;
    double s_sum = 0.0;
    simulateAndAccumulate(past, t, 0.0, &sum, &s_sum, nullptr, nullptr);

    const double T = _model.getTimeHorizon();
    const double M = static_cast<double>(_num_iterations);
    const double discount_factor = std::exp(-_model.getRiskFreeRate()*(T-t));

    PriceAndStdDev result;
    result.price = discount_factor * sum/M;
    // s_sum/M - (sum/M)^2 est une variance empirique : les arrondis flottants
    // peuvent la rendre legerement < 0 quand le payoff est quasi deterministe
    // (std::sqrt d'un negatif -> NaN -> "null" en JSON). On la clippe a 0.
    const double variance = std::max(0.0, discount_factor * discount_factor * (s_sum/ M - (sum/M)*(sum/M)));
    result.std_dev = std::sqrt(variance / M);
    return result;
}

void MonteCarlo::delta(const PnlMat* past, double t, double fd_step, PnlVect* deltas, PnlVect* delta_std_dev){
    pnl_vect_resize(deltas, _model.getD());
    pnl_vect_resize(delta_std_dev, _model.getD());
    pnl_vect_set_zero(deltas);
    pnl_vect_set_zero(delta_std_dev);

    simulateAndAccumulate(past, t, fd_step, nullptr, nullptr, deltas, delta_std_dev);
    finalizeDeltas(past, t, fd_step, deltas, delta_std_dev);
}


void MonteCarlo::PriceAndDeltas(const PnlMat* past, double t, double fd_step,
                                double& price, double& priceStdDev,
                                PnlVect* deltas, PnlVect* deltaStdDev)
{
    pnl_vect_resize(deltas, _model.getD());
    pnl_vect_resize(deltaStdDev, _model.getD());
    pnl_vect_set_zero(deltas);
    pnl_vect_set_zero(deltaStdDev);

    double sum = 0.0;
    double s_sum = 0.0;
    // Une seule simulation par itération : la même trajectoire _path sert au
    // payoff (pour le prix) ET de point de départ aux chemins bumpés ±fd_step
    // (pour les deltas), au lieu de deux boucles Monte-Carlo indépendantes.
    simulateAndAccumulate(past, t, fd_step, &sum, &s_sum, deltas, deltaStdDev);

    const double T = _model.getTimeHorizon();
    const double M = static_cast<double>(_num_iterations);
    const double discount = std::exp(-_model.getRiskFreeRate() * (T - t));
    price = discount * sum / M;
    const double variance = std::max(0.0, discount * discount * (s_sum / M - (sum / M) * (sum / M)));
    priceStdDev = std::sqrt(variance / M);

    finalizeDeltas(past, t, fd_step, deltas, deltaStdDev);
}
