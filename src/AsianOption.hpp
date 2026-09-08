#pragma once

#include "Option.hpp"

class AsianOption : public Option
{
private:
    double _strike;
public:
    AsianOption(PnlVect* weights, int num_steps, double strike);
    double ComputePayoff(const PnlMat* spots) const override;
};