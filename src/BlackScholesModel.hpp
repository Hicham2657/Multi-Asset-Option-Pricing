#pragma once
#include <pnl/pnl_matvect.h>
#include <pnl/pnl_random.h>


class BlackScholesModel{
    private:
        double _riskFreeRate;
        PnlVect* _sigmas;
        double _timeHorizon;
        double _correlation;
        PnlMat* _cholesky;
        PnlVect* _G;
        PnlMat* _S_tildas;
        
    public:
        BlackScholesModel(double riskFreeRate, const PnlVect* sigmas, double timeHorizon, double correlation);
        ~BlackScholesModel();
        BlackScholesModel(const BlackScholesModel&) = delete;
        BlackScholesModel& operator=(const BlackScholesModel&) = delete;
        void asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng);
        int getD() const;
        double getRiskFreeRate() const ;
        double getTimeHorizon() const;
        void generateTildePath(int K, int N, PnlRng* rng);
        void buildPath(PnlMat* path, int K);

};