#include "json_reader.hpp"
#include "json_helper.hpp"   // from_json(const nlohmann::json&, PnlVect*&)

#include <fstream>
#include <stdexcept>
#include <pnl/pnl_vector.h>

namespace
{
    /// "spot", "volatility" et "payoff coefficients" sont parfois donnés en
    /// taille 1 même quand D > 1. Dans ce cas on réplique la valeur sur les
    /// D composantes (même convention que test/test_json_reader.cpp).
    void broadcast_to_dim(PnlVect* v, int dim)
    {
        if (v != nullptr && v->size == 1 && dim > 1)
            pnl_vect_resize_from_scalar(v, dim, GET(v, 0));
    }

    /// Renvoie la première clé présente parmi `names`. Les fichiers de données
    /// du cours ont changé de nom au fil des versions :
    ///   "model size"   <-> "option size"
    ///   "timestep number" <-> "fixing dates number"
    /// On accepte les deux orthographes.
    const nlohmann::json& at_any(const nlohmann::json& j,
                                 std::initializer_list<const char*> names)
    {
        for (const char* name : names)
        {
            auto it = j.find(name);
            if (it != j.end())
                return *it;
        }
        throw std::runtime_error(
            std::string("clé absente du JSON (aucune de : ") + *names.begin() + ", ...)");
    }
}

PricingInput::PricingInput(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw std::runtime_error("Impossible d'ouvrir le fichier : " + path);

    const nlohmann::json j = nlohmann::json::parse(ifs);

    // --- champs scalaires -------------------------------------------------
    at_any(j, {"model size", "option size"}).get_to(dim);
    j.at("maturity").get_to(maturity);
    j.at("interest rate").get_to(interestRate);
    j.at("correlation").get_to(correlation);
    at_any(j, {"timestep number", "fixing dates number"}).get_to(fixingDatesNb);
    j.at("sample number").get_to(sampleNb);
    j.at("option type").get_to(optionType);
    strike = j.value("strike", 0.0);   // "strike" est absent pour "performance"
    fdStep = j.value("fd step", 0.1);  // pas de la différence finie pour les deltas

    // --- vecteurs (alloués ici, libérés par le destructeur) --------------
    // Si l'une des lectures échoue, on libère ce qui a déjà été alloué avant
    // de laisser l'exception se propager (le destructeur ne s'exécute pas si
    // le constructeur lève).
    try
    {
        j.at("spot").get_to(spots);
        j.at("volatility").get_to(volatilities);
        j.at("payoff coefficients").get_to(payoffCoeffs);
    }
    catch (...)
    {
        pnl_vect_free(&spots);
        pnl_vect_free(&volatilities);
        pnl_vect_free(&payoffCoeffs);
        throw;
    }

    broadcast_to_dim(spots,        dim);
    broadcast_to_dim(volatilities, dim);
    broadcast_to_dim(payoffCoeffs, dim);
}

PricingInput::~PricingInput()
{
    pnl_vect_free(&spots);
    pnl_vect_free(&volatilities);
    pnl_vect_free(&payoffCoeffs);
}
