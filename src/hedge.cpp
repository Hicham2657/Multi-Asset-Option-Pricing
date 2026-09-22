// Exécutable de couverture : construit le portefeuille de couverture le long
// d'une trajectoire de marché réalisée.
//
// Usage imposé par le sujet (version actuelle) :
//     ./hedge <market_file.txt> <data_input.json>
//
// Sortie : UNIQUEMENT le JSON `{"portfolio": [...]}` sur stdout (comme le fait
// `Portfolio::operator<<` de référence). Toute erreur est écrite sur stderr et
// le programme renvoie un code != 0.

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>

#include <nlohmann/json.hpp>
#include <pnl/pnl_matrix.h>

#include "json_reader.hpp"
#include "option_factory.hpp"
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "PortfolioBuilder.hpp"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage : " << argv[0] << " <market_file.txt> <data_input.json>\n";
        return EXIT_FAILURE;
    }

    try
    {
        // 1. Lecture des paramètres du cas de test et de la trajectoire de marché.
        const PricingInput in(argv[2]);

        PnlMat* marketPath = pnl_mat_create_from_file(argv[1]);
        if (marketPath == nullptr || marketPath->m == 0)
        {
            std::cerr << "Impossible de lire le fichier de marché : " << argv[1] << '\n';
            return EXIT_FAILURE;
        }
        if (in.hedgingDatesNb <= 0)
        {
            pnl_mat_free(&marketPath);
            std::cerr << "'hedging dates number' est absent ou nul dans " << argv[2] << '\n';
            return EXIT_FAILURE;
        }
        // (PortfolioBuilder::buildPortfolio vérifie elle-même que
        // hedgingDatesNb est multiple de fixingDatesNb, et lève une exception
        // sinon — cf. le catch ci-dessous.)

        // 2. Construction de l'option, du modèle et du moteur Monte-Carlo.
        const std::unique_ptr<Option> option = make_option(in);

        BlackScholesModel model(in.interestRate, in.volatilities,
                                in.maturity, in.correlation);

        MonteCarlo engine(model, *option, in.sampleNb);

        // 3. Parcours de la trajectoire de marché, rebalancement du portefeuille
        //    de couverture à chaque pas (méthode déjà implémentée : PortfolioBuilder).
        PortfolioBuilder builder(&engine);
        builder.buildPortfolio(marketPath, in.fdStep, in.hedgingDatesNb,
                               in.interestRate, in.maturity, in.fixingDatesNb);

        // 4. Sortie : {"portfolio": [...]}, comme Portfolio::operator<< de référence.
        const nlohmann::json out = { {"portfolio", builder.getPositions()} };
        std::cout << std::setw(4) << out << std::endl;

        pnl_mat_free(&marketPath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Erreur : " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
