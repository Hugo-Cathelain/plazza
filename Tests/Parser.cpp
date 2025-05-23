///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Reception/Parser.hpp"
#include "Errors/ParsingException.hpp"
#include "Errors/InvalidArgument.hpp"
#include <criterion/criterion.h>

///////////////////////////////////////////////////////////////////////////////
using namespace Plazza;

///////////////////////////////////////////////////////////////////////////////
Test(Parser, trim_whitespace)
{
    cr_assert_str_eq(Parser::Trim("  hello  ").c_str(), "hello", "Should trim leading and trailing spaces");
    cr_assert_str_eq(Parser::Trim("\t\nhello\r\f").c_str(), "hello", "Should trim all whitespace characters");
    cr_assert_str_eq(Parser::Trim("hello").c_str(), "hello", "Should not modify string without whitespace");
    cr_assert_str_eq(Parser::Trim("").c_str(), "", "Should handle empty string");
    cr_assert_str_eq(Parser::Trim("   ").c_str(), "", "Should return empty for whitespace-only string");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_single_valid_order)
{
    auto orders = Parser::ParseOrders("regina S x1");

    cr_assert_eq(orders.size(), 1, "Should parse one pizza");
    cr_assert_not_null(orders[0].get(), "Pizza should be created");
    cr_assert_eq(orders[0]->GetSize(), IPizza::Size::S, "Pizza should have size S");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_multiple_quantities)
{
    auto orders = Parser::ParseOrders("margarita M x3");

    cr_assert_eq(orders.size(), 3, "Should parse three pizzas");
    for (const auto& pizza : orders) {
        cr_assert_not_null(pizza.get(), "All pizzas should be created");
        cr_assert_eq(pizza->GetSize(), IPizza::Size::M, "All pizzas should have size M");
    }
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_multiple_orders_semicolon_separated)
{
    auto orders = Parser::ParseOrders("regina S x1; fantasia L x2; americana XL x1");

    cr_assert_eq(orders.size(), 4, "Should parse four pizzas total");
    cr_assert_eq(orders[0]->GetSize(), IPizza::Size::S, "First pizza should be size S");
    cr_assert_eq(orders[1]->GetSize(), IPizza::Size::L, "Second pizza should be size L");
    cr_assert_eq(orders[2]->GetSize(), IPizza::Size::L, "Third pizza should be size L");
    cr_assert_eq(orders[3]->GetSize(), IPizza::Size::XL, "Fourth pizza should be size XL");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_all_pizza_sizes)
{
    auto orders = Parser::ParseOrders("regina S x1; fantasia M x1; americana L x1; margarita XL x1; regina XXL x1");

    cr_assert_eq(orders.size(), 5, "Should parse five pizzas");
    cr_assert_eq(orders[0]->GetSize(), IPizza::Size::S, "First pizza should be size S");
    cr_assert_eq(orders[1]->GetSize(), IPizza::Size::M, "Second pizza should be size M");
    cr_assert_eq(orders[2]->GetSize(), IPizza::Size::L, "Third pizza should be size L");
    cr_assert_eq(orders[3]->GetSize(), IPizza::Size::XL, "Fourth pizza should be size XL");
    cr_assert_eq(orders[4]->GetSize(), IPizza::Size::XXL, "Fifth pizza should be size XXL");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_with_extra_whitespace)
{
    auto orders = Parser::ParseOrders("  regina   S   x1  ;  fantasia   M   x2  ");

    cr_assert_eq(orders.size(), 3, "Should parse three pizzas despite extra whitespace");
    cr_assert_eq(orders[0]->GetSize(), IPizza::Size::S, "First pizza should be size S");
    cr_assert_eq(orders[1]->GetSize(), IPizza::Size::M, "Second pizza should be size M");
    cr_assert_eq(orders[2]->GetSize(), IPizza::Size::M, "Third pizza should be size M");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_empty_string)
{
    auto orders = Parser::ParseOrders("");

    cr_assert_eq(orders.size(), 0, "Empty string should return no orders");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_whitespace_only)
{
    auto orders = Parser::ParseOrders("   \t\n  ");

    cr_assert_eq(orders.size(), 0, "Whitespace-only string should return no orders");
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_invalid_pizza_type)
{
    cr_assert_throw(
        Parser::ParseOrders("invalid_pizza S x1"),
        ParsingException,
        "Should throw ParsingException for invalid pizza type"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_invalid_size)
{
    cr_assert_throw(
        Parser::ParseOrders("regina Z x1"),
        ParsingException,
        "Should throw ParsingException for invalid pizza size"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_invalid_quantity_format)
{
    cr_assert_throw(
        Parser::ParseOrders("regina S x0"),
        ParsingException,
        "Should throw ParsingException for zero quantity"
    );

    cr_assert_throw(
        Parser::ParseOrders("regina S x-1"),
        ParsingException,
        "Should throw ParsingException for negative quantity"
    );

    cr_assert_throw(
        Parser::ParseOrders("regina S xabc"),
        ParsingException,
        "Should throw ParsingException for non-numeric quantity"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_missing_quantity_prefix)
{
    cr_assert_throw(
        Parser::ParseOrders("regina S 1"),
        ParsingException,
        "Should throw ParsingException for missing 'x' prefix"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_invalid_format)
{
    cr_assert_throw(
        Parser::ParseOrders("regina"),
        ParsingException,
        "Should throw ParsingException for incomplete order"
    );

    cr_assert_throw(
        Parser::ParseOrders("regina S"),
        ParsingException,
        "Should throw ParsingException for missing quantity"
    );

    cr_assert_throw(
        Parser::ParseOrders("S x1"),
        ParsingException,
        "Should throw ParsingException for missing pizza type"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_empty_segment)
{
    cr_assert_throw(
        Parser::ParseOrders("regina S x1;;fantasia M x1"),
        ParsingException,
        "Should throw ParsingException for empty segment between semicolons"
    );

    cr_assert_throw(
        Parser::ParseOrders("regina S x1; ;fantasia M x1"),
        ParsingException,
        "Should throw ParsingException for whitespace-only segment"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_large_quantity)
{
    auto orders = Parser::ParseOrders("regina S x100");

    cr_assert_eq(orders.size(), 100, "Should handle large quantities");
    for (const auto& pizza : orders) {
        cr_assert_not_null(pizza.get(), "All pizzas should be created");
        cr_assert_eq(pizza->GetSize(), IPizza::Size::S, "All pizzas should have correct size");
    }
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_very_large_quantity_overflow)
{
    cr_assert_throw(
        Parser::ParseOrders("regina S x999999999999999999999"),
        ParsingException,
        "Should throw ParsingException for quantity overflow"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_mixed_valid_invalid_orders)
{
    cr_assert_throw(
        Parser::ParseOrders("regina S x1; invalid_pizza M x1; fantasia L x2"),
        ParsingException,
        "Should throw ParsingException when any segment is invalid"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_case_sensitive_pizza_types)
{
    cr_assert_throw(
        Parser::ParseOrders("REGINA S x1"),
        ParsingException,
        "Should throw ParsingException for uppercase pizza type"
    );

    cr_assert_throw(
        Parser::ParseOrders("Regina S x1"),
        ParsingException,
        "Should throw ParsingException for capitalized pizza type"
    );
}

///////////////////////////////////////////////////////////////////////////////
Test(Parser, parse_case_sensitive_sizes)
{
    cr_assert_throw(
        Parser::ParseOrders("regina s x1"),
        ParsingException,
        "Should throw ParsingException for lowercase size"
    );
}
