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
#ifdef PLAZZA_BONUS
    , m_windowThread(std::bind(&Reception::WindowRoutine, this))
#endif
{
    m_pipe->Open();
    m_manager.Start();
#ifdef PLAZZA_BONUS
    m_windowThread.Start();
#endif
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
    if (orders.empty())
        return;

    for (const auto& pizza : orders) {
        size_t totalProcessingPizzas = 0;

        bool needNewKitchen = true;
        for (const auto& kitchen : m_kitchens) {
            totalProcessingPizzas = kitchen->status.pizzaCount;
            if (totalProcessingPizzas < static_cast<size_t>(2 * m_cookCount)) {
                kitchen->pipe->SendMessage(Message::Order{
                    kitchen->GetID(),
                    pizza->Pack()
                });
                needNewKitchen = false;
                break;
            }
        }

        if (needNewKitchen) {
            CreateKitchen();
            m_kitchens.back()->pipe->SendMessage(Message::Order{
                m_kitchens.back()->GetID(),
                pizza->Pack()
            });
            continue;
        }
    }
}

#ifdef PLAZZA_BONUS
///////////////////////////////////////////////////////////////////////////////
void Reception::WindowRoutine(void)
{
    sf::RenderWindow window(sf::VideoMode(985, 515), "Reception");
    sf::Event event;
    sf::Texture background;

    if (!background.loadFromFile("Assets/Images/Reception.png"))
    {
        return;
    }

    sf::Sprite sprite(background);
    sf::Music music;

    if (!music.openFromFile("Assets/Musics/Menu.ogg"))
    {
        return;
    }

    music.setLoop(true);
    music.play();

    while (m_windowThread.running && !m_shutdown)
    {
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {}
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    music.stop();

    if (window.isOpen())
    {
        window.close();
    }
}
#endif

} // !namespace Plazza
