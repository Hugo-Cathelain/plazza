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
    , m_id(s_nextId++)
    , m_hasForclosureStarted(false)
{
    Start();
    pipe = std::make_unique<Pipe>(
        std::string(RECEPTION_TO_KITCHEN_PIPE) + "_" + std::to_string(m_id),
        Pipe::OpenMode::WRITE_ONLY
    );
    pipe->Open();

    for (size_t i = 0; i != m_cookCount; i++)
    {
        m_cooks.push_back(std::make_unique<Cook>(*this, *m_stock));
    }
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
            // get pizza order
            // check death
            // return status
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



//TODO: getnextpizza()
//TODO: getnextpizza

///////////////////////////////////////////////////////////////////////////////
size_t Kitchen::GetID(void) const
{
    return (m_id);
}


///////////////////////////////////////////////////////////////////////////////
void Kitchen::SendStatus(void)
{

    Message status = Message::Status{m_id, };
    m_toReception->SendMessage(status);
    // send stocklist
    // passive amount of cooks
    // m_id
    // forclosure timepoint
    // look up cv
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::ForClosure(void)
{
    m_toReception->SendMessage(Message::Closed{m_id});
    m_cooks.clear();
}

} // !namespace Plazza
