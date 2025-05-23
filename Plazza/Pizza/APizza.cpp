///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Pizza/APizza.hpp"
#include "Pizza/Regina.hpp"
#include "Pizza/Americana.hpp"
#include "Pizza/Fantasia.hpp"
#include "Pizza/Margarita.hpp"
#include "Errors/InvalidArgument.hpp"
#include <sstream>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
double APizza::s_cookingTimeMultiplier = 1.0f;

///////////////////////////////////////////////////////////////////////////////
APizza::APizza(
    IPizza::Type type,
    IPizza::Size size,
    std::chrono::seconds baseCookingTime,
    const std::vector<Ingredient>& ingredients,
    const std::string& name
)
    : m_type(type)
    , m_size(size)
    , m_baseCookingTime(baseCookingTime)
    , m_ingredients(ingredients)
    , m_name(name)
{}

///////////////////////////////////////////////////////////////////////////////
IPizza::Type APizza::GetType(void) const
{
    return (m_type);
}

///////////////////////////////////////////////////////////////////////////////
IPizza::Size APizza::GetSize(void) const
{
    return (m_size);
}

///////////////////////////////////////////////////////////////////////////////
std::chrono::milliseconds APizza::GetCookingTime(void) const
{
    return (std::chrono::duration_cast<std::chrono::milliseconds>(
        m_baseCookingTime * s_cookingTimeMultiplier
    ));
}

///////////////////////////////////////////////////////////////////////////////
const std::vector<Ingredient>& APizza::GetIngredients(void) const
{
    return (m_ingredients);
}

///////////////////////////////////////////////////////////////////////////////
void APizza::SetCookingTimeMultiplier(double multiplier)
{
    if (multiplier <= 0.0)
    {
        throw InvalidArgument("Cooking time multiplier must be positive");
    }

    s_cookingTimeMultiplier = multiplier;
}

///////////////////////////////////////////////////////////////////////////////
double APizza::GetCookingTimeMultiplier(void)
{
    return (s_cookingTimeMultiplier);
}

///////////////////////////////////////////////////////////////////////////////
uint16_t APizza::Pack(void) const
{
    uint32_t packedData = 0;

    packedData |= (static_cast<uint32_t>(m_type) << 8);
    packedData |= (static_cast<uint32_t>(m_size));
    return (packedData);
}

///////////////////////////////////////////////////////////////////////////////
std::optional<std::unique_ptr<IPizza>> IPizza::Unpack(uint16_t packed)
{
    IPizza::Type type = static_cast<IPizza::Type>((packed >> 8) & 0xFF);
    IPizza::Size size = static_cast<IPizza::Size>(packed & 0xFF);

    std::unique_ptr<IPizza> pizzaUniquePtr;

    switch (type) {
        case IPizza::Type::Regina:
            pizzaUniquePtr = std::make_unique<Regina>(size); break;
        case IPizza::Type::Margarita:
            pizzaUniquePtr = std::make_unique<Margarita>(size); break;
        case IPizza::Type::Americana:
            pizzaUniquePtr = std::make_unique<Americana>(size); break;
        case IPizza::Type::Fantasia:
            pizzaUniquePtr = std::make_unique<Fantasia>(size); break;
        default:
            return (std::nullopt);
    }

    return (std::make_optional(std::move(pizzaUniquePtr)));
}

///////////////////////////////////////////////////////////////////////////////
std::string APizza::SizeToString(IPizza::Size size)
{
    if (size == IPizza::Size::S) return ("S");
    if (size == IPizza::Size::M) return ("M");
    if (size == IPizza::Size::L) return ("L");
    if (size == IPizza::Size::XL) return ("XL");
    if (size == IPizza::Size::XXL) return ("XXL");
    return ("");
}

///////////////////////////////////////////////////////////////////////////////
std::string APizza::ToString(void) const
{
    return (m_name + "(" + SizeToString(m_size) + ")");
}

} // !namespace Plazza
