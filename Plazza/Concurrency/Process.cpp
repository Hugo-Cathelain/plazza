///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Concurrency/Process.hpp"
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Process::Process(std::function<void()> function)
    : m_function(function)
    , m_pid(-1)
    , m_status(IProcess::Status::STOPPED)
    , m_returnValue(0)
{}

///////////////////////////////////////////////////////////////////////////////
Process::~Process()
{
    if (m_pid > 0)
    {
        if (this->IsRunning())
        {
            if (kill(m_pid, SIGTERM) == 0)
            {
                int status;
                pid_t wait_ret = waitpid(m_pid, &status, 0);

                if (wait_ret == m_pid)
                {
                    UpdateStatusFromWait(status);
                }
                else
                {
                    kill(m_pid, SIGKILL);
                    wait_ret = waitpid(m_pid, &status, 0);
                    if (wait_ret == m_pid)
                    {
                        UpdateStatusFromWait(status);
                    }
                    else
                    {
                        m_status = IProcess::Status::ERROR;
                    }
                }
            } else {
                if (errno == ESRCH)
                {
                    int status;
                    if (waitpid(m_pid, &status, WNOHANG) == m_pid)
                    {
                        UpdateStatusFromWait(status);
                    } else
                    {
                        m_status = IProcess::Status::FINISHED;
                    }
                }
                else
                {
                    m_status = IProcess::Status::ERROR;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void Process::Start(void)
{
    if (m_status == IProcess::Status::RUNNING)
    {
        throw std::runtime_error("Process already started.");
    }

    m_pid = fork();

    if (m_pid < 0)
    {
        m_status = IProcess::Status::ERROR;
        throw std::runtime_error("Fork failed: " + std::string(strerror(errno)));
    }
    else if (m_pid == 0)
    {
        try
        {
            if (m_function)
            {
                m_function();
            }
            _exit(0);
        }
        catch (const std::exception& e)
        {
            _exit(1);
        }
        catch (...)
        {
            _exit(2);
        }
    }
    else
    {
        m_status = IProcess::Status::RUNNING;
        m_returnValue = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
void Process::UpdateStatusFromWait(int status)
{
    if (WIFEXITED(status))
    {
        m_returnValue = WEXITSTATUS(status);
        m_status = IProcess::Status::FINISHED;
    }
    else if (WIFSIGNALED(status))
    {
        m_returnValue = WTERMSIG(status);
        m_status = IProcess::Status::ERROR;
    }
    else
    {
        m_status = IProcess::Status::ERROR;
    }
}

///////////////////////////////////////////////////////////////////////////////
void Process::Wait(void)
{
    if (m_pid == -1)
    {
        throw std::runtime_error(
            "Process not started or already waited upon without restart."
        );
    }

    if (m_status != IProcess::Status::RUNNING)
    {
        return;
    }

    int status;
    pid_t result = waitpid(m_pid, &status, 0);

    if (result == -1)
    {
        m_status = IProcess::Status::ERROR;
        throw std::runtime_error(
            "Waitpid failed: " + std::string(strerror(errno))
        );
    }
    else if (result == m_pid)
    {
        UpdateStatusFromWait(status);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Process::IsRunning(void)
{
    if (
        m_pid == -1 ||
        m_status == IProcess::Status::FINISHED ||
        m_status == IProcess::Status::ERROR
    )
    {
        return (false);
    }
    if (m_status == IProcess::Status::STOPPED && m_pid == -1)
    {
        return (false);
    }

    int status;
    pid_t result = waitpid(m_pid, &status, WNOHANG);

    if (result == 0)
    {
        m_status = IProcess::Status::RUNNING;
        return (true);
    }
    else if (result == m_pid)
    {
        UpdateStatusFromWait(status);
        return (false);
    }
    else if (result == -1)
    {
        m_status = IProcess::Status::ERROR;
    }
    return (false);
}

///////////////////////////////////////////////////////////////////////////////
int Process::GetReturnValue(void)
{
    return (m_returnValue);
}

///////////////////////////////////////////////////////////////////////////////
IProcess::Status Process::GetStatus(void) const
{
    return (m_status);
}

///////////////////////////////////////////////////////////////////////////////
pid_t Process::GetPid(void) const
{
    return (m_pid);
}

///////////////////////////////////////////////////////////////////////////////
bool Process::IsParent(void) const
{
    return (m_pid > 0);
}

///////////////////////////////////////////////////////////////////////////////
bool Process::IsChild(void) const
{
    return (m_pid == 0);
}

} // !namespace Plazza
