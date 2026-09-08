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
        
    public:
        BlackScholesModel(double riskFreeRate, const PnlVect* sigmas, double timeHorizon, double correlation);
        ~BlackScholesModel();
        BlackScholesModel(const BlackScholesModel&) = delete;
        BlackScholesModel& operator=(const BlackScholesModel&) = delete;
        void asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng);
        std::size_t getD() const;
        double getRiskFreeRate() const ;
        double getTimeHorizon() const;
        PnlMat *generateTildePath(PnlRng *rng,int K, int D, int N,double dt);
        void buildPath(PnlMat* stildas, PnlMat* path, int K, int D, int N);

};