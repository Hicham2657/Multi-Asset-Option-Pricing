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
    void buildPortfolio(const PnlMat* path,double fdstep,int N, double riskfree, double Maturity);

    virtual ~PortfolioBuilder();
};

