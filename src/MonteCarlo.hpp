#pragma once
#include <pnl/pnl_random.h>
#include <pnl/pnl_matvect.h>
#include <cstddef>

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
        // Buffers pour les chemins bumpes +-fd_step (deltas), reutilises
        // entre tous les appels a simulateAndAccumulate() au lieu d'etre
        // recrees a chaque appel : pnl_mat_clone() ne realloue que si la
        // taille change, donc une fois la taille stabilisee (des le premier
        // appel), plus aucune allocation ici.
        PnlMat* _pathUp;
        PnlMat* _pathDown;

        // Coeur commun a price()/delta()/PriceAndDeltas() : simule
        // _num_iterations trajectoires (une par iteration, via _model.asset)
        // et accumule les sommes brutes necessaires au prix (sum/s_sum) et/ou
        // aux deltas (deltaSum/deltaSumSq). Un pointeur nul desactive le
        // calcul correspondant (le travail associe - payoff de base, ou
        // shift/unshift + payoffs bumpes - n'est pas fait), pour ne payer que
        // ce qui est demande.
        void simulateAndAccumulate(const PnlMat* past, double t, double fd_step,
                                   double* sum, double* s_sum,
                                   PnlVect* deltaSum, PnlVect* deltaSumSq);

        // Transforme les sommes brutes de deltas (accumulees par
        // simulateAndAccumulate) en deltas/ecarts-types finaux (coefficient
        // d'actualisation, moyenne, variance) - partage par delta() et
        // PriceAndDeltas().
        void finalizeDeltas(const PnlMat* past, double t, double fd_step,
                            PnlVect* deltas, PnlVect* deltaStdDev) const;

    public:
        MonteCarlo(BlackScholesModel& model, const Option& option, std::size_t num_iterations);
        ~MonteCarlo();

        MonteCarlo(const MonteCarlo&) = delete;
        MonteCarlo& operator=(const MonteCarlo&) = delete;

        PriceAndStdDev price(const PnlMat* past, double t);
        void delta(const PnlMat* past,double t,double fdstep, PnlVect* deltas, PnlVect* delta_std_dev);

        // Calcule le prix ET les deltas en une seule passe de simulation (les
        // memes M trajectoires servent au prix et aux deltas, au lieu de deux
        // simulations independantes comme le ferait price() + delta()).
        //
        // `deltas` et `deltaStdDev` doivent etre des PnlVect* deja alloues
        // (ex. pnl_vect_new()) : ils sont redimensionnes et remplis en place.
        // PriceAndDeltas() ne les alloue ni ne les libere : c'est a l'appelant
        // de les creer et de les liberer (cf price0.cpp / PortfolioBuilder).
        void PriceAndDeltas(const PnlMat* past, double t, double fd_step,
                            double& price, double& priceStdDev,
                            PnlVect* deltas, PnlVect* deltaStdDev);

};  