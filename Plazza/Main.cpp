///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/CLI.hpp"
#include "Kitchen/Kitchen.hpp"
#include "Pizza/APizza.hpp"
#include "Errors/InvalidArgument.hpp"
#include <iostream>
#include "Utils/Timer.hpp"

// #include "Concurrency/Process.hpp"
// #include "IPC/Message.hpp"
// #include "IPC/Pipe.hpp"

// static std::string KITCHEN_TO_RECEPTION_PIPE = "/tmp/plazza_kitchen_to_reception_pipe";
// static std::string RECEPTION_TO_KITCHEN_PIPE = "/tmp/plazza_reception_to_kitchen_pipe";

// using namespace Plazza;

// void routine(void)
// {
//     Pipe read(RECEPTION_TO_KITCHEN_PIPE, Pipe::OpenMode::READ_ONLY);

//     read.Open();

//     while (true)
//     {
//         while (const auto& message = read.PollMessage())
//         {
//             std::cout << "Received Message" << std::endl;
//             if (auto status = message->GetIf<Message::Status>())
//             {
//                 std::cout << "Id: " << status->id << std::endl;
//                 std::cout << "Stock: " << status->stock << std::endl;
//             }
//             if (message->Is<Message::Closed>())
//             {
//                 return;
//             }
//         }
//     }
// }

// int main(void)
// {
//     Process kitchen(routine);

//     kitchen.Start();

//     Pipe write(RECEPTION_TO_KITCHEN_PIPE, Pipe::OpenMode::WRITE_ONLY);

//     write.Open();

//     write.SendMessage(Message(Message::Status{5, "Hello World"}));

//     std::this_thread::sleep_for(std::chrono::milliseconds(2000));

//     write.SendMessage(Message(Message::Closed{}));

//     write.Close();
// }

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <multiplier> <cooks_per_kitchen> <restock_time_ms>"
                  << std::endl;
        return (84);
    }

    double cookingTimeMultiplier;
    int cooksPerKitchen;
    long long ingredientRestockTimeMs;

    try
    {
        size_t pos;
        cookingTimeMultiplier = std::stod(argv[1], &pos);
        if (pos != std::string(argv[1]).length())
        {
            throw Plazza::InvalidArgument("Invalid multiplier format");
        }

        cooksPerKitchen = std::stoi(argv[2], &pos);
        if (pos != std::string(argv[2]).length())
        {
            throw Plazza::InvalidArgument("Invalid cooks_per_kitchen format");
        }

        ingredientRestockTimeMs = std::stoll(argv[3], &pos);
        if (pos != std::string(argv[3]).length())
        {
            throw Plazza::InvalidArgument("Invalid restock_time_ms format");
        }

        if (cookingTimeMultiplier <= 0)
        {
            std::cerr << "Error: Cooking time multiplier must be positive."
                      << std::endl;
            return (84);
        }
        if (cooksPerKitchen <= 0)
        {
            std::cerr << "Error: Number of cooks per kitchen must be positive."
                      << std::endl;
            return (84);
        }
        if (ingredientRestockTimeMs <= 0)
        {
            std::cerr << "Error: Ingredient restock time must be positive."
                      << std::endl;
            return (84);
        }
    }
    catch (const Plazza::InvalidArgument& e)
    {
        std::cerr << e.what() << std::endl;
        return (84);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected Exception: " << e.what() << std::endl;
        return (84);
    }

    Plazza::APizza::SetCookingTimeMultiplier(cookingTimeMultiplier);

    Plazza::Reception reception(std::chrono::milliseconds(ingredientRestockTimeMs), cooksPerKitchen);
    Plazza::CLI cli(reception);

    Plazza::Kitchen k1, k2, k3;

    cli.Run();

    return (0);
}
