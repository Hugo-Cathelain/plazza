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
    std::lock_guard<std::mutex> lock(m_kitchenMutex);
    std::cout << "Pizzeria Status:" << std::endl;

    std::cout << "Kitchen(s): (" << m_kitchens.size() << ')' << std::endl;
    for (const auto& kitchen : m_kitchens)
    {
        const Message::Status& st = kitchen->status;

        std::cout << "\t" << st.id << ":" << std::endl;
        std::cout << "\t\tCooks: " << st.idleCount << "/" << m_cookCount
                  << std::endl;
        std::cout << "\t\tPizza: " << st.pizzaCount << "("
                  << (m_cookCount - st.idleCount) + st.pizzaCount << ")"
                  << std::endl;
        std::cout << "\t\tStock: " << st.stock << std::endl;
        std::cout << "\t\tClosure Time: " << st.timestamp << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
std::optional<std::shared_ptr<Kitchen>> Reception::GetKitchenByID(size_t id)
{
    std::lock_guard<std::mutex> lock(m_kitchenMutex);
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
    std::lock_guard<std::mutex> lock(m_kitchenMutex);
    m_kitchens.push_back(std::make_shared<Kitchen>(
        m_cookCount, 1.0, m_restockTime
    ));
}

///////////////////////////////////////////////////////////////////////////////
void Reception::RemoveKitchen(size_t id)
{
    std::lock_guard<std::mutex> lock(m_kitchenMutex);
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
                std::lock_guard<std::mutex> lock(m_kitchenMutex);
                auto it = std::find_if(m_kitchens.begin(), m_kitchens.end(),
                [target_id = status->id](const std::shared_ptr<Kitchen>& k_ptr)
                {
                    return (k_ptr->GetID() == target_id);
                });
                if (it != m_kitchens.end())
                {
                    (*it)->status = *status;
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
            else if (const auto& closed = message->GetIf<Message::Closed>())
            {
                if (auto kitchen = GetKitchenByID(closed->id))
                {
                    kitchen.value()->pipe->SendMessage(
                        Message::Closed{closed->id}
                    );
                    RemoveKitchen(closed->id);
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
    {
        return;
    }

    bool needsInitialKitchen = false;
    {
        std::lock_guard<std::mutex> lock(m_kitchenMutex);
        if (m_kitchens.empty())
        {
            needsInitialKitchen = true;
        }
    }
    if (needsInitialKitchen)
    {
        CreateKitchen();
    }

    std::vector<Message::Status> allStatus;

    {
        std::lock_guard<std::mutex> lock(m_kitchenMutex);
        for (const auto& kitchen : m_kitchens)
        {
            allStatus.push_back(kitchen->status);
        }
    }

    for (const auto& pizza : orders)
    {
        bool needNewKitchen = true;

        std::sort(allStatus.begin(), allStatus.end(),
        [&](const Message::Status& st1, const Message::Status& st2)
        {
            if (st1.idleCount != st2.idleCount)
            {
                return (st1.idleCount > st2.idleCount);
            }

            if (st1.pizzaCount != st2.pizzaCount)
            {
                return (st1.pizzaCount > st2.pizzaCount);
            }

            return (st1.id < st2.id);
        });

        for (auto& st : allStatus)
        {
            size_t total = m_cookCount - st.idleCount + st.pizzaCount;

            if (total < static_cast<size_t>(1.7 * m_cookCount))
            {
                if (auto kitchen = GetKitchenByID(st.id))
                {
                    std::lock_guard<std::mutex> lock(m_kitchenMutex);
                    kitchen.value()->pipe->SendMessage(Message::Order{
                        st.id, pizza->Pack()
                    });
                }

                if (st.idleCount > 0)
                {
                    st.idleCount--;
                }
                else
                {
                    st.pizzaCount++;
                }
                needNewKitchen = false;
                break;
            }
        }

        if (needNewKitchen)
        {
            CreateKitchen();

            {
                std::lock_guard<std::mutex> lock(m_kitchenMutex);
                allStatus.push_back(m_kitchens.back()->status);
                m_kitchens.back()->pipe->SendMessage(Message::Order{
                    m_kitchens.back()->GetID(), pizza->Pack()
                });
            }

            if (allStatus.back().idleCount > 0)
            {
                allStatus.back().idleCount--;
            }
            else
            {
                allStatus.back().pizzaCount++;
            }
        }
    }
}

#ifdef PLAZZA_BONUS
///////////////////////////////////////////////////////////////////////////////
void Reception::WindowRoutine(void)
{
    const float RECEPTION_HEIGHT = 515.f;
    const float KITCHEN_HEIGHT = 390.f;
    const float WINDOW_WIDTH = 985.f;
    const float SCROLL_SPEED = 30.f;
    const float SCREEN_MARGIN = 200.f;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    const float screenApiHeight = static_cast<float>(desktop.height);

    auto get_view_and_content_heights =
        [&](size_t p_kitchenCount_lambda) -> std::pair<float, float>
    {
        float contentH = RECEPTION_HEIGHT + p_kitchenCount_lambda * KITCHEN_HEIGHT;
        float desiredWindowH = contentH;
        float maxPossibleWindowH = screenApiHeight - SCREEN_MARGIN;
        float windowH = std::min(desiredWindowH, maxPossibleWindowH);
        windowH = std::max(windowH, std::min(RECEPTION_HEIGHT, maxPossibleWindowH));
        windowH = std::max(windowH, 100.f);
        return {contentH, windowH};
    };

    size_t kitchenCountSnapshot = 0;
    {
        std::lock_guard<std::mutex> lock(m_kitchenMutex);
        kitchenCountSnapshot = m_kitchens.size();
    }

    auto initial_dims = get_view_and_content_heights(kitchenCountSnapshot);
    float actualWindowHeight = initial_dims.second;

    sf::RenderWindow window(sf::VideoMode(
        static_cast<unsigned int>(WINDOW_WIDTH),
        static_cast<unsigned int>(actualWindowHeight)
    ), "The Plazza", sf::Style::Default);
    sf::View view;
    sf::Event event;
    sf::Texture backgroundTexture;
    sf::Texture kitchensTexture;

    if (!kitchensTexture.loadFromFile("Assets/Images/Kitchens.png"))
    {
        return;
    }

    if (!backgroundTexture.loadFromFile("Assets/Images/Reception.png"))
    {
        return;
    }

    sf::Sprite kitchenSprite(kitchensTexture);
    sf::Sprite backgroundSprite(backgroundTexture);
    sf::Music music;
    if (!music.openFromFile("Assets/Musics/Game.ogg"))
    {
        return;
    }

    view.setSize(sf::Vector2f(WINDOW_WIDTH, actualWindowHeight));
    if (initial_dims.first <= actualWindowHeight)
    {
        view.setCenter(WINDOW_WIDTH / 2.f, initial_dims.first / 2.f);
    }
    else
    {
        view.setCenter(WINDOW_WIDTH / 2.f, actualWindowHeight / 2.f);
    }
    window.setView(view);
    music.setLoop(true);
    music.play();

    while (m_windowThread.running && !m_shutdown && window.isOpen())
    {
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::MouseWheelScrolled)
            {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                {
                    size_t current_kitchen_count_for_scroll = 0;
                    {
                        std::lock_guard<std::mutex> lock(m_kitchenMutex);
                        current_kitchen_count_for_scroll = m_kitchens.size();
                    }
                    float liveContentHeight = RECEPTION_HEIGHT +
                        current_kitchen_count_for_scroll * KITCHEN_HEIGHT;
                    float currentViewHeight = view.getSize().y;

                    if (liveContentHeight > currentViewHeight)
                    {
                        float scrollDelta = -event.mouseWheelScroll.delta * SCROLL_SPEED;
                        sf::Vector2f center = view.getCenter();
                        float newCenterY = center.y + scrollDelta;
                        float viewHalfHeight = currentViewHeight / 2.f;
                        float minCenterY = viewHalfHeight;
                        float maxCenterY = liveContentHeight - viewHalfHeight;
                        newCenterY = std::max(minCenterY, std::min(newCenterY, maxCenterY));
                        view.setCenter(center.x, newCenterY);
                        window.setView(view);
                    }
                }
            }
        }
        if (!window.isOpen())
        {
            break;
        }

        std::vector<std::shared_ptr<Kitchen>> kitchensToDraw;
        {
            std::lock_guard<std::mutex> lock(m_kitchenMutex);
            kitchensToDraw = m_kitchens;
        }

        if (kitchensToDraw.size() != kitchenCountSnapshot)
        {
            auto new_dims = get_view_and_content_heights(kitchensToDraw.size());
            float newContentTotalHeight = new_dims.first;
            float newActualWindowHeight = new_dims.second;

            sf::Vector2f oldViewCenter = view.getCenter();
            sf::Vector2f oldViewSize = view.getSize();

            window.setSize(sf::Vector2u(
                static_cast<unsigned int>(WINDOW_WIDTH),
                static_cast<unsigned int>(newActualWindowHeight)
            ));
            view.setSize(sf::Vector2f(WINDOW_WIDTH, newActualWindowHeight));

            float oldViewTop = oldViewCenter.y - oldViewSize.y / 2.f;
            float targetNewCenterY = oldViewTop + newActualWindowHeight / 2.f;
            float viewHalfHeightNew = newActualWindowHeight / 2.f;
            float minCenterY = viewHalfHeightNew;
            float maxCenterY = newContentTotalHeight - viewHalfHeightNew;

            if (newContentTotalHeight <= newActualWindowHeight)
            {
                targetNewCenterY = newContentTotalHeight / 2.f;
            }
            else
            {
                targetNewCenterY = std::max(minCenterY,
                    std::min(targetNewCenterY, maxCenterY));
            }
            view.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, targetNewCenterY));
            window.setView(view);
            kitchenCountSnapshot = kitchensToDraw.size();
        }

        backgroundSprite.setPosition(sf::Vector2f(0.0f,
            kitchensToDraw.size() * KITCHEN_HEIGHT));
        window.clear();
        window.draw(backgroundSprite);

        for (size_t i = 0; i < kitchensToDraw.size(); i++)
        {
            kitchenSprite.setPosition(sf::Vector2f(0.0f,
                (kitchensToDraw.size() - 1 - i) * KITCHEN_HEIGHT));
            kitchenSprite.setTextureRect(sf::IntRect(
                sf::Vector2i(0, (kitchensToDraw[i]->GetID() % 20) *
                    static_cast<int>(KITCHEN_HEIGHT)),
                sf::Vector2i(static_cast<int>(WINDOW_WIDTH),
                    static_cast<int>(KITCHEN_HEIGHT))
            ));
            window.draw(kitchenSprite);
        }
        window.display();
    }
    music.stop();
}
#endif

} // !namespace Plazza
