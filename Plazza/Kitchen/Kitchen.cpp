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
    , m_pizzaTime(0)
    , status{m_id, "5 5 5 5 5 5 5 5 5", 0, numberOfCooks, 0, 0}
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
{
    if (m_isRoutineRunning)
    {
        ForClosure();

        auto timeout = SteadyClock::Now() + Seconds(5);
        while (m_isRoutineRunning && SteadyClock::Now() < timeout)
        {
            std::this_thread::sleep_for(Milliseconds(10));
        }
    }

    if (IsRunning())
    {
        Wait();
    }
}

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
        while (const auto& message = pipe->PollMessage())
        {
            if (message->Is<Message::RequestStatus>())
            {
                SendStatus();
            }
            else if (const auto& order = message->GetIf<Message::Order>())
            {
                AddPizzaToQueue(order->pizza);
            }
            else if (message->Is<Message::Closed>())
            {
                ForClosure();
            }
        }
        ForClosureCheck();

        std::this_thread::sleep_for(Milliseconds(10));
    }

    m_pizzaQueueCV.NotifyAll();
}

///////////////////////////////////////////////////////////////////////////////
size_t Kitchen::GetID(void) const
{
    return (m_id);
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::SendStatus(void)
{
    std::unique_lock<std::mutex> lock(m_pizzaQueueMutex);
    std::string pack = m_stock->Pack();
    Message status = Message::Status{
        m_id,
        pack,
        m_elapsedMs,
        static_cast<size_t>(m_idleCookCount),
        m_pizzaQueue.size(),
        m_pizzaTime
    };

    m_toReception->SendMessage(status);
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::NotifyPizzaCompletion(const IPizza& pizza)
{
    m_pizzaTime -= pizza.GetCookingTime().count();
    SendStatus();
    m_toReception->SendMessage(Message::CookedPizza{m_id, pizza.Pack()});
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::ForClosureCheck(void)
{
    if (!m_isRoutineRunning)
    {
        return;
    }

    m_idleCookCount = 0;

    for (const auto& cooks : m_cooks)
    {
        if (!cooks->IsCooking())
        {
            m_idleCookCount++;
        }
    }

    if (m_idleCookCount != static_cast<int>(m_cookCount))
    {
        m_forclosureTime = SteadyClock::Now();
        m_elapsedMs = 0;
    }
    else
    {
        m_elapsedMs = SteadyClock::DurationToMs(
            SteadyClock::Elapsed(m_forclosureTime, SteadyClock::Now())
        );

        if (m_elapsedMs >= 5000)
        {
            m_toReception->SendMessage(Message::Closed{m_id});
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void Kitchen::ForClosure(void)
{
    m_isRoutineRunning = false;

    for (auto& cook : m_cooks)
    {
        cook->running = false;
    }

    m_pizzaQueueCV.NotifyAll();

    for (auto& cook : m_cooks)
    {
        if (cook->Joinable())
        {
            cook->Join();
        }
    }

    m_cooks.clear();
}

///////////////////////////////////////////////////////////////////////////////
std::optional<uint16_t> Kitchen::TryGetNextPizza(Milliseconds timeout)
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
void Kitchen::AddPizzaToQueue(uint16_t packedPizza)
{
    {
        std::lock_guard<std::mutex> lock(m_pizzaQueueMutex);
        m_pizzaQueue.push(packedPizza);
    }

    if (auto pizza = IPizza::Unpack(packedPizza))
    {
        m_pizzaTime += static_cast<int64_t>(pizza.value()->GetCookingTime().count());
    }
    SendStatus();
    m_pizzaQueueCV.NotifyOne();
}

} // !namespace Plazza
