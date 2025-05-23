///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Stock.hpp"
#include "Kitchen/Kitchen.hpp"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
/// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Stock::Stock(std::chrono::milliseconds restockTime, Kitchen& kitchen)
    : Thread(std::bind(&Stock::Routine, this))
    , m_restockTime(restockTime)
    , m_kitchen(kitchen)
{
    for (int i = 0; i < static_cast<int>(Ingredient::SIZE); i++)
    {
        m_stock[static_cast<Ingredient>(i)] = 5;
    }

    Start();
}

///////////////////////////////////////////////////////////////////////////////
Stock::~Stock()
{}

///////////////////////////////////////////////////////////////////////////////
bool Stock::TryReserveIngredients(const std::vector<Ingredient>& ingredients)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    for (auto ingredient : ingredients)
    {
        if (m_stock[ingredient] <= 0)
        {
            return (false);
        }
    }

    for (auto ingredient : ingredients)
    {
        m_stock[ingredient]--;
    }

    return (true);
}

///////////////////////////////////////////////////////////////////////////////
bool Stock::WaitAndReserveIngredients(
    const std::vector<Ingredient>& ingredients,
    std::chrono::milliseconds timeout
)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        bool canReserve = true;
        for (auto ingredient : ingredients)
        {
            if (m_stock[ingredient] <= 0)
            {
                canReserve = false;
                break;
            }
        }

        if (canReserve)
        {
            for (auto ingredient : ingredients)
            {
                m_stock[ingredient]--;
            }
            return (true);
        }

        m_cv.WaitFor(lock, std::chrono::milliseconds(100));
    }

    return (false);
}

///////////////////////////////////////////////////////////////////////////////
void Stock::Routine(void)
{
    while (m_running)
    {
        std::this_thread::sleep_for(m_restockTime);

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_stock)
        {
            pair.second++;
        }
        m_cv.NotifyAll();
    }
}

} // !namespace Plazza
