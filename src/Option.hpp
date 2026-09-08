#pragma once
#include <pnl/pnl_matvect.h>
#include <cstddef>

class Option
{
protected:
    PnlVect* _weights;
    std::size_t _num_steps;
public:
    Option(PnlVect* weights, std::size_t num_steps);
    virtual ~Option();
    virtual double ComputePayoff(const PnlMat *spots) const = 0;

    PnlVect* GetWeights() const;
    std::size_t GetNumSteps() const;
};