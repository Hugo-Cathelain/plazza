///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Kitchen/Kitchen.hpp"
#include "IPC/IIPCChannel.hpp"
#include "IPC/Message.hpp"
#include "IPC/Pipe.hpp"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
/// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Kitchen::Kitchen(
    size_t numberOfCooks,
    float multiplier,
    std::chrono::milliseconds restockTime
)
    : Process(std::bind(&Kitchen::Routine, this))
    , m_multiplier(multiplier)
    , m_stock(restockTime)
    , m_pool()
{
    for (size_t i = 0; i < numberOfCooks; ++i)
    {
        m_cooks.push_back(Cook());
    }

    Start();
}

///////////////////////////////////////////////////////////////////////////////
Kitchen::~Kitchen()
{}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::Routine(void)
{
#ifdef PLAZZA_BONUS
    m_window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(200, 200), "Kitchen", sf::Style::Default
    );

    sf::Event event;
    while (m_window->isOpen())
    {
        while (m_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_window->close();
            }
        }

        m_window->clear();

        m_window->display();
    }
#endif
}

} // !namespace Plazza
