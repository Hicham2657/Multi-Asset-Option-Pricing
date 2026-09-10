#pragma once

#include <pnl/pnl_matrix.h>
#include <pnl/pnl_random.h>
#include <pnl/pnl_vector.h>

#include <initializer_list>
#include <memory>

struct PnlVectDeleter
{
    void operator()(PnlVect* vect) const
    {
        pnl_vect_free(&vect);
    }
};

struct PnlMatDeleter
{
    void operator()(PnlMat* mat) const
    {
        pnl_mat_free(&mat);
    }
};

struct PnlRngDeleter
{
    void operator()(PnlRng* rng) const
    {
        pnl_rng_free(&rng);
    }
};

using PnlVectPtr = std::unique_ptr<PnlVect, PnlVectDeleter>;
using PnlMatPtr = std::unique_ptr<PnlMat, PnlMatDeleter>;
using PnlRngPtr = std::unique_ptr<PnlRng, PnlRngDeleter>;

inline PnlVectPtr makeVector(std::initializer_list<double> values)
{
    return PnlVectPtr{pnl_vect_create_from_ptr(
        static_cast<int>(values.size()), values.begin())};
}

inline PnlMatPtr makeMatrix(int rows, int columns,
                            std::initializer_list<double> values)
{
    return PnlMatPtr{pnl_mat_create_from_ptr(rows, columns, values.begin())};
}

inline PnlRngPtr makeRng(unsigned long seed = 0)
{
    PnlRngPtr rng{pnl_rng_create(PNL_RNG_MERSENNE)};
    pnl_rng_sseed(rng.get(), seed);
    return rng;
}
