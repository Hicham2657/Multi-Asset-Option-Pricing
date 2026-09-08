#include "AsianOption.hpp"
#include <pnl/pnl_matrix.h>
#include <algorithm>
#include <pnl/pnl_vector.h>

AsianOption::AsianOption(PnlVect* weights, int num_steps, double strike)
    : Option(weights, num_steps), _strike(strike) {}

double AsianOption::ComputePayoff(const PnlMat* spots) const
{
    double sum = 0.0;
    for (int d = 0; d < spots->n; ++d) {
        double crtactifsum=0.0;
        for (int i = 0; i < spots->m; ++i) {
            crtactifsum += MGET(spots, i, d);
        }
        sum += GET(_weights, d) * crtactifsum;
    }
    double weighted_average = sum / spots->m;
    return std::max(weighted_average - _strike, 0.0);
}
