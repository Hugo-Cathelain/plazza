///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Pizza.hpp"

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Pizza::Pizza(PizzaType type, PizzaSize size, int id)
    : type(type)
    , size(size)
    , id(id)
{}

///////////////////////////////////////////////////////////////////////////////
std::optional<Pizza> Pizza::unpack(const std::string& data)
{
    try
    {
        size_t pos1 = data.find(':');
        size_t pos2 = data.rfind(':');

        if (pos1 == std::string::npos || pos2 == std::string::npos || pos1 == pos2)
        {
            return std::nullopt;
        }

        PizzaType type = static_cast<PizzaType>(std::stoi(data.substr(0, pos1)));
        PizzaSize size = static_cast<PizzaSize>(std::stoi(data.substr(pos1 + 1, pos2 - (pos1 + 1))));
        int id = std::stoi(data.substr(pos2 + 1));
        return std::make_optional<Pizza>(type, size, id);
    }
    catch  (...)
    {
        return std::nullopt;
    }
}

///////////////////////////////////////////////////////////////////////////////
std::vector<Ingredient> Pizza::getRequiredIngredients(PizzaType type)
{
    switch (type)
    {
        case PizzaType::Margarita: return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere};
        case PizzaType::Regina:    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere, Ingredient::Ham, Ingredient::Mushrooms};
        case PizzaType::Americana: return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere, Ingredient::Steak};
        case PizzaType::Fantasia:  return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Eggplant, Ingredient::GoatCheese, Ingredient::ChiefLove};
        default: return {};
    }
}

///////////////////////////////////////////////////////////////////////////////
std::chrono::seconds Pizza::getBaseCookingTime(PizzaType type)
{
    switch (type) {
        case PizzaType::Margarita: return std::chrono::seconds(1);
        case PizzaType::Regina:    return std::chrono::seconds(2);
        case PizzaType::Americana: return std::chrono::seconds(2);
        case PizzaType::Fantasia:  return std::chrono::seconds(4);
        default: return std::chrono::seconds(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
std::string Pizza::pack(void) const
{
    return
        std::to_string(static_cast<int>(type)) + ":" +
        std::to_string(static_cast<int>(size)) + ":" +
        std::to_string(id)
    ;
}

} // namespace Plazza
