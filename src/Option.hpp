#pragma once
#include <pnl/pnl_matvect.h>
#include <cstddef>

class Option
{
protected:
    PnlVect* _weights;
    std::size_t _num_steps;
public:
    Option(const PnlVect* weights, std::size_t num_steps);
    virtual ~Option();
    Option(const Option&) = delete;
    Option& operator=(const Option&) = delete;
    virtual double ComputePayoff(const PnlMat *spots) const = 0;

    const PnlVect* GetWeights() const;
    std::size_t GetNumSteps() const;
};