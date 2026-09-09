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
        PnlMat* _sTilde;
        
    public:
        BlackScholesModel(double riskFreeRate, const PnlVect* sigmas, double timeHorizon, double correlation);
        ~BlackScholesModel();
        BlackScholesModel(const BlackScholesModel&) = delete;
        BlackScholesModel& operator=(const BlackScholesModel&) = delete;
        void asset(const PnlMat *past, double t, PnlMat *path, PnlRng *rng);
        int getD() const;
        double getRiskFreeRate() const ;
        double getTimeHorizon() const;
        void simulateSTilde(int K, int N, double first_step, PnlRng* rng);
        void buildPathFromSTilde(PnlMat* path, int K, const PnlVect* s_t);
        double shift_asset(double t, PnlMat* path1, PnlMat* path2,double fdstep, int asset_i);
        double unshift_asset(double t, PnlMat* path1, PnlMat* path2,double fdstep, int asset_i);

};