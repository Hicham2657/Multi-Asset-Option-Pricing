#include "Option.hpp"
#include "Basket.hpp"
#include "MonteCarlo.hpp"
#include "BlackScholesModel.hpp"
#include <iostream>
#include "pnl/pnl_finance.h"
#include "AsianOption.hpp"

int main(){

    PnlVect* weights = pnl_vect_create_from_scalar(1, 1.0); 
    Basket Call{weights, 1, 100};
    
    PnlVect* sigmas = pnl_vect_create_from_scalar(1, 0.2);
    BlackScholesModel model{0.05,sigmas, 1.0, 0.0};

    PnlMat* past = pnl_mat_create_from_scalar(1,1,100.0);
    MonteCarlo mc{model, Call, 100};
    double ref = pnl_bs_call(100.0, 100.0, 1.0, 0.05, 0.0, 0.2);

    int inside = 0;
    for (int k = 1; k <= 10; ++k) {
        PriceAndCI res = mc.price(past, 0.0);
        bool ok = std::abs(res.price - ref) < res.ci;
        inside += ok;
        std::cout << "run " << k << " : " << res.price
                << " +/- " << res.ci << (ok ? "  OK" : "  HORS") << "\n";
}
    std::cout << inside << "/10 dans l'IC\n";
    pnl_mat_free(&past);

// PnlVect* weights1 = pnl_vect_create_from_scalar(1, 1.0); 
//     AsianOption Asian{weights1, 10, 100.0};
//     MonteCarlo mc1{model, Asian, 100};
    // std::cout << mc1.price(past, 0.0).price;

    return 0;
}