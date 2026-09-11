#pragma once
#include <iostream>
#include "pnl/pnl_vector.h"

class PricingResults
{
  public:
    double price;
    const PnlVect* delta;
    double priceStdDev;
    const PnlVect* deltaStdDev;

    PricingResults(double p_price, double p_priceStdDev, const PnlVect* const p_delta, const PnlVect* const p_deltaStdDev);

    friend std::ostream& operator<<(std::ostream& stm, const PricingResults& res);
};

