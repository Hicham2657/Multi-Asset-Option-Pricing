#pragma once

#include "Option.hpp"

class OptionPerformance : public Option{
    public:
        OptionPerformance(const PnlVect* weights, std::size_t num_steps);
        double ComputePayoff(const PnlMat* spots) const override;
};