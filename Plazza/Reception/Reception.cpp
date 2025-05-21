///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/Reception.hpp"
#include "iostream"

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
Reception::~Reception() {}

///////////////////////////////////////////////////////////////////////////////
void Reception::DisplayStatus(void)
{
    std::cout << "status check: \n";
    int i = 1;

    for (const auto &it : m_kitchens)
    {
        std::cout << "kitchen " << i << ": \n";
        // thing for the cooks.
        // thing for ingredients
    }
}

///////////////////////////////////////////////////////////////////////////////
void Reception::NewKitchen(void)
{
    m_kitchens.push_back(std::make_unique<Kitchen>(m_cookCount, 1.0, m_restockTime));
}

void Reception::RemoveKitchen(Kitchen* kitchen)
{
    auto it = std::remove_if(m_kitchens.begin(), m_kitchens.end(),
        [kitchen](const std::unique_ptr<Kitchen>& ptr) {
            return ptr.get() == kitchen;  // Compare raw pointers
        });

    if (it != m_kitchens.end())
        m_kitchens.erase(it, m_kitchens.end());
}

} // !namespace Plazza
