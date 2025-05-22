///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "IPC/Message.hpp"

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
void Message::AppendToBuffer(std::vector<char>& buffer, const std::string& str)
{
    uint32_t len = static_cast<uint32_t>(str.length());
    AppendToBuffer(buffer, len);
    buffer.insert(buffer.end(), str.begin(), str.end());
}

///////////////////////////////////////////////////////////////////////////////
bool Message::ReadFromBuffer(
    const char*& current,
    const char* end,
    std::string& str
)
{
    uint32_t len;
    if (!ReadFromBuffer(current, end, len))
    {
        return (false);
    }
    if (current + static_cast<ptrdiff_t>(len) > end)
    {
        return (false);
    }
    str.assign(current, len);
    current += len;
    return (true);
}

///////////////////////////////////////////////////////////////////////////////
std::optional<Message> Message::Unpack(const std::vector<char>& buffer)
{
    const char* current = buffer.data();
    const char* const buffer_end = buffer.data() + buffer.size();

    uint32_t payload_len;
    if (!ReadFromBuffer(current, buffer_end, payload_len))
    {
        return (std::nullopt);
    }

    if (static_cast<long int>(payload_len) != (buffer_end - current))
    {
        return (std::nullopt);
    }

    const char* payload_start = current;
    const char* const payload_actual_end = payload_start + payload_len;

    uint8_t type_idx_val;
    if (!ReadFromBuffer(current, payload_actual_end, type_idx_val))
    {
        return (std::nullopt);
    }
    size_t type_idx = static_cast<size_t>(type_idx_val);

    std::optional<Message> result = std::nullopt;

    if (type_idx == 0)
    {
        result = Message(Message::Closed{});
    }
    else if (type_idx == 1)
    {
        result = Message(Message::Order{});
    }
    else if (type_idx == 2)
    {
        Message::Status data;
        if (
            ReadFromBuffer(current, payload_actual_end, data.id) &&
            ReadFromBuffer(current, payload_actual_end, data.stock)
        )
        {
            result = Message(data);
        }
    }
    else if (type_idx == 3)
    {
        result = Message(Message::RequestStatus{});
    }
    else
    {
        return (std::nullopt);
    }

    if (!result || current != payload_actual_end)
    {
        return (std::nullopt);
    }

    return (result);
}

///////////////////////////////////////////////////////////////////////////////
std::vector<char> Message::Pack(void) const
{
    std::vector<char> payload_buffer;

    uint8_t type_idx = static_cast<uint8_t>(m_data.index());
    AppendToBuffer(payload_buffer, type_idx);

    std::visit([&payload_buffer](const auto& data)
    {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, Message::Closed>)
        {
        } else if constexpr (std::is_same_v<T, Message::Order>)
        {
        }
        else if constexpr (std::is_same_v<T, Message::Status>)
        {
            AppendToBuffer(payload_buffer, data.id);
            AppendToBuffer(payload_buffer, data.stock);
        } else if constexpr (std::is_same_v<T, Message::RequestStatus>)
        {
        }
    }, m_data);

    std::vector<char> final_buffer;
    uint32_t len = static_cast<uint32_t>(payload_buffer.size());
    AppendToBuffer(final_buffer, len);
    final_buffer.insert(
        final_buffer.end(),
        payload_buffer.begin(),
        payload_buffer.end()
    );

    return (final_buffer);
}

} // !namespace Plazza
