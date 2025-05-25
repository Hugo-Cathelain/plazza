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
        auto pizza =  m_kitchen.TryGetNextPizza(Milliseconds(100));
        if (pizza && running)
        {
            CookPizza(pizza.value());
        }

        if (!running)
        {
            break;
        }

        std::this_thread::yield();
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Cook::CookPizza(uint16_t packedPizza)
{
    if (!running)
    {
        return (false);
    }

    if (auto pizza = IPizza::Unpack(packedPizza))
    {
        auto ingredients = pizza.value()->GetIngredients();

        if (!m_stock.WaitAndReserveIngredients(ingredients, Seconds(2)))
        {
            if (running)
            {
                m_kitchen.AddPizzaToQueue(packedPizza);
            }
            return (false);
        }

        if (!running)
        {
            return (false);
        }

        m_kitchen.SendStatus();

        m_cooking = true;
        std::this_thread::sleep_for(pizza.value()->GetCookingTime());
        m_cooking = false;

        if (running)
        {
            m_kitchen.NotifyPizzaCompletion(*pizza.value());
        }

        return (true);
    }

    m_cooking = false;
    return (false);
}

///////////////////////////////////////////////////////////////////////////////
bool Cook::IsCooking(void) const
{
    return (m_cooking);
}

} // !namespace Plazza
