///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/Reception.hpp"
#include "iostream"
#include "IPC/Message.hpp"
#include "IPC/Pipe.hpp"
#include "Pizza/APizza.hpp"

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Reception::Reception(std::chrono::milliseconds restockTime, size_t CookCount)
    : m_restockTime(restockTime)
    , m_cookCount(CookCount)
    , m_pipe(std::make_unique<Pipe>(
        KITCHEN_TO_RECEPTION_PIPE,
        Pipe::OpenMode::READ_ONLY
    ))
    , m_manager(std::bind(&Reception::ManagerThread, this))
    , m_shutdown(false)
{
    m_pipe->Open();
    m_manager.Start();
    CreateKitchen();
}

///////////////////////////////////////////////////////////////////////////////
Reception::~Reception()
{
    m_shutdown = true;
    m_manager.running = false;

    if (m_manager.Joinable())
    {
        m_manager.Join();
    }
}

///////////////////////////////////////////////////////////////////////////////
void Reception::DisplayStatus(void)
{
    std::cout << "Pizzeria Status:" << std::endl;

    std::cout << "Kitchen(s): (" << m_kitchens.size() << ')' << std::endl;
    for (const auto& kitchen : m_kitchens)
    {
        kitchen->pipe->SendMessage(Message::RequestStatus{});
        // TODO: thing to do here
        (void)kitchen;
    }
}

///////////////////////////////////////////////////////////////////////////////
std::optional<std::shared_ptr<Kitchen>> Reception::GetKitchenByID(size_t id)
{
    auto it = std::find_if(m_kitchens.begin(), m_kitchens.end(),
        [id](const std::shared_ptr<Kitchen>& kitchen) {
            return (kitchen->GetID() == id);
        });

    if (it != m_kitchens.end())
    {
        return (*it);
    }
    return (std::nullopt);
}

///////////////////////////////////////////////////////////////////////////////
void Reception::CreateKitchen(void)
{
    m_kitchens.push_back(std::make_shared<Kitchen>(
        m_cookCount, 1.0, m_restockTime
    ));
}

///////////////////////////////////////////////////////////////////////////////
void Reception::RemoveKitchen(size_t id)
{
    m_kitchens.erase(std::remove_if(m_kitchens.begin(), m_kitchens.end(),
        [id](const std::shared_ptr<Kitchen>& kitchen) {
            return (kitchen->GetID() == id);
        }),
        m_kitchens.end()
    );
}

///////////////////////////////////////////////////////////////////////////////
void Reception::ManagerThread(void)
{
    while (m_manager.running && !m_shutdown)
    {
        while (const auto& message = m_pipe->PollMessage())
        {
            if (const auto& status = message->GetIf<Message::Status>())
            {
                if (auto kitchen = GetKitchenByID(status->id))
                {
                    kitchen.value()->status = *status;
                }
            }
            else if (const auto& cooked = message->GetIf<Message::CookedPizza>())
            {
                if (auto pizza = APizza::Unpack(cooked->pizza))
                {
                    std::string msg = pizza.value()->ToString();

                    msg = (msg[0] == 'E' ? "An " : "A ") + msg + " is ready!";

                    std::cout << msg << std::endl;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

///////////////////////////////////////////////////////////////////////////////
void Reception::ProcessOrders(const Parser::Orders& orders)
{
    (void)orders;
    // TODO: Add logic to sparse the command to all the kitchen
    //
    // Use the kitchen status to see the current capacity of each kitchen
    // in term of available cooks, or ingredients. Do not dispatch a pizza
    // if no ingredient are ready for it
    // Avoid creating a kitchen when every kitchen is saturated, the default
    // Calculation for the saturation is 2 * N cooks, but for the real saturation
    // we will use something like 1.6 * N cooks to open a new kitchen.
    // Do not create a lot of kitchen, always keep in mind a ratio: pizza/cook/kitchen
    // Prioriterize the kitchen with the available ingredients in the stock
    // Stock > Cook > new Kitchen
    // For the cooks, use a rotation algorithm to avoid deadlocks, each cook can
    // grab ingredient and can ask the other for pending ingredient.
    // There will always be pending cooks without any ingredient but that do not matter
    // In a kitchen with 5 cooks, only 3 really works and the other two are waiting
    // for the ingredients/
}

} // !namespace Plazza
