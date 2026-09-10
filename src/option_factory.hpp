#pragma once

#include <memory>
#include "Option.hpp"

struct PricingInput;

/// Construit l'objet `Option` correspondant à `in.optionType`.
///
///   "basket"      -> Basket
///   "asian"       -> AsianOption
///   "performance" -> NON IMPLÉMENTÉ (lève std::runtime_error) : il reste à
///                    écrire une classe PerformanceOption (payoff décrit dans
///                    manquants/pricer.pdf, section « Option performance sur
///                    panier ») puis à l'ajouter ici.
///
/// Lève `std::runtime_error` si le type est inconnu ou non implémenté.
std::unique_ptr<Option> make_option(const PricingInput& in);
