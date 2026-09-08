#include "Option.hpp"
#include <pnl/pnl_vector.h>

Option::Option(PnlVect* weights, std::size_t num_steps):_weights(weights), _num_steps(num_steps){}
    
Option::~Option(){pnl_vect_free(&_weights);}

PnlVect* Option::GetWeights() const {return _weights;}
std::size_t Option::GetNumSteps() const {return _num_steps;}