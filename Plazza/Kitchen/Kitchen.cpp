///////////////////////////////////////////////////////////////////////////////
/// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Kitchen/Kitchen.hpp"
#include "IPC/Message.hpp"
#include "IPC/Pipe.hpp"
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
{
    Start();
    pipe = std::make_unique<Pipe>(RECEPTION_TO_KITCHEN_PIPE + m_id, Pipe::OpenMode::WRITE_ONLY);

}

///////////////////////////////////////////////////////////////////////////////
Kitchen::~Kitchen()
{}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::Routine(void)
{
    std::cout << "Hello From Kitchen" << std::endl;
    m_stock = std::make_unique<Stock>(m_restockTime, *this);
    KitchenForclosed();
}

///////////////////////////////////////////////////////////////////////////////
size_t Kitchen::getId(void)
{
    return (m_id);
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::KitchenForclosed(void)
{
    pipe = std::make_unique<Pipe>(RECEPTION_TO_KITCHEN_PIPE + m_id, Pipe::OpenMode::READ_ONLY);
    m_toReception = std::make_unique<Pipe>(KITCHEN_TO_RECEPTION_PIPE, Pipe::OpenMode::WRITE_ONLY);

    m_toReception->Open();
    m_toReception->SendMessage(Message::Closed{});
    m_toReception->Close();
}

} // !namespace Plazza
