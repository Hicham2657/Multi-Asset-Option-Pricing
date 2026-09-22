#include <stdexcept>
#include <list>
#include <cmath>
#include "MonteCarlo.hpp"
#include "PortfolioBuilder.hpp"


PortfolioBuilder::PortfolioBuilder(MonteCarlo* mc): lastcomposition(pnl_vect_new()),
                                                    monteCarlo(mc),positions(){}

PortfolioBuilder::~PortfolioBuilder(){
    pnl_vect_free(&lastcomposition);
    // Chaque Position garde des PnlVect* alloués par buildPortfolio() (via
    // MonteCarlo::PriceAndDeltas) ; Position n'a pas de destructeur qui les
    // libère (ni chez nous, ni dans la version de référence des profs) :
    // c'est à PortfolioBuilder, qui les a créés, de le faire.
    for (Position& position : positions) {
        pnl_vect_free(&position.deltas);
        pnl_vect_free(&position.deltasStdDev);
    }
}


double PortfolioBuilder::PortfolioValue(const PnlMat* path,const PnlVect* deltas,int ligne){
    double value=0;
    for(int i=0; i<path->n; i++){
        value+=pnl_mat_get(path,ligne,i)*pnl_vect_get(deltas,i);
    }
    return value;
}



void PortfolioBuilder::buildPortfolio(const PnlMat* path, double fdstep, int N, double riskfree,
                                      double Maturity, int fixingDatesNb) {
    //N pas de rebalancement (grille H), fixingDatesNb pas de constatation (grille N)

    // BlackScholesModel::asset() attend un "past" indexé par les dates de
    // CONSTATATION de l'option (fixingDatesNb lignes au maximum), pas par les
    // dates de REBALANCEMENT (N, généralement beaucoup plus fin) : lui donner
    // une ligne à chaque pas de rebalancement fait lire des valeurs de marché
    // prises aux mauvaises dates dès qu'on dépasse la 1re constatation. Le
    // sujet suppose H (=N ici) multiple de fixingDatesNb (les dates de
    // constatation tombent exactement sur des dates de rebalancement) ; voir
    // pricer.pdf sur les deux grilles. Vérifié ici (idée de Reda,
    // "optim portfoliobuilder") : la classe qui a besoin de l'hypothèse la
    // vérifie elle-même plutôt que de compter sur l'appelant.
    if (fixingDatesNb <= 0 || N % fixingDatesNb != 0) {
        throw std::invalid_argument(
            "buildPortfolio: 'hedging dates number' doit etre un multiple de "
            "'fixing dates number'/'timestep number'");
    }
    const int hedgeStepsPerFixing = N / fixingDatesNb;

    double portfoliovalue=0;
    double timestep=Maturity/static_cast<double>(N);
    double capitalisation=exp(timestep*riskfree);

    //Initialisation
    PnlVect row = pnl_vect_wrap_mat_row(path, 0);
    PnlMat* past = pnl_mat_create(1, path->n);
    pnl_mat_set_row(past, &row, 0);
    double time = 0.0;

    // PriceAndDeltas() ne possede pas les PnlVect* qu'on lui passe : c'est
    // nous qui les allouons, un jeu neuf par position (chaque Position garde
    // un pointeur vers "ses" deltas, il ne faut donc pas reutiliser le meme
    // buffer d'une date de rebalancement a l'autre, sous peine que toutes les
    // positions du historique finissent par pointer vers les memes valeurs).
    double price = 0.0, priceStdDev = 0.0;
    PnlVect* deltas = pnl_vect_new();
    PnlVect* deltasStdDev = pnl_vect_new();
    monteCarlo->PriceAndDeltas(past, time, fdstep, price, priceStdDev, deltas, deltasStdDev);

    double Cash = price;
    for (int d = 0; d < path->n; ++d)
    {
        Cash -= MGET(path, 0, d) * GET(deltas, d);
    }
    pnl_vect_clone(lastcomposition, deltas);
    positions.emplace_back(0, price, priceStdDev, deltas, deltasStdDev, price);

    //hedging t>0 : `past` grandit d'une ligne à chaque pas (la ligne de
    // marché courante, nécessaire comme "spot courant" pour asset()), puis
    // rétrécit aussitôt après si ce pas n'était pas une vraie date de
    // constatation — sinon la ligne reste, définitivement acquise comme
    // constatation réalisée pour la suite. Une seule matrice `past` pour
    // toute la boucle (pas de réallocation à chaque pas).
    for (int i = 1; i <= N; ++i){
        time+=timestep;
        row = pnl_vect_wrap_mat_row(path, i);
        pnl_mat_add_row(past, past->m, &row);

        deltas = pnl_vect_new();
        deltasStdDev = pnl_vect_new();
        monteCarlo->PriceAndDeltas(past, time, fdstep, price, priceStdDev, deltas, deltasStdDev);

        Cash*=capitalisation;
        for(int j=0; j<path->n; j++){
            double crt_delta=pnl_vect_get(deltas,j);
            double last_delta=pnl_vect_get(lastcomposition,j);
            Cash-=pnl_mat_get(path, i, j)*(crt_delta-last_delta);
        }
        portfoliovalue=PortfolioValue(path,deltas,i);
        positions.emplace_back(i, price, priceStdDev, deltas, deltasStdDev, Cash+portfoliovalue);
        pnl_vect_clone(lastcomposition, deltas);

        if (i % hedgeStepsPerFixing != 0) {
            pnl_mat_del_row(past, past->m - 1);
        }
    }
    pnl_mat_free(&past);
}
