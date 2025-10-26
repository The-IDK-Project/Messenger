#include "protocols/ProtocolHandler.h"
#include "utils/Logger.h"

void ProtocolHandler::set_message_callback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void ProtocolHandler::set_room_callback(RoomCallback callback) {
    room_callback_ = std::move(callback);
}

void ProtocolHandler::set_user_callback(UserCallback callback) {
    user_callback_ = std::move(callback);
}

void ProtocolHandler::set_error_callback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
}

bool ProtocolHandler::has_capability(ProtocolCapabilities capability) const {
    return (get_capabilities() & static_cast<uint32_t>(capability)) != 0;
}