#include "option_factory.hpp"

#include <stdexcept>

#include "json_reader.hpp"
#include "Basket.hpp"
#include "AsianOption.hpp"

std::unique_ptr<Option> make_option(const PricingInput& in)
{
    const std::size_t num_steps = static_cast<std::size_t>(in.fixingDatesNb);

    if (in.optionType == "basket")
        return std::make_unique<Basket>(in.payoffCoeffs, num_steps, in.strike);

    if (in.optionType == "asian")
        return std::make_unique<AsianOption>(in.payoffCoeffs, num_steps, in.strike);

    if (in.optionType == "performance")
        throw std::runtime_error(
            "option type 'performance' non implementee : creer une classe "
            "PerformanceOption (voir manquants/pricer.pdf) et l'ajouter dans "
            "make_option().");

    throw std::runtime_error("option type inconnu : '" + in.optionType + "'");
}
