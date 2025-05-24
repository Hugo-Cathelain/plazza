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
    // TODO: Add logic to sparse the command to all the kitchen

    if (orders.empty())
        return;

    std::cout << "Processing " << orders.size() << " orders..." << std::endl;

    // Poll all kitchens for their current status
    std::vector<size_t> availableKitchens = {};
    for (const auto& kitchen : m_kitchens) {
        auto Status = kitchen->status;

        if (Status.stock.empty()) {
            continue;
        }
        availableKitchens.push_back(kitchen->GetID());
    }

    std::cout << "Available kitchens: ";
    for (const auto& kitchenId : availableKitchens) {
        std::cout << kitchenId << " \n";
    }

    // Check if we need to create a new kitchen based on order load
    int numberOfKitchens = availableKitchens.size();
    bool needNewKitchen = false;
    int totalCooks = numberOfKitchens * m_cookCount;
    std::cout << "Total cooks: " << totalCooks << std::endl;
    std::cout << "Orders: " << orders.size() << std::endl;
    // If orders exceed 1.6 * available cooks, create a new kitchen
    if (orders.size() > static_cast<size_t>(1.6 * totalCooks)) {
        needNewKitchen = true;
    }
    std::cout << "Need new kitchen: " << (needNewKitchen ? "Yes" : "No") << std::endl;
    if (needNewKitchen) {
        CreateKitchen();
    }
    m_kitchens.back()->pipe->SendMessage(Message::Order{
        m_kitchens.back()->GetID(),
        orders[0]->Pack()
    });

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

///////////////////////////////////////////////////////////////////////////////
// void Reception::ProcessOrders(const Parser::Orders& orders)
// {
//     if (orders.empty())
//         return;

//     std::cout << "Processing " << orders.size() << " orders..." << std::endl;

//     // Poll all kitchens for their current status
//     std::vector<size_t> availableKitchens;
//     for (const auto& kitchen : m_kitchens) {
//         kitchen->pipe->SendMessage(Message::RequestStatus{});

//         // In a real implementation, we would wait for and process responses here
//         // For now, assume all kitchens are potentially available
//         availableKitchens.push_back(kitchen->GetID());
//     }

//     // Check if we need to create a new kitchen based on order load
//     bool needNewKitchen = false;
//     size_t totalCooks = m_kitchens.size() * m_cookCount;
//     // If orders exceed 1.6 * available cooks, create a new kitchen
//     if (orders.size() > static_cast<size_t>(1.6 * totalCooks)) {
//         needNewKitchen = true;
//     }

//     // Process each order
//     size_t orderIndex = 0;
//     for (const auto& pizza : orders) {
//         bool orderAssigned = false;

//         // First attempt: Try to assign to kitchens with available ingredients
//         for (size_t kitchenId : availableKitchens) {
//             auto kitchen = GetKitchenByID(kitchenId);
//             if (!kitchen)
//                 continue;

//             // In a real implementation, we would check if the kitchen has the ingredients
//             // based on the status response we received

//             // Send the order to the kitchen
//             kitchen->pipe->SendMessage(Message::Order{
//                 kitchenId,
//                 pizza->Pack()
//             });

//             std::cout << "Sent " << static_cast<int>(pizza->GetType())
//                       << " size " << static_cast<int>(pizza->GetSize())
//                       << " to kitchen " << kitchenId << std::endl;

//             orderAssigned = true;
//             break;
//         }

//         // If order couldn't be assigned to any kitchen, create a new one if needed
//         if (!orderAssigned && needNewKitchen) {
//             CreateKitchen();
//             size_t newKitchenId = m_kitchens.back()->GetID();

//             m_kitchens.back()->pipe->SendMessage(Message::Order{
//                 newKitchenId,
//                 pizza->Pack()
//             });

//             std::cout << "Created new kitchen " << newKitchenId
//                       << " for unassigned order" << std::endl;

//             // Only create one kitchen per batch of orders
//             needNewKitchen = false;
//         }

//         // Handle the case where we couldn't assign the order
//         if (!orderAssigned && !needNewKitchen) {
//             // In a real implementation, we might queue this order for later
//             std::cout << "Warning: Order #" << orderIndex
//                       << " could not be assigned to any kitchen" << std::endl;
//         }

//         orderIndex++;
//     }

//     // Poll for any messages from kitchens (cooked pizzas, etc.)
//     while (auto message = m_pipe->PollMessage()) {
//         if (message->Is<Message::CookedPizza>()) {
//             auto* cookedPizza = message->GetIf<Message::CookedPizza>();
//             if (cookedPizza) {
//                 std::cout << "Received cooked pizza notification from kitchen "
//                           << cookedPizza->id << std::endl;
//             }
//         } else if (message->Is<Message::Status>()) {
//             auto* status = message->GetIf<Message::Status>();
//             if (status) {
//                 std::cout << "Received status update from kitchen "
//                           << status->id << ": " << status->stock << std::endl;
//             }
//         } else if (message->Is<Message::Closed>()) {
//             auto* closed = message->GetIf<Message::Closed>();
//             if (closed) {
//                 std::cout << "Kitchen " << closed->id << " has closed" << std::endl;
//                 RemoveKitchen(closed->id);
//             }
//         }
//     }
// }

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

    if (window.isOpen())
    {
        window.close();
    }
}
#endif


} // !namespace Plazza
