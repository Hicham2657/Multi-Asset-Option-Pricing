#include "option_factory.hpp"

#include <stdexcept>

#include "json_reader.hpp"
#include "Basket.hpp"
#include "AsianOption.hpp"
#include "OptionPerformance.hpp"

std::unique_ptr<Option> make_option(const PricingInput& in)
{
    const std::size_t num_steps = static_cast<std::size_t>(in.fixingDatesNb);

    if (in.optionType == "basket")
        return std::make_unique<Basket>(in.payoffCoeffs, num_steps, in.strike);

    if (in.optionType == "asian")
        return std::make_unique<AsianOption>(in.payoffCoeffs, num_steps, in.strike);

    if (in.optionType == "performance")
        // L'option performance n'a pas de strike (payoff = 1 + somme des
        // performances positives période par période).
        return std::make_unique<OptionPerformance>(in.payoffCoeffs, num_steps);

    throw std::runtime_error("option type inconnu : '" + in.optionType + "'");
}
