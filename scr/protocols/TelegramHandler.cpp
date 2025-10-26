#include "protocols/TelegramHandler.h"
#include "utils/Logger.h"

TelegramHandler::TelegramHandler()
    : state_(ProtocolState::DISCONNECTED)
    , auth_state_(0)
    , is_authorized_(false) {
}

TelegramHandler::TelegramHandler(const std::string& api_id, const std::string& api_hash)
    : api_id_(api_id)
    , api_hash_(api_hash)
    , state_(ProtocolState::DISCONNECTED)
    , auth_state_(0)
    , is_authorized_(false) {
}

TelegramHandler::~TelegramHandler() {
    disconnect();
}

bool TelegramHandler::connect() {
    if (state_ != ProtocolState::DISCONNECTED) {
        return false;
    }

    state_ = ProtocolState::CONNECTING;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    state_ = ProtocolState::CONNECTED;
    LOG_INFO("TelegramHandler connected (simulated)");
    return true;
}

void TelegramHandler::disconnect() {
    state_ = ProtocolState::DISCONNECTED;
    LOG_INFO("TelegramHandler disconnected");
}

bool TelegramHandler::send_message(const std::string& room_id, const std::string& message) {
    if (!is_connected()) {
        return false;
    }

    LOG_INFO("Telegram message sent to " + room_id + ": " + message);
    return true;
}

std::vector<ChatRoom> TelegramHandler::get_rooms() {
    std::vector<ChatRoom> rooms;
    if (is_connected()) {
        ChatRoom room1;
        room1.id = "tg_chat_1";
        room1.name = "Telegram Chat";
        room1.protocol = "telegram";
        room1.type = RoomType::DIRECT;
        rooms.push_back(room1);

        ChatRoom room2;
        room2.id = "tg_group_1";
        room2.name = "Telegram Group";
        room2.protocol = "telegram";
        room2.type = RoomType::GROUP;
        rooms.push_back(room2);
    }

    return rooms;
}

User TelegramHandler::get_current_user() {
    User user;
    user.id = "tg_user_1";
    user.username = "telegram_user";
    user.display_name = "Telegram User";
    user.protocols = {"telegram"};
    return user;
}

uint32_t TelegramHandler::get_capabilities() const {
    return static_cast<uint32_t>(ProtocolCapabilities::MESSAGES) |
           static_cast<uint32_t>(ProtocolCapabilities::FILES) |
           static_cast<uint32_t>(ProtocolCapabilities::ENCRYPTION) |
           static_cast<uint32_t>(ProtocolCapabilities::VOICE) |
           static_cast<uint32_t>(ProtocolCapabilities::VIDEO);
}