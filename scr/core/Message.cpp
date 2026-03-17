#include "core/Message.h"
#include "utils/StringUtils.h"
#include "utils/JsonParser.h"
#include <chrono>
#include <random>
#include <sstream>

Message::Message()
    : type(MessageType::TEXT)
    , status(MessageStatus::SENDING)
    , timestamp(std::chrono::system_clock::now()) {
    generate_id();
}

Message::Message(const std::string& content,
                 const std::string& sender_id,
                 const std::string& sender_name,
                 const std::string& protocol,
                 const std::string& room_id,
                 MessageType type)
    : content(content)
    , sender_id(sender_id)
    , sender_name(sender_name)
    , room_id(room_id)
    , protocol(protocol)
    , type(type)
    , status(MessageStatus::SENT)
    , timestamp(std::chrono::system_clock::now()) {
    generate_id();
}

std::string Message::to_json() const {
    JsonValue json = JsonValue::object();
    json["id"] = id;
    json["content"] = content;
    json["sender_id"] = sender_id;
    json["sender_name"] = sender_name;
    json["room_id"] = room_id;
    json["protocol"] = protocol;
    json["type"] = static_cast<int>(type);
    json["status"] = static_cast<int>(status);
    json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();

    if (!reply_to_id.empty()) {
        json["reply_to_id"] = reply_to_id;
    }

    if (!metadata.empty()) {
        json["metadata"] = metadata;
    }

    return json.to_string();
}

Message Message::from_json(const std::string& json_str) {
    try {
        JsonValue json = JsonValue::parse(json_str);
        Message msg;

        msg.id = json["id"].as_string();
        msg.content = json["content"].as_string();
        msg.sender_id = json["sender_id"].as_string();
        msg.sender_name = json["sender_name"].as_string();
        msg.room_id = json["room_id"].as_string();
        msg.protocol = json["protocol"].as_string();
        msg.type = static_cast<MessageType>(json["type"].as_int());
        msg.status = static_cast<MessageStatus>(json["status"].as_int());

        int64_t timestamp_ms = json["timestamp"].as_int();
        msg.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestamp_ms));

        if (json.has_key("reply_to_id")) {
            msg.reply_to_id = json["reply_to_id"].as_string();
        }

        if (json.has_key("metadata")) {
            msg.metadata = json["metadata"].as_string();
        }

        return msg;
    } catch (const std::exception& e) {
        return Message();
    }
}

bool Message::is_from_me(const std::string& my_user_id) const {
    return sender_id == my_user_id;
}

std::string Message::get_display_time() const {
    auto now = std::chrono::system_clock::now();
    auto diff = now - timestamp;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(diff);

    if (hours.count() < 24) {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm = *std::localtime(&time_t);

        char buffer[9];
        std::strftime(buffer, sizeof(buffer), "%H:%M", &tm);
        return buffer;
    } else {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm = *std::localtime(&time_t);

        char buffer[11];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
        return buffer;
    }
}

std::string Message::get_file_url() const {
    if (type != MessageType::FILE && type != MessageType::IMAGE && type != MessageType::VIDEO_MESSAGE) {
        return "";
    }

    try {
        JsonValue meta = JsonValue::parse(metadata);
        if (meta.has_key("file_url")) {
            return meta["file_url"].as_string();
        }
    } catch (...) {
    }

    return "";
}

std::string Message::get_image_dimensions() const {
    if (type != MessageType::IMAGE && type != MessageType::VIDEO_MESSAGE) {
        return "";
    }

    try {
        JsonValue meta = JsonValue::parse(metadata);
        if (meta.has_key("width") && meta.has_key("height")) {
            return std::to_string(meta["width"].as_int()) + "x" +
                   std::to_string(meta["height"].as_int());
        }
    } catch (...) {
    }

    return "";
}

void Message::generate_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    const char* hex_chars = "0123456789abcdef";
    std::stringstream ss;
    ss << std::hex << timestamp;

    for (int i = 0; i < 8; ++i) {
        ss << hex_chars[dis(gen)];
    }

    id = ss.str();
}