///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Kitchen/Kitchen.hpp"
#include "IPC/Message.hpp"
#include "IPC/Pipe.hpp"
#include "Pizza/PizzaFactory.hpp"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
/// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
size_t Kitchen::s_nextId = 0;

///////////////////////////////////////////////////////////////////////////////
Kitchen::Kitchen(
    size_t numberOfCooks,
    double multiplier,
    std::chrono::milliseconds restockTime
)
    : Process(std::bind(&Kitchen::Routine, this))
    , m_restockTime(restockTime)
    , m_multiplier(multiplier)
    , m_cookCount(numberOfCooks)
    , m_id(s_nextId++)
{
    Start();
    pipe = std::make_unique<Pipe>(
        std::string(RECEPTION_TO_KITCHEN_PIPE) + "_" + std::to_string(m_id),
        Pipe::OpenMode::WRITE_ONLY
    );
    pipe->Open();
}

///////////////////////////////////////////////////////////////////////////////
Kitchen::~Kitchen()
{}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::Routine(void)
{
    m_stock = std::make_unique<Stock>(m_restockTime, *this);
    pipe = std::make_unique<Pipe>(
        std::string(RECEPTION_TO_KITCHEN_PIPE) + "_" + std::to_string(m_id),
        Pipe::OpenMode::READ_ONLY
    );
    m_toReception = std::make_unique<Pipe>(
        std::string(KITCHEN_TO_RECEPTION_PIPE),
        Pipe::OpenMode::WRITE_ONLY
    );

    std::cout << "Kitchen " << m_id << " is started" << std::endl;

    pipe->Open();
    m_toReception->Open();

    int cycle = 0;
    while (cycle < 10)
    {
        std::cout << "Kitchen " << m_id << " is running" << std::endl;

        while (const auto& message = pipe->PollMessage())
        {
            if (message->Is<Message::RequestStatus>())
            {
                m_toReception->SendMessage(Message::Status{
                    m_id, std::to_string(cycle)
                });
                std::cout << "Kitchen " << m_id << " status sent" << std::endl;
            }
        }

        if (cycle == 5)
        {
            PizzaFactory& factory = PizzaFactory::GetInstance();
            m_toReception->SendMessage(Message::CookedPizza{
                m_id,
                factory.CreatePizza("regina", IPizza::Size::XL)->Pack()
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        cycle++;
    }

    std::cout << "Kitchen " << m_id << " is closing" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
size_t Kitchen::GetID(void) const
{
    return (m_id);
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::NotifyPizzaCompletion(const IPizza& pizza)
{
    m_toReception->SendMessage(Message::CookedPizza{m_id, pizza.Pack()});
}

} // !namespace Plazza
