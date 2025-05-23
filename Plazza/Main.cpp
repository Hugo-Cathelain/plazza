///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Core.hpp"
#include "Errors/Exception.hpp"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    try
    {
        Plazza::Core core(argc, argv);

        if (core.IsInitialized())
        {
            core.OpenPizzeria();
        }
    }
    catch (const Plazza::Exception& e)
    {
        std::cerr << e.what() << std::endl;
        return (84);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected Exception: " << e.what() << std::endl;
        return (84);
    }

    return (0);
}
