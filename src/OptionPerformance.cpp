#include "OptionPerformance.hpp"
#include <pnl/pnl_matrix.h>
#include <algorithm>

OptionPerformance::OptionPerformance(const PnlVect* weights, std::size_t num_steps) : Option(weights, num_steps){}

double OptionPerformance::ComputePayoff(const PnlMat* spots) const
{
    double res = 1.0;
    PnlVect lastRow = pnl_vect_wrap_mat_row(spots, 0);
    double lastpayoff =pnl_vect_scalar_prod(&lastRow, _weights);

    for (int i = 1; i < spots->m; ++i)
    {
        PnlVect todayRow =pnl_vect_wrap_mat_row(spots, i);
        double todayspayoff =pnl_vect_scalar_prod(&todayRow, _weights);
        res += std::max(todayspayoff / lastpayoff - 1.0,0.0);
        lastpayoff = todayspayoff;
    }
    return res;
}