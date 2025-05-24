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
    , m_forclosureTime(SteadyClock::Now())
    , m_isRoutineRunning(true)
    , m_elapsedMs(0)
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
void Kitchen::RoutineInitialization(void)
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

    for (size_t i = 0; i < m_cookCount; i++)
    {
        m_cooks.push_back(std::make_unique<Cook>(*this, *m_stock));
    }
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::Routine(void)
{
    RoutineInitialization();

    while (m_isRoutineRunning)
    {
        // std::cout << "Kitchen " << m_id << " is running" << std::endl;

        while (const auto& message = pipe->PollMessage())
        {
            if (message->Is<Message::RequestStatus>())
            {
                SendStatus();
                std::cout << "Kitchen " << m_id << " status sent" << std::endl;
            }
            else if (const auto& order = message->GetIf<Message::Order>())
            {
                std::cout << "pizza pizza: " << order << std::endl;
                std::cout << "Kitchen " << m_id << " Order sent" << std::endl;
                AddPizzaToQueue(order->pizza);
            }
        }
        ForClosureCheck();

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    std::cout << "Kitchen " << m_id << " is closing" << std::endl;
    m_pizzaQueueCV.NotifyAll();
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
    std::string pack = m_stock->Pack();
    Message status = Message::Status{
        m_id,
        pack,
        m_elapsedMs,
        static_cast<size_t>(m_idleCookCount),
        m_pizzaQueue.size()
    };

    m_toReception->SendMessage(status);
    // look up cv
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::NotifyPizzaCompletion(const IPizza& pizza)
{
    m_toReception->SendMessage(Message::CookedPizza{m_id, pizza.Pack()});
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::ForClosureCheck(void)
{
    m_idleCookCount = 0;

    for (const auto& cooks : m_cooks)
    {
        if (!cooks->IsCooking())
        {
            m_idleCookCount++;
        }
    }

    if (static_cast<size_t>(m_idleCookCount.load()) != m_cookCount)
    {
        m_forclosureTime = SteadyClock::Now();
        m_elapsedMs = 0;
    }
    if (static_cast<size_t>(m_idleCookCount.load()) == m_cookCount)
    {
    m_elapsedMs = SteadyClock::DurationToMs(
        SteadyClock::Elapsed(m_forclosureTime, SteadyClock::Now()));

        if (m_elapsedMs >= 5000)
        {
            ForClosure();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::ForClosure(void)
{
    m_isRoutineRunning = false;
    m_pizzaQueueCV.NotifyAll();
    m_cooks.clear();
    m_toReception->SendMessage(Message::Closed{m_id});
}

///////////////////////////////////////////////////////////////////////////////
std::optional<uint16_t> Kitchen::TryGetNextPizza(
    std::chrono::milliseconds timeout
)
{
    std::unique_lock<std::mutex> lock(m_pizzaQueueMutex);

    if (m_pizzaQueueCV.GetNativeHandle().wait_for(
        lock,
        timeout,
        [this]
        {
            return (!m_pizzaQueue.empty() || !m_isRoutineRunning);
        })
    )
    {
    if (!m_pizzaQueue.empty())
    {
        uint16_t pizza = m_pizzaQueue.front();
        m_pizzaQueue.pop();
        return (pizza);
    }
    }
    return (std::nullopt);
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::AddPizzaToQueue(uint16_t pizza)
{
    if (!pizza)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_pizzaQueueMutex);
        m_pizzaQueue.push(pizza);
    }
    m_pizzaQueueCV.NotifyOne();
}

} // !namespace Plazza
