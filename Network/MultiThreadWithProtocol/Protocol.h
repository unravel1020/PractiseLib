#pragma once

#include <string>
#include <cstdint>

namespace net {
namespace protocol {

enum class MessageType {
    DATA,
    EXIT,
    ERROR_MSG,   // ✅ 改名避免冲突
    PING,
    PONG
};

struct MessageHeader {
    uint8_t version;
    uint8_t type;
    uint32_t length;
};

class Protocol {
public:
    static const uint8_t CURRENT_VERSION = 1;

    static std::string serialize(MessageType type, const std::string& data);
    static bool deserialize(const char* buffer, size_t buffer_len, MessageType& out_type, std::string& out_data);
};

} // namespace protocol
} // namespace net