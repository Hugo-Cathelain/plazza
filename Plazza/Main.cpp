///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Concurrency/Process.hpp"
#include "IPC/Pipe.hpp"      // Added for IPC
#include "IPC/Message.hpp"   // Added for IPC
#include <iostream>
#include <unistd.h>
#include <string>           // Added for std::string
#include <vector>           // Added for std::vector

// Define a pipe name (must be accessible by both processes)
const std::string RECEPTION_TO_KITCHEN_PIPE = "/tmp/plazza_reception_to_kitchen_pipe";

void kitchen_process_main() // Renamed to avoid conflict if kitchen becomes a class
{
    std::cout << "Kitchen (PID: " << getpid() << ") started." << std::endl;

    try {
        Plazza::Pipe kitchenPipe(RECEPTION_TO_KITCHEN_PIPE, Plazza::Pipe::OpenMode::READ_ONLY);
        std::cout << "Kitchen: Attempting to open pipe " << RECEPTION_TO_KITCHEN_PIPE << " for reading..." << std::endl;
        kitchenPipe.Open();
        std::cout << "Kitchen: Pipe opened for reading." << std::endl;

        Plazza::Message receivedMsg;
        std::cout << "Kitchen: Waiting for message from Reception..." << std::endl;
        kitchenPipe >> receivedMsg; // Receive message

        std::cout << "Kitchen: Received message!" << std::endl;
        std::cout << "Kitchen: Message Type: " << static_cast<int>(receivedMsg.type) << std::endl;
        std::cout << "Kitchen: Message Payload: \"" << receivedMsg.GetPayloadAsString() << "\"" << std::endl;

        kitchenPipe.Close();
        std::cout << "Kitchen: Pipe closed." << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "Kitchen (PID: " << getpid() << ") IPC Error: " << e.what() << std::endl;
        _exit(3); // Exit with a different code on IPC error
    } catch (...) {
        std::cerr << "Kitchen (PID: " << getpid() << ") Unknown IPC Error." << std::endl;
        _exit(4);
    }

    std::cout << "Kitchen (PID: " << getpid() << ") processing done and finishing." << std::endl;
    // _exit(0) will be called by Process class wrapper if this function returns normally
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Cleanup pipe from previous runs if it exists (optional, for convenience during testing)
    // In a real scenario, the creator is responsible for removal at the appropriate time.
    // Plazza::Pipe tempCleaner(RECEPTION_TO_KITCHEN_PIPE, Plazza::Pipe::OpenMode::WRITE_ONLY); // Mode doesn't matter for unlink
    // tempCleaner.RemoveResource();


    try
    {
        Plazza::Process kitchenCp(kitchen_process_main);

        std::cout << "Reception: Starting kitchen process..." << std::endl;
        kitchenCp.Start();
        std::cout << "Reception: Kitchen process started with PID: " << kitchenCp.GetPid() << std::endl;

        // Give the kitchen a moment to start and open its end of the pipe
        // sleep(1);

        try {
            Plazza::Pipe receptionPipe(RECEPTION_TO_KITCHEN_PIPE, Plazza::Pipe::OpenMode::WRITE_ONLY);
            std::cout << "Reception: Attempting to open pipe " << RECEPTION_TO_KITCHEN_PIPE << " for writing..." << std::endl;
            receptionPipe.Open(); // This will also create the FIFO if it doesn't exist
            std::cout << "Reception: Pipe opened for writing." << std::endl;

            Plazza::Message orderMsg(Plazza::Message::Type::ORDER_PIZZA);
            std::string orderDetails = "Regina XXL x1; Fantasia S x2";
            orderMsg.SetPayloadFromString(orderDetails);

            std::cout << "Reception: Sending message to Kitchen..." << std::endl;
            receptionPipe << orderMsg; // Send message
            std::cout << "Reception: Message sent." << std::endl;

            receptionPipe.Close();
            std::cout << "Reception: Pipe closed." << std::endl;
            // Reception, as the creator in this simple example, removes the resource.
            receptionPipe.RemoveResource();
            std::cout << "Reception: Pipe resource " << RECEPTION_TO_KITCHEN_PIPE << " removed." << std::endl;

        } catch (const std::runtime_error& ipc_error) {
            std::cerr << "Reception IPC Error: " << ipc_error.what() << std::endl;
            // Decide if we should try to kill the child or just wait if IPC failed
        }


        std::cout << "Reception: Waiting for kitchen process to complete its tasks..." << std::endl;
        // The original loop to check IsRunning can be kept or removed
        // depending on whether other work is done while kitchen runs.
        // For this example, we'll just wait.
        // while (kitchenCp.IsRunning())
        // {
        //     // std::cout << "Reception: Kitchen is running..." << std::endl;
        //     sleep(1);
        // }

        kitchenCp.Wait(); // Wait for the child process to finish

        std::cout << "Reception: Kitchen process finished." << std::endl;
        if (kitchenCp.GetStatus() == Plazza::IProcess::Status::FINISHED)
        {
            std::cout << "Reception: Kitchen finished with exit code: " << kitchenCp.GetReturnValue() << std::endl;
        } else {
            std::cout << "Reception: Kitchen ended with status: " << static_cast<int>(kitchenCp.GetStatus())
                      << " and code: " << kitchenCp.GetReturnValue() << std::endl;
        }
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << "Main Error: " << error.what() << std::endl;
        // If pipe creation failed in parent, child might be stuck.
        // Consider cleanup or sending SIGTERM to child if _pid is known.
        return (1);
    }
    catch (...)
    {
        std::cerr << "Unknown error in main." << std::endl;
        return (2);
    }

    return (0);
}
