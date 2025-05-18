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

    Pipe pipe("/tmp/plazza_reception_to_kitchen", Pipe::OpenMode::WRITE_ONLY);

    pipe.Open();

    Message msg(Message::Type::ORDER_PIZZA);

    msg.SetPayloadFromString("Regina XXL x1; Fantasia S x2");

    pipe << msg;

    pipe.Close();

    pipe.RemoveResource();
}

///////////////////////////////////////////////////////////////////////////////
Kitchen::~Kitchen()
{}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::Routine(void)
{
    Pipe pipe("/tmp/plazza_reception_to_kitchen", Pipe::OpenMode::READ_ONLY);

    pipe.Open();

    Message msg;

    pipe >> msg;

    std::cout << "Kitchen: Received message!" << std::endl;
    std::cout << "Kitchen: Message Type: " << static_cast<int>(msg.type) << std::endl;
    std::cout << "Kitchen: Message Payload: \"" << msg.GetPayloadAsString() << "\"" << std::endl;

    pipe.Close();
}

} // !namespace Plazza
