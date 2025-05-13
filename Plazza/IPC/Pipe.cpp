///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "IPC/Pipe.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <system_error>
#include <cstring>
#include <iostream>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
Pipe::Pipe(const std::string& name, Pipe::OpenMode mode)
    : m_name(name)
    , m_mode(mode)
    , m_fd(-1)
    , m_errorState(false)
    , m_isCreator(false)
{}

///////////////////////////////////////////////////////////////////////////////
Pipe::Pipe(Pipe&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_mode(other.m_mode)
    , m_fd(other.m_fd)
    , m_errorState(other.m_errorState)
    , m_isCreator(other.m_isCreator)
{
    other.m_fd = -1;
    other.m_isCreator = false;
    other.m_errorState = true;
}

///////////////////////////////////////////////////////////////////////////////
Pipe::~Pipe()
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
Pipe& Pipe::operator=(Pipe&& other) noexcept
{
    if (this != &other)
    {
        if (IsOpen())
        {
            Close();
        }

        m_name = std::move(other.m_name);
        m_mode = other.m_mode;
        m_fd = other.m_fd;
        m_errorState = other.m_errorState;
        m_isCreator = other.m_isCreator;

        other.m_fd = -1;
        other.m_isCreator = false;
        other.m_errorState = true;
    }
    return (*this);
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::TryCreateFifo(void)
{
    struct stat st;

    if (stat(m_name.c_str(), &st) == -1)
    {
        if (errno == ENOENT)
        {
            if (mkfifo(m_name.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1)
            {
                if (errno != EEXIST)
                {
                    throw std::runtime_error("Failed to create FIFO '" + m_name + "': " + strerror(errno));
                }
            }
            else
            {
                m_isCreator = true;
            }
        }
        else
        {
            throw std::runtime_error("Failed to stat FIFO '" + m_name + "': " + strerror(errno)); 
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::Open(void)
{
    if (IsOpen())
    {
        return;
    }
    m_errorState = false;
    m_isCreator = false;

    TryCreateFifo();

    int flags = 0;
    if (m_mode == OpenMode::READ_ONLY)
    {
        flags = O_RDONLY;
    }
    else if (m_mode == OpenMode::WRITE_ONLY)
    {
        flags = O_WRONLY;
    }
    else
    {
        m_errorState = true;
        throw std::runtime_error("Unsupported pipe mode for opening.");
    }

    m_fd = ::open(m_name.c_str(), flags);

    if (m_fd == -1)
    {
        m_errorState = true;
        throw std::runtime_error(
            "Failed to open FIFO '" + m_name + "' for " +
            (m_mode == OpenMode::READ_ONLY ? "reading" : "writing") +
            ": " + strerror(errno)
        );
    }
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::Close(void)
{
    if (IsOpen())
    {
        if (::close(m_fd) == -1)
        {
            std::cerr << "Warning: error closing pipe '"
                      << m_name << "' fd " << m_fd << ": "
                      << strerror(errno) << std::endl;
        }
        m_fd = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Pipe::IsOpen(void) const
{
    return (m_fd != -1);
}

///////////////////////////////////////////////////////////////////////////////
bool Pipe::IsGood(void) const
{
    return (IsOpen() && !m_errorState);
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::RemoveResource(void)
{
    if (!m_name.empty())
    {
        if (::unlink(m_name.c_str()) == -1)
        {
            if (errno != ENOENT)
            {
                std::cerr << "Warning: error unlinking pipe '"
                          << m_name << "': " << strerror(errno) << std::endl;
            }
        }
        m_isCreator = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
bool Pipe::ReadBytes(char* buffer, size_t bytes)
{
    if (!IsGood() || m_mode != OpenMode::READ_ONLY)
    {
        m_errorState = true;
        return (false);
    }

    size_t totalBytesRead = 0;
    while (totalBytesRead < bytes)
    {
        ssize_t bytesReadNow = ::read(
            m_fd, buffer + totalBytesRead, bytes - totalBytesRead
        );

        if (bytesReadNow < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            m_errorState = true;
            return (false);
        }
        if (bytesReadNow == 0)
        {
            m_errorState = true;
            return (false);
        }
        totalBytesRead += bytesReadNow;
    }
    return (true);
}

///////////////////////////////////////////////////////////////////////////////
bool Pipe::WriteBytes(const char* buffer, size_t bytes)
{
    if (!IsGood() || m_mode != OpenMode::WRITE_ONLY)
    {
        m_errorState = true;
        return (false);
    }

    size_t totalBytesWritten = 0;
    while (totalBytesWritten < bytes)
    {
        ssize_t bytesWrittenNow = ::write(
            m_fd, buffer + totalBytesWritten, bytes - totalBytesWritten
        );

        if (bytesWrittenNow < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EPIPE)
            {
                m_errorState = true;
                return (false);
            }
            m_errorState = true;
            return (false);
        }
        totalBytesWritten += bytesWrittenNow;
    }
    return (true);
}

///////////////////////////////////////////////////////////////////////////////
IIPCChannel& Pipe::operator<<(const Message& msg)
{
    if (!IsGood() || m_mode != OpenMode::WRITE_ONLY)
    {
        m_errorState = true;
        throw std::runtime_error(
            "Pipe not open for writing or in error state: " + m_name
        );
    }

    std::vector<char> packedMessage = msg.Pack();
    if (
        packedMessage.empty() &&
        msg.payload.empty() &&
        msg.type == Message::Type::UNDEFINED
    )
    {
        return (*this);
    }

    if (!WriteBytes(packedMessage.data(), packedMessage.size()))
    {
        throw std::runtime_error(
            "Failed to write entire message to pipe '" + m_name +
            "'. Last error: " + strerror(errno)
        );
    }
    return (*this);
}

///////////////////////////////////////////////////////////////////////////////
IIPCChannel& Pipe::operator>>(Message& msg)
{
    if (!IsGood() || m_mode != OpenMode::READ_ONLY)
    {
        m_errorState = true;
        throw std::runtime_error(
            "Pipe not open for reading or in error state: " + m_name
        );
    }

    char headerBuffer[Message::MESSAGE_HEADER_ONLY_SIZE];
    if (!ReadBytes(headerBuffer, Message::MESSAGE_HEADER_ONLY_SIZE))
    {
        if (!IsOpen() && !m_errorState)
        {
            m_errorState = true;
        }
        throw std::runtime_error(
            "Failed to read message header from pipe '" + m_name +
            "'. EOF or read error. Last error: " + strerror(errno)
        );
    }

    msg.type = static_cast<Message::Type>(headerBuffer[0]);
    uint32_t payloadSize = 0;
    std::memcpy(
        &payloadSize,
        headerBuffer + Message::MESSAGE_TYPE_SIZE,
        Message::PAYLOAD_SIZE_INDICATOR_SIZE
    );

    msg.payload.clear();
    if (payloadSize > 0)
    {
        msg.payload.resize(payloadSize);
        if (!ReadBytes(msg.payload.data(), payloadSize))
        {
            throw std::runtime_error(
                "Failed to read message payload (size " +
                std::to_string(payloadSize) +
                ") from pipe '" + m_name + "'. Last error: " + strerror(errno)
            );
        }
    }

    return *this;
}

} // !namespace Plazza
