#include "Basket.hpp"
#include <pnl/pnl_vector.h>
#include <pnl/pnl_matrix.h>
#include <algorithm>


Basket::Basket(const PnlVect* weights, std::size_t num_steps, double strike): Option(weights, num_steps), _strike(strike){}

double Basket::ComputePayoff(const PnlMat* spots) const{
    PnlVect S_T = pnl_vect_wrap_mat_row(spots, spots->m -1);
    return std::max(pnl_vect_scalar_prod(&S_T, _weights) - _strike, 0.0);
}