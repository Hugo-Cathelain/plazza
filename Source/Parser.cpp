///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Parser.hpp"
#include <algorithm>
#include <regex>
#include <map>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
inline void Parser::ltrim(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(),
    [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

///////////////////////////////////////////////////////////////////////////////
inline void Parser::rtrim(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}

///////////////////////////////////////////////////////////////////////////////
inline void Parser::trim(std::string& str)
{
    ltrim(str);
    rtrim(str);
}

///////////////////////////////////////////////////////////////////////////////
std::vector<Order> Parser::parseOrder(const std::string& order)
{
    std::vector<Order> orders;

    std::string command = order;
    trim(command);

    if (command.empty())
    {
        return orders;
    }

    std::stringstream orderStream(order);
    std::string currentSegment;
    int segmentIndex = 0;

    const std::regex orderRegex(R"(^\s*([a-zA-Z]+)\s+(S|M|L|XL|XXL)\s+(x[1-9][0-9]*)\s*$)");

    while (std::getline(orderStream, currentSegment, ';'))
    {
        segmentIndex++;
        trim(currentSegment);

        if (currentSegment.empty())
        {
            continue;
        }

        std::smatch matches;

        if (std::regex_match(currentSegment, matches, orderRegex))
        {
            if (matches.size() == 4)
            {
                Order order;

                static const std::map<std::string, PizzaSize> PIZZA_SIZES = {};
                static const std::map<std::string, PizzaType> PIZZA_TYPES = {};

            }
        }
    }

    return orders;
}

} // namespace Plazza
