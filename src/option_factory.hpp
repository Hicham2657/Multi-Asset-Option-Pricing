#pragma once

#include <memory>
#include "Option.hpp"

struct PricingInput;

/// Construit l'objet `Option` correspondant à `in.optionType`.
///
///   "basket"      -> Basket
///   "asian"       -> AsianOption
///   "performance" -> OptionPerformance  (sans strike)
///
/// Lève `std::runtime_error` si le type est inconnu.
std::unique_ptr<Option> make_option(const PricingInput& in);
