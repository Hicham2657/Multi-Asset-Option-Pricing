#include <stdexcept>
#include <list>
#include <cmath>
#include "MonteCarlo.hpp"
#include "PortfolioBuilder.hpp"


PortfolioBuilder::PortfolioBuilder(MonteCarlo* mc): lastcomposition(pnl_vect_new()),
                                                    monteCarlo(mc),positions(){}

PortfolioBuilder::~PortfolioBuilder(){pnl_vect_free(&lastcomposition);}


double PortfolioBuilder::PortfolioValue(const PnlMat* path,const PnlVect* deltas,int ligne){
    double value=0;
    for(int i=0; i<path->n; i++){
        value+=pnl_mat_get(path,ligne,i)*pnl_vect_get(deltas,i);
    }
    return value;
}



void PortfolioBuilder::buildPortfolio(const PnlMat* path,double fdstep,int N, int H ,double riskfree, double Maturity) { 
    //N point d'observation pour l'option //H point de balancement
    if ((H - 1) % N != 0) {
    throw std::invalid_argument("Les dates de fixing doivent etre incluses dans les dates de rebalancement");
    }

    double portfoliovalue=0;
    double timestep_rebalancement=Maturity/static_cast<double>(H-1);
    double capitalisation=exp(timestep_rebalancement*riskfree);

    //Initialisation
    PnlMat* past = pnl_mat_create(0, path->n);
    PnlVect row = pnl_vect_wrap_mat_row(path, 0);
    pnl_mat_add_row(past, past->m, &row);
    double time = 0.0;
    PricingResults result =monteCarlo->PriceAndDeltas(past, time, fdstep);
    double Cash = result.price;
    for (int d = 0; d < path->n; ++d)
    {
        Cash -= MGET(path, 0, d) * GET(result.delta, d);
    }
    pnl_vect_clone(lastcomposition, result.delta);
    positions.emplace_back(0,result.price,result.priceStdDev, result.delta,result.deltaStdDev,result.price);

    //hedging t>0
    int observationStep = (H - 1) / N;
    for (int i = 1; i < H; i ++){
        time+=timestep_rebalancement;
        row = pnl_vect_wrap_mat_row(path, i);
        pnl_mat_add_row(past, past->m, &row);
        result=monteCarlo->PriceAndDeltas(past,time, fdstep);
        Cash*=capitalisation;
        for(int j=0; j<path->n; j++){
            double crt_delta=pnl_vect_get(result.delta,j);
            double last_delta=pnl_vect_get(lastcomposition,j);
            Cash-=pnl_mat_get(path, i, j)*(crt_delta-last_delta);
        }
        portfoliovalue=PortfolioValue(path,result.delta,i);
        positions.emplace_back(i,result.price,result.priceStdDev, result.delta,result.deltaStdDev,Cash+portfoliovalue);
        pnl_vect_clone(lastcomposition, result.delta);
        if (i % observationStep != 0) {
            pnl_mat_del_row(past, past->m - 1);
        }
    }    
    pnl_mat_free(&past);
}