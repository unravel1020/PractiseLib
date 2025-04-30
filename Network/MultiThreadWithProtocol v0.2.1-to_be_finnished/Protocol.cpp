#include "Protocol.h"
#include <cstring>
#include <winsock2.h>
#include <iostream>
#include <cstdint>
#include <zlib.h>

namespace net {
namespace protocol {

uint32_t calculate_crc32(const void* data, size_t len) {
    return crc32(0, reinterpret_cast<const Bytef*>(data), static_cast<int>(len));
}

std::string Protocol::serialize(MessageType type, const std::string& data) {
    MessageHeader header;
    header.version = CURRENT_VERSION;
    header.type = static_cast<uint8_t>(type);
    header.length = htonl(static_cast<uint32_t>(data.size()));
    header.crc32 = htonl(calculate_crc32(data.data(), data.size()));

    std::string packet;
    packet.reserve(sizeof(header) + data.size());
    packet.append(reinterpret_cast<const char*>(&header), sizeof(header));
    packet.append(data);

    return packet;
}

bool Protocol::deserialize(const char* buffer, size_t buffer_len, MessageType& out_type, std::string& out_data) {
    if (buffer_len < sizeof(MessageHeader)) {
        std::cerr << "[Protocol] Buffer too small for header.\n";
        return false;
    }

    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(buffer);

    if (header->version != CURRENT_VERSION) {
        std::cerr << "[Protocol] Unsupported protocol version: " << (int)header->version << "\n";
        out_type = MessageType::ERROR_MSG;
        out_data = "Unsupported protocol version";
        return true;
    }

    uint32_t payload_len = ntohl(header->length);
    if (buffer_len < sizeof(MessageHeader) + payload_len) {
        std::cerr << "[Protocol] Incomplete message received.\n";
        return false;
    }

    // 校验 CRC32
    uint32_t expected_crc = ntohl(header->crc32);
    uint32_t actual_crc = calculate_crc32(buffer + sizeof(MessageHeader), payload_len);
    if (expected_crc != actual_crc) {
        std::cerr << "[Protocol] CRC check failed: expected " << expected_crc << ", got " << actual_crc << "\n";
        out_type = MessageType::ERROR_MSG;
        out_data = "CRC check failed";
        return true;
    }

    out_type = static_cast<MessageType>(header->type);
    out_data.assign(buffer + sizeof(MessageHeader), payload_len);
    return true;
}

} // namespace protocol
} // namespace net