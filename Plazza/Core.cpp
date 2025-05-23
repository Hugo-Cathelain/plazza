///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Core.hpp"
#include "Errors/InvalidArgument.hpp"
#include "Pizza/APizza.hpp"
#include <iostream>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Core::Core(int argc, char* argv[])
    : m_initialized(false)
{
    ParseArguments(argc, argv);

    Plazza::APizza::SetCookingTimeMultiplier(m_cookingTimeMultiplier);

    m_reception = std::make_unique<Reception>(
        std::chrono::milliseconds(m_restockTimeMs),
        m_cooksPerKitchen
    );

    m_cli = std::make_unique<CLI>(*m_reception);
}

///////////////////////////////////////////////////////////////////////////////
void Core::PrintUsage(const std::string& arg0)
{
    std::cerr << "Usage: "
              << arg0
              << " <multiplier>"
              << " <cooks_per_kitchen>"
              << " <restock_time_ms>"
              << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
void Core::ParseArguments(int argc, char* argv[])
{
    if (argc != 4)
    {
        return (PrintUsage(argv[0]));
    }

    size_t pos;
    m_cookingTimeMultiplier = std::stod(argv[1], &pos);
    if (pos != std::string(argv[1]).length())
    {
        throw InvalidArgument("Invalid multiplier format");
    }

    m_cooksPerKitchen = std::stoi(argv[2], &pos);
    if (pos != std::string(argv[2]).length())
    {
        throw InvalidArgument("Invalid cooks_per_kitchen format");
    }

    m_restockTimeMs = std::stoll(argv[3], &pos);
    if (pos != std::string(argv[3]).length())
    {
        throw InvalidArgument("Invalid restock_time_ms format");
    }

    if (m_cookingTimeMultiplier <= 0.0)
    {
        throw InvalidArgument("Cooking time multiplier must be positive.");
    }

    if (m_cooksPerKitchen <= 0)
    {
        throw InvalidArgument("Number of cooks per kitchen must be positive.");
    }

    if (m_restockTimeMs <= 0)
    {
        throw InvalidArgument("Ingredient restock time must be positive.");
    }

    m_initialized = true;
}

///////////////////////////////////////////////////////////////////////////////
bool Core::IsInitialized(void) const
{
    return (m_initialized);
}

///////////////////////////////////////////////////////////////////////////////
void Core::OpenPizzeria(void)
{
    if (m_initialized)
    {
        m_cli->Run();
    }
}

} // !namespace Plazza
