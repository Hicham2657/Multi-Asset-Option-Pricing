#pragma once

#include <list>
#include <nlohmann/json_fwd.hpp>
#include "MonteCarlo.hpp"

class Position {
public:
    int date;
    PnlVect *deltas;
    PnlVect *deltasStdDev;
    double price;
    double priceStdDev;
    double portfolioValue;

    Position(int date, double price, double priceStdDev, PnlVect* deltas, PnlVect* deltasStdDev, double portfolioValue);
    friend void to_json(nlohmann::json &j, const Position &positions);
    void print() const;
};

class Portfolio {
public:
    MonteCarlo &monteCarlo;
    std::list<Position> positions;

    Portfolio(nlohmann::json &jsonParams, MonteCarlo &monteCarlo);
    ~Portfolio();
};
