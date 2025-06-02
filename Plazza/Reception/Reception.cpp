///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/Reception.hpp"
#include "iostream"
#include "IPC/Message.hpp"
#include "IPC/Pipe.hpp"
#include "Pizza/APizza.hpp"
#include "Utils/Logger.hpp"
#include <math.h>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Reception::Reception(Milliseconds restockTime, size_t CookCount)
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
        std::cout << "\t\tPizza Completion Time : " << st.pizzaTime << std::endl;
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

    Logger::Info(
        "KITCHEN",
        "New kitchen created: " + std::to_string(m_kitchens.back()->GetID())
    );
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
    Logger::Info(
        "KITCHEN",
        "Kitchen closed: " + std::to_string(id)
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

                    Logger::Info(
                        "RECEPTION",
                        msg + " Cooked by " + std::to_string(cooked->id)
                    );
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

        std::this_thread::sleep_for(Milliseconds(10));
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

            if (st1.pizzaTime != st2.pizzaTime)
            {
                return (st1.pizzaTime < st2.pizzaTime);
            }

            return (st1.id < st2.id);
        });

        for (auto& st : allStatus)
        {
            size_t total = m_cookCount - st.idleCount + st.pizzaCount;

            if (total < static_cast<size_t>(2.0 * m_cookCount))
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
                Logger::Debug(
                    "RECEPTION",
                    pizza->ToString() + " dispatched to kitchen " +
                    std::to_string(st.id)
                );
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

            Logger::Debug(
                "RECEPTION",
                pizza->ToString() + " dispatched to kitchen "
                + std::to_string(allStatus.back().id)
            );
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

    // Z particle system for sleeping effect
    struct ZParticle {
        sf::Vector2f position;
        float alpha;
        float age;
        size_t kitchenIndex;
    };
    std::vector<ZParticle> zParticles;
    const float Z_SPAWN_INTERVAL = 0.8f; // New Z every 0.8 seconds
    const float Z_LIFETIME = 2.4f; // Z lives for 2.4 seconds (3 Zs max at 0.8s intervals)
    float lastZSpawn = 0.0f;

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
    sf::Clock clock;
    sf::View view;
    sf::Event event;
    sf::Texture backgroundTexture;
    sf::Texture kitchensTexture;
    sf::Texture cookTexture;
    sf::Font font;

    if (!kitchensTexture.loadFromFile("Assets/Images/Kitchens.png"))
    {
        return;
    }

    if (!backgroundTexture.loadFromFile("Assets/Images/Reception.png"))
    {
        return;
    }

    if (!cookTexture.loadFromFile("Assets/Images/Cooks.png"))
    {
        return;
    }

    if (!font.loadFromFile("Assets/Fonts/lilitaone.ttf"))
    {
        return;
    }

    sf::Sprite kitchenSprite(kitchensTexture);
    sf::Sprite backgroundSprite(backgroundTexture);
    sf::Sprite cookSprite(cookTexture);
    sf::Music music;
    sf::Text text;

    text.setFont(font);
    text.setFillColor(sf::Color::White);

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
    window.setFramerateLimit(60);
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

        std::vector<Message::Status> kitchenStatuses;
        {
            std::lock_guard<std::mutex> lock(m_kitchenMutex);
            for (const auto& kitchen : m_kitchens)
            {
                kitchenStatuses.push_back(kitchen->status);
            }
        }

        if (kitchenStatuses.size() != kitchenCountSnapshot)
        {
            auto new_dims = get_view_and_content_heights(kitchenStatuses.size());
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
            kitchenCountSnapshot = kitchenStatuses.size();
        }

        float time = clock.getElapsedTime().asSeconds();

        // Update Z particles
        for (auto it = zParticles.begin(); it != zParticles.end();)
        {
            it->age += 1.0f / 60.0f; // Assuming 60 FPS
            it->position.y -= 20.0f / 60.0f; // Move up slowly
            it->position.x += std::sin(it->age * 3.0f) * 10.0f / 60.0f; // Slight horizontal drift
            it->alpha = 1.0f - (it->age / Z_LIFETIME); // Fade out over time

            if (it->age >= Z_LIFETIME)
            {
                it = zParticles.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Spawn new Z particles for idling cooks
        if (time - lastZSpawn >= Z_SPAWN_INTERVAL)
        {
            for (size_t i = 0; i < kitchenStatuses.size(); i++)
            {
                sf::Vector2f offset = sf::Vector2f(
                    0.0f, (kitchenStatuses.size() - 1 - i) * KITCHEN_HEIGHT
                );

                ZParticle newZ;
                newZ.position = offset + sf::Vector2f(WINDOW_WIDTH - 380, KITCHEN_HEIGHT - 230);
                newZ.alpha = 1.0f;
                newZ.age = 0.0f;
                newZ.kitchenIndex = i;
                zParticles.push_back(newZ);
            }
            lastZSpawn = time;
        }

        backgroundSprite.setPosition(sf::Vector2f(0.0f,
            kitchenStatuses.size() * KITCHEN_HEIGHT));
        window.clear();
        window.draw(backgroundSprite);

        static const sf::IntRect DESK_RECT({135, 168}, {122, 64});
        static const sf::IntRect BODY_RECT({0, 168}, {57, 55});
        static const sf::IntRect RHAND_RECT({57, 168}, {23, 35});
        static const sf::IntRect LHAND_RECT({80, 168}, {23, 35});
        static const sf::IntRect ZLTRS_RECT({103, 168}, {32, 32});
        static const sf::IntRect EXCLA_RECT({257, 168}, {20, 40});

        for (size_t i = 0; i < kitchenStatuses.size(); i++)
        {
            sf::Vector2f offset = sf::Vector2f(
                0.0f, (kitchenStatuses.size() - 1 - i) * KITCHEN_HEIGHT
            );

            float breathingOffset = std::sin(time * 2.5f + i * 0.5f) * 3.0f;
            float cookingOffset = std::sin(time * 7.5f + i * 0.7f) * 5.0f;

            kitchenSprite.setPosition(offset);
            kitchenSprite.setTextureRect(sf::IntRect(
                sf::Vector2i(0, (kitchenStatuses[i].id % 20) *
                    static_cast<int>(KITCHEN_HEIGHT)),
                sf::Vector2i(static_cast<int>(WINDOW_WIDTH),
                    static_cast<int>(KITCHEN_HEIGHT))
            ));
            window.draw(kitchenSprite);

            // Kitchen ID
            text.setPosition(offset);
            text.setString(std::to_string(kitchenStatuses[i].id));
            window.draw(text);

            // Idling Cooks
                // body
            cookSprite.setTextureRect(BODY_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 390, KITCHEN_HEIGHT - 148));
            window.draw(cookSprite);
                // left hand
            cookSprite.setTextureRect(RHAND_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 394, KITCHEN_HEIGHT - 136));
            window.draw(cookSprite);
                // right hand
            cookSprite.setTextureRect(LHAND_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 352, KITCHEN_HEIGHT - 136));
            window.draw(cookSprite);
                // head
            cookSprite.setTextureRect(sf::IntRect({78 * ((int)kitchenStatuses[i].id % 8), 0}, {78, 84}));
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 404, KITCHEN_HEIGHT - 212 + breathingOffset));
            window.draw(cookSprite);
                // desk
            cookSprite.setTextureRect(DESK_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 422, KITCHEN_HEIGHT - 108));
            window.draw(cookSprite);
                // data
            text.setString(std::to_string(kitchenStatuses[i].idleCount));
            sf::FloatRect bound = text.getLocalBounds();
            text.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 422 + ((122 - bound.width) / 2.f), KITCHEN_HEIGHT - 82));
            window.draw(text);

            // Active Cooks
                // body
            cookSprite.setTextureRect(BODY_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 212, KITCHEN_HEIGHT - 148));
            window.draw(cookSprite);
                // left hand
            cookSprite.setTextureRect(RHAND_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 216, KITCHEN_HEIGHT - 136 - cookingOffset));
            window.draw(cookSprite);
                // right hand
            cookSprite.setTextureRect(LHAND_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 174, KITCHEN_HEIGHT - 136 + cookingOffset));
            window.draw(cookSprite);
                // head
            cookSprite.setTextureRect(sf::IntRect({78 * ((int)kitchenStatuses[i].id % 8), 84}, {78, 84}));
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 226, KITCHEN_HEIGHT - 212 + breathingOffset));
            window.draw(cookSprite);
                // desk
            cookSprite.setTextureRect(DESK_RECT);
            cookSprite.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 244, KITCHEN_HEIGHT - 108));
            window.draw(cookSprite);
                // data
            text.setString(std::to_string(m_cookCount - kitchenStatuses[i].idleCount));
            bound = text.getLocalBounds();
            text.setPosition(offset + sf::Vector2f(WINDOW_WIDTH - 244 + ((122 - bound.width) / 2.f), KITCHEN_HEIGHT - 82));
            window.draw(text);
        }

        // Draw Z particles
        for (const auto& z : zParticles)
        {
            cookSprite.setTextureRect(ZLTRS_RECT);
            cookSprite.setPosition(z.position);
            cookSprite.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(z.alpha * 255)));
            window.draw(cookSprite);
        }

        // Reset sprite color for next frame
        cookSprite.setColor(sf::Color::White);

        window.display();
    }
    music.stop();
}
#endif

} // !namespace Plazza
