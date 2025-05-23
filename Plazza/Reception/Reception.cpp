///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/Reception.hpp"
#include "iostream"
#include "IPC/Message.hpp"
#include "IPC/Pipe.hpp"

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Reception::Reception(std::chrono::milliseconds restockTime, size_t CookCount)
    : m_restockTime(restockTime)
    , m_cookCount(CookCount)
{}

///////////////////////////////////////////////////////////////////////////////
Reception::~Reception()
{}

///////////////////////////////////////////////////////////////////////////////
void Reception::DisplayStatus(void)
{
    std::cout << "Pizzeria Status:" << std::endl;

    std::cout << "Kitchen(s): (" << m_kitchens.size() << ')' << std::endl;
    for (const auto& kitchen : m_kitchens)
    {
        // TODO: thing to do here
        (void)kitchen;
    }
}

///////////////////////////////////////////////////////////////////////////////
std::optional<std::unique_ptr<Kitchen>&> Reception::GetKitchenByID(size_t id)
{
    auto it = std::find_if(m_kitchens.begin(), m_kitchens.end(),
        [id](const std::unique_ptr<Kitchen>& kitchen) {
            return (kitchen->getId() == id);
        });

    if (it != m_kitchens.end())
    {
        return (*it);
    }
    return (std::nullopt);
}

///////////////////////////////////////////////////////////////////////////////
void Reception::CreateKitchen(void)
{
    m_kitchens.push_back(std::make_unique<Kitchen>(
        m_cookCount, 1.0, m_restockTime
    ));
}

///////////////////////////////////////////////////////////////////////////////
void Reception::RemoveKitchen(size_t id)
{
    m_kitchens.erase(std::remove_if(m_kitchens.begin(), m_kitchens.end(),
        [id](const std::unique_ptr<Kitchen>& kitchen) {
            return (kitchen->getId() == id);
        }),
        m_kitchens.end()
    );
}

///////////////////////////////////////////////////////////////////////////////
void Reception::ProcessOrders(const Parser::Orders& orders)
{
    (void)orders;
    // TODO: Add logic to sparse the command to all the kitchen
    //
    // Use the kitchen status to see the current capacity of each kitchen
    // in term of available cooks, or ingredients. Do not dispatch a pizza
    // if no ingredient are ready for it
    // Avoid creating a kitchen when every kitchen is saturated, the default
    // Calculation for the saturation is 2 * N cooks, but for the real saturation
    // we will use something like 1.6 * N cooks to open a new kitchen.
    // Do not create a lot of kitchen, always keep in mind a ratio: pizza/cook/kitchen
    // Prioriterize the kitchen with the available ingredients in the stock
    // Stock > Cook > new Kitchen
    // For the cooks, use a rotation algorithm to avoid deadlocks, each cook can
    // grab ingredient and can ask the other for pending ingredient.
    // There will always be pending cooks without any ingredient but that do not matter
    // In a kitchen with 5 cooks, only 3 really works and the other two are waiting
    // for the ingredients/
}

} // !namespace Plazza
