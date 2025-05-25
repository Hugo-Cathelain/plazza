///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Stock.hpp"
#include "Kitchen/Kitchen.hpp"
#include "Errors/ParsingException.hpp"
#include <iostream>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
/// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Stock::Stock(Milliseconds restockTime, Kitchen& kitchen)
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
std::map<Ingredient, int> Stock::Unpack(const std::string& stockStr)
{
    std::map<Ingredient, int> stock;

    std::stringstream iss(stockStr);

    for (int i = 0; i < static_cast<int>(Ingredient::SIZE); i++)
    {
        int quantity = 0;

        if ((iss >> quantity))
        {
            stock[static_cast<Ingredient>(i)] = quantity;
        }
        else
        {
            throw ParsingException("Invalid packed stock");
        }
    }

    return (stock);
}

///////////////////////////////////////////////////////////////////////////////
std::string Stock::Pack(void) const
{
    std::string buffer;

    for (int i = 0; i < static_cast<int>(Ingredient::SIZE); i++)
    {
        buffer += std::to_string(m_stock.at(static_cast<Ingredient>(i)));

        if (i != static_cast<int>(Ingredient::SIZE) - 1)
        {
            buffer += ' ';
        }
    }

    return (buffer);
}

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
    Milliseconds timeout
)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto deadline = SteadyClock::Now() + timeout;

    while (SteadyClock::Now() < deadline)
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

        m_cv.WaitFor(lock, Milliseconds(100));
    }

    return (false);
}

///////////////////////////////////////////////////////////////////////////////
void Stock::Routine(void)
{
    while (running)
    {
        std::this_thread::sleep_for(m_restockTime);

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_stock)
        {
            pair.second++;
        }
        m_cv.NotifyAll();
        m_kitchen.SendStatus();
    }
}

} // !namespace Plazza
