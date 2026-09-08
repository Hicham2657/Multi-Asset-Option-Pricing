#pragma once
#include "Option.hpp"
#include <cstddef>

class Basket : public Option{
    private:
        double _strike;
    public:
        Basket(const PnlVect* weights, std::size_t num_steps, double strike);
        double ComputePayoff(const PnlMat* spots) const override;
};