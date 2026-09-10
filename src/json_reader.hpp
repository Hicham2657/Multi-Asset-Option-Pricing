#pragma once

#include <cstddef>
#include <string>
#include <pnl/pnl_vector.h>

/// Paramètres d'un cas de pricing, lus depuis un fichier `<data_input>.json`.
///
/// L'objet POSSÈDE les trois `PnlVect*` (`spots`, `volatilities`,
/// `payoffCoeffs`) : ils sont alloués par le constructeur et libérés par le
/// destructeur (RAII). La copie est interdite pour éviter toute double
/// libération.
///
/// Correspondance clé JSON -> attribut (notations du sujet).
/// Certaines clés ont deux orthographes selon la version des fichiers de
/// données du cours : les deux sont acceptées.
///   "model size" / "option size"          -> dim           (D, nb de sous-jacents)
///   "strike"                              -> strike        (K ; absent pour "performance")
///   "spot"                                -> spots         (S_0, ramené à la taille D)
///   "maturity"                            -> maturity      (T)
///   "volatility"                          -> volatilities  (sigma, ramené à la taille D)
///   "interest rate"                       -> interestRate  (r)
///   "correlation"                         -> correlation   (rho)
///   "option type"                         -> optionType    ("basket"|"asian"|"performance")
///   "payoff coefficients"                 -> payoffCoeffs  (lambda, ramené à la taille D)
///   "timestep number" / "fixing dates number" -> fixingDatesNb (N)
///   "sample number"                       -> sampleNb      (M)
///   "fd step"                             -> fdStep        (pas de diff. finie, deltas)
///
/// Les clés "model type", "trend" et "hedging dates number" sont ignorées
/// (non utilisées pour le prix / delta en 0).
struct PricingInput
{
    int         dim           = 0;
    double      strike        = 0.0;
    double      maturity      = 0.0;
    double      interestRate  = 0.0;
    double      correlation   = 0.0;
    int         fixingDatesNb = 0;
    std::size_t sampleNb      = 0;
    double      fdStep        = 0.1;
    std::string optionType;

    PnlVect*    spots         = nullptr;
    PnlVect*    volatilities  = nullptr;
    PnlVect*    payoffCoeffs  = nullptr;

    /// Lit et valide le fichier JSON pointé par `path`.
    /// Lève `std::runtime_error` si le fichier est illisible ou mal formé,
    /// et l'exception `nlohmann::json` d'origine si une clé obligatoire manque.
    explicit PricingInput(const std::string& path);
    ~PricingInput();

    PricingInput(const PricingInput&)            = delete;
    PricingInput& operator=(const PricingInput&) = delete;
};
