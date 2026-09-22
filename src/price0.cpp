// Exécutable de calcul du prix (et, à terme, des deltas) en t = 0.
//
// Usage imposé par le sujet :
//     ./price0 <data_input.json>
//
// Sortie : UNIQUEMENT le bloc JSON produit par `PricingResults` sur stdout.
// Toute erreur est écrite sur stderr et le programme renvoie un code != 0.

#include <cstdlib>
#include <iostream>
#include <memory>

#include <pnl/pnl_vector.h>
#include <pnl/pnl_matrix.h>

#include "json_reader.hpp"
#include "option_factory.hpp"
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "pricing_results.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage : " << argv[0] << " <data_input.json>\n";
        return EXIT_FAILURE;
    }

    try
    {
        // 1. Lecture des paramètres du cas de test.
        const PricingInput in(argv[1]);

       // 2. Construction de l'option et du modèle de marché.
        const std::unique_ptr<Option> option = make_option(in);

        BlackScholesModel model(in.interestRate, in.volatilities,
                                in.maturity, in.correlation);

        MonteCarlo engine(model, *option, in.sampleNb);

        // 3. État du marché connu en t = 0 : une seule ligne = S_0.
        PnlMat* past = pnl_mat_create(1, in.dim);
        pnl_mat_set_row(past, in.spots, 0);

        // 4. Prix ET deltas en 0, calculés en une seule passe de simulation
        //    (MonteCarlo::PriceAndDeltas). On alloue nous-mêmes les vecteurs
        //    de sortie : PriceAndDeltas() les redimensionne/remplit en place
        //    mais ne les possède pas.
        double price = 0.0, priceStdDev = 0.0;
        PnlVect* delta = pnl_vect_new();
        PnlVect* deltaStdDev = pnl_vect_new();
        engine.PriceAndDeltas(past, 0.0, in.fdStep, price, priceStdDev, delta, deltaStdDev);

        const PricingResults out(price, priceStdDev, delta, deltaStdDev);
        std::cout << out << std::endl;

        // 5. Libération.
        pnl_vect_free(&delta);
        pnl_vect_free(&deltaStdDev);
        pnl_mat_free(&past);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Erreur : " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
