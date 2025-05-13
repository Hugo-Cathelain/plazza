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
const size_t Message::MESSAGE_TYPE_SIZE = sizeof(Message::Type);
const size_t Message::PAYLOAD_SIZE_INDICATOR_SIZE = sizeof(uint32_t);
const size_t Message::MESSAGE_HEADER_ONLY_SIZE =
    MESSAGE_TYPE_SIZE + PAYLOAD_SIZE_INDICATOR_SIZE;

///////////////////////////////////////////////////////////////////////////////
Message::Message(Message::Type type, std::vector<char> data)
    : type(type)
    , payload(std::move(data))
{}

///////////////////////////////////////////////////////////////////////////////
bool Message::Unpack(
    const char* buffer,
    size_t size,
    Message& out,
    size_t& bytes
)
{
    if (size < MESSAGE_HEADER_ONLY_SIZE)
    {
        bytes = 0;
        return (false);
    }

    size_t offset = 0;

    out.type = static_cast<Message::Type>(buffer[offset]);
    offset += MESSAGE_TYPE_SIZE;

    uint32_t netPayloadSize = 0;
    std::memcpy(&netPayloadSize, buffer + offset, PAYLOAD_SIZE_INDICATOR_SIZE);
    offset += PAYLOAD_SIZE_INDICATOR_SIZE;

    if (size < MESSAGE_HEADER_ONLY_SIZE + netPayloadSize)
    {
        bytes = 0;
        return (false);
    }

    out.payload.assign(buffer + offset, buffer + offset + netPayloadSize);
    offset += netPayloadSize;

    bytes = offset;
    return (true);
}

///////////////////////////////////////////////////////////////////////////////
std::vector<char> Message::Pack(void) const
{
    std::vector<char> buffer;

    buffer.reserve(MESSAGE_HEADER_ONLY_SIZE + payload.size());
    buffer.push_back(static_cast<char>(type));

    uint32_t netPayloadSize = static_cast<uint32_t>(payload.size());
    char sizeBytes[PAYLOAD_SIZE_INDICATOR_SIZE];

    std::memcpy(sizeBytes, &netPayloadSize, PAYLOAD_SIZE_INDICATOR_SIZE);
    for (size_t i = 0; i < PAYLOAD_SIZE_INDICATOR_SIZE; ++i)
    {
        buffer.push_back(sizeBytes[i]);
    }

    buffer.insert(buffer.end(), payload.begin(), payload.end());

    return (buffer);
}

///////////////////////////////////////////////////////////////////////////////
std::string Message::GetPayloadAsString(void) const
{
    return (std::string(payload.begin(), payload.end()));
}

///////////////////////////////////////////////////////////////////////////////
void Message::SetPayloadFromString(const std::string& str)
{
    payload.assign(str.begin(), str.end());
}

} // !namespace Plazza
