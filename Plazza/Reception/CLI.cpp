///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/CLI.hpp"
#include "Reception/Parser.hpp"
#include "Errors/ParsingException.hpp"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
CLI::CLI(Reception& reception)
    : m_reception(reception)
{}

///////////////////////////////////////////////////////////////////////////////
void CLI::DisplayPrompt(void) const
{
    std::cout << "> ";
    std::cout.flush();
}

///////////////////////////////////////////////////////////////////////////////
void CLI::Run(void)
{
    std::string line;
    std::cout << "Welcome to Plazza!" << std::endl;
    std::cout << "Enter pizza orders "
              << "(e.g., 'regina XXL x1; margarita S x2') "
              << "or 'status' or 'exit'."
              << std::endl;

    while (true)
    {
        DisplayPrompt();

        if (!std::getline(std::cin, line))
        {
            std::cout << "Exiting Plazza." << std::endl;
            break;
        }

        std::string command = Parser::Trim(line);

        if (command.empty())
        {
            continue;
        }

        if (command == "exit")
        {
            std::cout << "Exiting Plazza." << std::endl;
            break;
        }

        ProcessCommand(command);
    }
}

///////////////////////////////////////////////////////////////////////////////
void CLI::ProcessCommand(const std::string& line)
{
    if (line == "status")
    {
        m_reception.DisplayStatus();
        // TODO: Add reception DisplayStatus function
    }
    else
    {
        try
        {
            Parser::Orders orders = Parser::ParseOrders(line);

            if (!orders.empty())
            {
                m_reception.ProcessOrders(orders);
            }
        }
        catch (const ParsingException& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

} // !namespace Plazza
