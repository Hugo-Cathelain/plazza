///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/Parser.hpp"
#include "Pizza/PizzaFactory.hpp"
#include <sstream>
#include <stdexcept>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
const std::regex Parser::PIZZA_ORDER_EXP{
    R"(^\s*([a-zA-Z]+)\s+(S|M|L|XL|XXL)\s+(x[1-9][0-9]*)\s*$)"
};

///////////////////////////////////////////////////////////////////////////////
std::string Parser::Trim(const std::string& str)
{
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);

    if (start == std::string::npos)
    {
        return ("");
    }
    return (str.substr(start, end - start + 1));
}

///////////////////////////////////////////////////////////////////////////////
Parser::Orders Parser::ParseOrders(const std::string& line)
{
    Orders orders;
    std::string trimmedLine = Trim(line);

    if (trimmedLine.empty())
    {
        return (orders);
    }

    std::stringstream ss(trimmedLine);
    std::string segment;

    while (std::getline(ss, segment, ';'))
    {
        std::string trimmedSegment = Trim(segment);

        if (trimmedSegment.empty())
        {
            throw std::runtime_error("Invalid order format: empty segment found in order list.");
        };

        std::smatch match;
        if (std::regex_match(trimmedSegment, match, PIZZA_ORDER_EXP))
        {
            if (match.size() == 4)
            {
                std::string typeStr = match[1].str();
                std::string sizeStr = match[2].str();
                std::string numStr = match[3].str();

                int quantity;
                try
                {
                    quantity = std::stoi(numStr.substr(1));
                    if (quantity <= 0)
                    {
                        throw std::runtime_error("Invalid quantity: must be a positive integer.");
                    }
                }
                catch (const std::out_of_range& oor)
                {
                    throw std::runtime_error("Invalid quantity: number too large. " + numStr.substr(1));
                }
                catch (const std::invalid_argument& ia)
                {
                    throw std::runtime_error("Invalid quantity format: " + numStr.substr(1));
                }

                PizzaFactory factory;

                if (!factory.HasFactory(typeStr))
                {
                    throw std::runtime_error("Invalid pizza type: unknown type '" + typeStr + "'");
                }

                orders.push_back(std::make_tuple(
                    factory.CreatePizza(typeStr, IPizza::Size::XL),
                    static_cast<unsigned int>(quantity))
                );
            }
            else
            {
                throw std::runtime_error("Internal parser error: regex match failed unexpectedly for segment: " + trimmedSegment);
            }
        }
        else
        {
            throw std::runtime_error("Invalid order format for segment: '" + trimmedSegment + "'. Expected format: TYPE SIZE xNUMBER");
        }
    }

    if (orders.empty() && !trimmedLine.empty())
    {
        throw std::runtime_error("Invalid order format: No valid orders found in non-empty input.");
    }

    return (orders);
}

} // !namespace Plazza
