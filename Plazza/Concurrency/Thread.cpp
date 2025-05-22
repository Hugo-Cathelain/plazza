///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Concurrency/Thread.hpp"
#include <utility>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Thread::Thread(std::function<void()> function)
    : m_function(std::move(function))
    , m_started(false)
    , m_running(false)
{}

///////////////////////////////////////////////////////////////////////////////
Thread::~Thread()
{
    if (m_thread.joinable())
    {
        try
        {
            m_thread.join();
        }
        catch (...)
        {}
    }
}

///////////////////////////////////////////////////////////////////////////////
void Thread::Start(void)
{
    if (m_started)
    {
        throw std::runtime_error("Thread already started.");
    }
    if (!m_function)
    {
        throw std::runtime_error("Cannot start thread with no function.");
    }
    try
    {
        m_running = true;
        m_thread = std::thread(m_function);
        m_started = true;
    }
    catch (const std::exception& e)
    {
        m_started = false;
        throw std::runtime_error("Failed to start thread: " + std::string(e.what()));
    }
}

///////////////////////////////////////////////////////////////////////////////
void Thread::Join(void)
{
    if (!m_started)
    {
        return;
    }

    if (m_thread.joinable())
    {
        try
        {
            m_thread.join();
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Failed to join thread: " + std::string(e.what())
            );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void Thread::Detach(void)
{
    if (!m_started)
    {
        return;
    }

    if (m_thread.joinable())
    {
        try
        {
            m_thread.detach();
        } catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Failed to detach thread: " + std::string(e.what())
            );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Thread::Joinable(void) const
{
    if (!m_started)
    {
        return (false);
    }
    return (m_thread.joinable());
}

} // !namespace Plazza
