#pragma once

#include "Option.hpp"

class AsianOption : public Option
{
private:
    double _strike;
public:
    AsianOption(const PnlVect* weights, std::size_t num_steps, double strike);
    double ComputePayoff(const PnlMat* spots) const override;
};