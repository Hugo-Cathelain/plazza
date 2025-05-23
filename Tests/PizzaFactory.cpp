///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include <criterion/criterion.h>
#include "Pizza/PizzaFactory.hpp"

///////////////////////////////////////////////////////////////////////////////
using namespace Plazza;

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, singleton_instance)
{
    PizzaFactory& f1 = PizzaFactory::GetInstance();
    PizzaFactory& f2 = PizzaFactory::GetInstance();

    cr_assert_eq(&f1, &f2, "PizzaFactory should be a singleton");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_valid_pizza_regina)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza = factory.CreatePizza("regina", IPizza::Size::S);

    cr_assert_not_null(pizza.get(), "Should create a valid Regina pizza");
    cr_assert_eq(pizza->GetSize(), IPizza::Size::S, "Pizza should have correct size");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_valid_pizza_fantasia)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza = factory.CreatePizza("fantasia", IPizza::Size::M);

    cr_assert_not_null(pizza.get(), "Should create a valid Fantasia pizza");
    cr_assert_eq(pizza->GetSize(), IPizza::Size::M, "Pizza should have correct size");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_valid_pizza_americana)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza = factory.CreatePizza("americana", IPizza::Size::L);

    cr_assert_not_null(pizza.get(), "Should create a valid Americana pizza");
    cr_assert_eq(pizza->GetSize(), IPizza::Size::L, "Pizza should have correct size");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_valid_pizza_margarita)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza = factory.CreatePizza("margarita", IPizza::Size::XL);

    cr_assert_not_null(pizza.get(), "Should create a valid Margarita pizza");
    cr_assert_eq(pizza->GetSize(), IPizza::Size::XL, "Pizza should have correct size");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_invalid_pizza_type)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza = factory.CreatePizza("invalid_type", IPizza::Size::S);

    cr_assert_null(pizza.get(), "Should return nullptr for invalid pizza type");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_pizza_empty_type)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza = factory.CreatePizza("", IPizza::Size::M);

    cr_assert_null(pizza.get(), "Should return nullptr for empty pizza type");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, has_factory_valid_types)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    cr_assert(factory.HasFactory("regina"), "Should have regina factory");
    cr_assert(factory.HasFactory("fantasia"), "Should have fantasia factory");
    cr_assert(factory.HasFactory("americana"), "Should have americana factory");
    cr_assert(factory.HasFactory("margarita"), "Should have margarita factory");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, has_factory_invalid_type)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    cr_assert_not(factory.HasFactory("invalid_type"), "Should not have invalid factory");
    cr_assert_not(factory.HasFactory(""), "Should not have empty factory");
    cr_assert_not(factory.HasFactory("REGINA"), "Should be case sensitive");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, get_factory_list)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto factories = factory.GetFactoryList();

    cr_assert_eq(factories.size(), 4, "Should have 4 registered factories");

    // Check that all expected types are present
    bool has_regina = false, has_fantasia = false, has_americana = false, has_margarita = false;

    for (const auto& type : factories)
    {
        if (type == "regina") has_regina = true;
        else if (type == "fantasia") has_fantasia = true;
        else if (type == "americana") has_americana = true;
        else if (type == "margarita") has_margarita = true;
    }

    cr_assert(has_regina, "Factory list should contain regina");
    cr_assert(has_fantasia, "Factory list should contain fantasia");
    cr_assert(has_americana, "Factory list should contain americana");
    cr_assert(has_margarita, "Factory list should contain margarita");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_multiple_pizzas_same_type)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizza1 = factory.CreatePizza("margarita", IPizza::Size::S);
    auto pizza2 = factory.CreatePizza("margarita", IPizza::Size::M);

    cr_assert_not_null(pizza1.get(), "First pizza should be created");
    cr_assert_not_null(pizza2.get(), "Second pizza should be created");
    cr_assert_neq(pizza1.get(), pizza2.get(), "Pizzas should be different instances");
    cr_assert_eq(pizza1->GetSize(), IPizza::Size::S, "First pizza should have size S");
    cr_assert_eq(pizza2->GetSize(), IPizza::Size::M, "Second pizza should have size M");
}

///////////////////////////////////////////////////////////////////////////////
Test(PizzaFactory, create_all_sizes)
{
    PizzaFactory& factory = PizzaFactory::GetInstance();

    auto pizzaS = factory.CreatePizza("regina", IPizza::Size::S);
    auto pizzaM = factory.CreatePizza("regina", IPizza::Size::M);
    auto pizzaL = factory.CreatePizza("regina", IPizza::Size::L);
    auto pizzaXL = factory.CreatePizza("regina", IPizza::Size::XL);

    cr_assert_not_null(pizzaS.get(), "Should create size S pizza");
    cr_assert_not_null(pizzaM.get(), "Should create size M pizza");
    cr_assert_not_null(pizzaL.get(), "Should create size L pizza");
    cr_assert_not_null(pizzaXL.get(), "Should create size XL pizza");

    cr_assert_eq(pizzaS->GetSize(), IPizza::Size::S, "Size S pizza should have correct size");
    cr_assert_eq(pizzaM->GetSize(), IPizza::Size::M, "Size M pizza should have correct size");
    cr_assert_eq(pizzaL->GetSize(), IPizza::Size::L, "Size L pizza should have correct size");
    cr_assert_eq(pizzaXL->GetSize(), IPizza::Size::XL, "Size XL pizza should have correct size");
}
