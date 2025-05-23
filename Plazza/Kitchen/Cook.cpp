///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Kitchen/Cook.hpp"
#include "Kitchen/Kitchen.hpp"
#include "Kitchen/Stock.hpp"

///////////////////////////////////////////////////////////////////////////////
/// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Cook::Cook(Kitchen& kitchen, Stock& stock)
    : Thread(std::bind(&Cook::Routine, this))
    , m_kitchen(kitchen)
    , m_stock(stock)
    , m_cooking(false)
{
    Start();
}

///////////////////////////////////////////////////////////////////////////////
void Cook::Routine(void)
{
    while (running)
    {
        if (true) // Get next pizza with 1sec timeout
        {
            // CookPizza();
        }

        std::this_thread::yield();
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Cook::CookPizza(const IPizza& pizza)
{
    auto ingredients = pizza.GetIngredients();

    if (!m_stock.WaitAndReserveIngredients(ingredients, std::chrono::seconds(2)))
    {
        // Return pizza to queue if ingredients unavailable
        // m_kitchen.ReturnPizza(pizza);
        return (false);
    }

    m_cooking = true;
    std::this_thread::sleep_for(pizza.GetCookingTime());
    m_cooking = false;
    m_kitchen.NotifyPizzaCompletion(pizza);

    return (true);
}

} // !namespace Plazza
