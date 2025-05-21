///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/CLI.hpp"
#include "Kitchen/Kitchen.hpp"
#include "Pizza/APizza.hpp"
#include "Errors/InvalidArgument.hpp"
#include <iostream>

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

    float cookingTimeMultiplier;
    int cooksPerKitchen;
    long long ingredientRestockTimeMs;

    try
    {
        size_t pos;
        cookingTimeMultiplier = std::stof(argv[1], &pos);
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

    Plazza::Reception reception;
    Plazza::CLI cli(reception);

    Plazza::Kitchen kitchen1;

    cli.Run();

    return (0);
}
