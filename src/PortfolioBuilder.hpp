#pragma once

#include <list>
#include "MonteCarlo.hpp"
#include "pricing_results.hpp"
#include "portfolio.hpp"


class PortfolioBuilder
{
protected:
    PnlVect* lastcomposition;
    MonteCarlo* monteCarlo;
    std::list<Position> positions;

public:
    PortfolioBuilder(MonteCarlo* monteCarlo);
    double PortfolioValue(const PnlMat* path,const PnlVect* deltas,int ligne);

    // `N` = nombre de pas de REBALANCEMENT (grille H, "hedging dates number").
    // `fixingDatesNb` = nombre de dates de CONSTATATION de l'option (grille N,
    // "fixing dates number"/"timestep number") : ce sont deux grilles
    // différentes (H est en général beaucoup plus fine que N). Lève
    // std::invalid_argument si N n'est pas multiple de fixingDatesNb.
    void buildPortfolio(const PnlMat* path, double fdstep, int N, double riskfree,
                        double Maturity, int fixingDatesNb);

    // Accès en lecture aux positions construites par buildPortfolio(), pour
    // que l'appelant (ex. hedge.cpp) puisse les sérialiser.
    const std::list<Position>& getPositions() const { return positions; }

    virtual ~PortfolioBuilder();
};
