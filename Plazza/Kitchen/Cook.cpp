///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Kitchen/Cook.hpp"
#include "Kitchen/Kitchen.hpp"
#include "Kitchen/Stock.hpp"
#include <iostream>

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
        if (auto pizza =  m_kitchen.TryGetNextPizza(std::chrono::seconds(1)))
        {
            CookPizza(pizza.value());
        }

        std::this_thread::yield();
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Cook::CookPizza(uint16_t packedPizza)
{
    if (auto pizza = IPizza::Unpack(packedPizza))
    {
        auto ingredients = pizza.value()->GetIngredients();

        if (!m_stock.WaitAndReserveIngredients(ingredients, std::chrono::seconds(2)))
        {
            m_kitchen.AddPizzaToQueue(packedPizza);
            return (false);
        }

        m_cooking = true;
        std::this_thread::sleep_for(pizza.value()->GetCookingTime());
        m_cooking = false;
        m_kitchen.NotifyPizzaCompletion(**pizza);

        return (true);
    }
    else
    {
        m_cooking = false;
        return (false);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Cook::IsCooking(void) const
{
    return (m_cooking);
}

} // !namespace Plazza
