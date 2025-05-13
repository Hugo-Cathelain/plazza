///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Concurrency/Process.hpp"
#include <iostream>
#include <unistd.h>

void kitchen(void)
{
    std::cout << "Kitchen (PID: " << getpid() << ") started and doing work..." << std::endl;
    sleep(3); // Simulate work
    std::cout << "Kitchen (PID: " << getpid() << ") finishing." << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    try
    {
        Plazza::Process cp(kitchen);

        std::cout << "Reception: Starting kitchen process..." << std::endl;
        cp.Start();
        std::cout << "Reception: Kitchen process started with PID: " << cp.GetPid() << std::endl;

        while (cp.IsRunning())
        {
            std::cout << "Reception: Kitchen is running..." << std::endl;
            sleep(1);
        }

        std::cout << "Reception: Kitchen process no longer running. Waiting to collect status..." << std::endl;
        cp.Wait();

        if (cp.GetStatus() == Plazza::IProcess::Status::FINISHED)
        {
            std::cout << "Reception: Kitchen finished with exit code: " << cp.GetReturnValue() << std::endl;
        } else {
            std::cout << "Reception: Kitchen ended with status: " << static_cast<int>(cp.GetStatus())
                      << " and code: " << cp.GetReturnValue() << std::endl;
        }
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << error.what() << std::endl;
        return (1);
    }

    return (0);
}
