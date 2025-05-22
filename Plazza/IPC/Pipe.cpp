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
{}

///////////////////////////////////////////////////////////////////////////////
Pipe::Pipe(Pipe&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_mode(other.m_mode)
    , m_fd(other.m_fd)
    , m_buffer(std::move(other.m_buffer))
{
    other.m_fd = -1;
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
        Close();

        m_name = std::move(other.m_name);
        m_mode = other.m_mode;
        m_fd = other.m_fd;
        m_buffer = std::move(other.m_buffer);

        other.m_fd = -1;
    }
    return (*this);
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::Open(void)
{
    if (m_fd != -1)
    {
        return;
    }

    if (mkfifo(m_name.c_str(), 0666) == -1)
    {
        if (errno != EEXIST)
        {
            throw std::system_error(
                errno,
                std::system_category(),
                "Failed to create FIFO: " + m_name
            );
        }
    }

    int flags = 0;
    if (m_mode == OpenMode::READ_ONLY)
    {
        flags = O_RDONLY | O_NONBLOCK;
    }
    else
    {
        flags = O_WRONLY;
    }

    m_fd = open(m_name.c_str(), flags);
    if (m_fd == -1)
    {
        throw std::system_error(
            errno,
            std::system_category(),
            "Failed to open FIFO: " + m_name
        );
    }
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::Close(void)
{
    if (m_fd != -1)
    {
        close(m_fd);
        m_fd = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
void Pipe::SendMessage(const Message& message)
{
    if (m_mode != OpenMode::WRITE_ONLY)
    {
        throw std::runtime_error(
            "Pipe not opened in WRITE_ONLY mode for SendMessage."
        );
    }
    if (m_fd == -1)
    {
        throw std::runtime_error("Pipe is not open for SendMessage.");
    }

    std::vector<char> packed_message = message.Pack();
    if (packed_message.empty())
    {
        return;
    }

    ssize_t total_written = 0;
    size_t total_to_write = packed_message.size();
    const char* data_ptr = packed_message.data();

    while (total_written < static_cast<ssize_t>(total_to_write))
    {
        ssize_t bytes_written = write(
            m_fd, data_ptr + total_written, total_to_write - total_written
        );
        if (bytes_written == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            throw std::system_error(
                errno,
                std::system_category(),
                "Failed to write to pipe"
            );
        }
        total_written += bytes_written;
    }
}

///////////////////////////////////////////////////////////////////////////////
std::optional<Message> Pipe::PollMessage(void)
{
    if (m_mode != OpenMode::READ_ONLY || m_fd == -1)
    {
        return (std::nullopt);
    }

    char read_buf[4096];
    ssize_t bytes_read = read(m_fd, read_buf, sizeof(read_buf));

    if (bytes_read > 0)
    {
        m_buffer.insert(m_buffer.end(), read_buf, read_buf + bytes_read);
    } else if (bytes_read == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // No data available right now, this is expected for O_NONBLOCK.
            // Try to parse any existing data in buffer.
        }
        else if (errno == EINTR)
        {
            // Interrupted, also try to parse existing buffer.
        }
         else
         {
            throw std::system_error(
                errno, std::system_category(), "Pipe read error"
            );
        }
    }
    else if (bytes_read == 0)
    {
        // EOF - other end closed.
        // If m_buffer is empty, future calls will also yield EOF and return nullopt.
        // If m_buffer has data, we try to parse it below.
        // Consider this a signal that no more data will EVER come.
        // If m_buffer is empty after this, it's truly the end.
    }

    if (m_buffer.size() < sizeof(uint32_t))
    {
        return (std::nullopt);
    }

    uint32_t declared_payload_len;
    std::memcpy(&declared_payload_len, m_buffer.data(), sizeof(uint32_t));

    uint32_t required_total_len = sizeof(uint32_t) + declared_payload_len;

    if (m_buffer.size() < required_total_len)
    {
        if (bytes_read == 0 && m_buffer.size() < required_total_len)
        {
            m_buffer.clear();
        }
        return (std::nullopt);
    }

    std::vector<char> current_message_bytes(
        m_buffer.begin(), m_buffer.begin() + required_total_len
    );
    std::optional<Message> unpacked_msg = Message::Unpack(current_message_bytes);

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + required_total_len);

    if (!unpacked_msg)
    {
        // Unpack failed, meaning the data was malformed.
        // Data has been consumed from buffer. Log or handle as error.
        // std::cerr << "Warning: Message::Unpack failed. Discarded malformed message segment." << std::endl;
    }
    
    return (unpacked_msg);
}

} // !namespace Plazza
