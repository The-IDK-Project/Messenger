#include "core/ChatRoom.h"
#include "utils/StringUtils.h"
#include "utils/JsonParser.h"
#include <algorithm>

ChatRoom::ChatRoom()
    : type(RoomType::GROUP)
    , status(RoomStatus::ACTIVE)
    , is_encrypted(false)
    , last_activity(std::chrono::system_clock::now())
    , created_at(std::chrono::system_clock::now()) {
}

ChatRoom::ChatRoom(const std::string& id,
                   const std::string& name,
                   const std::string& protocol,
                   RoomType type)
    : id(id)
    , name(name)
    , protocol(protocol)
    , type(type)
    , status(RoomStatus::ACTIVE)
    , is_encrypted(false)
    , last_activity(std::chrono::system_clock::now())
    , created_at(std::chrono::system_clock::now()) {
    generate_display_name();
}

std::string ChatRoom::to_json() const {
    JsonValue json = JsonValue::object();
    json["id"] = id;
    json["name"] = name;
    json["protocol"] = protocol;

    JsonValue participants_array = JsonValue::array();
    for (const auto& participant : participants) {
        participants_array.push_back(participant);
    }
    json["participants"] = participants_array;

    json["type"] = static_cast<int>(type);
    json["status"] = static_cast<int>(status);
    json["is_encrypted"] = is_encrypted;

    if (!last_message_id.empty()) {
        json["last_message_id"] = last_message_id;
    }

    json["last_activity"] = JsonValue(static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        last_activity.time_since_epoch()).count()));
    json["created_at"] = JsonValue(static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        created_at.time_since_epoch()).count()));

    return json.to_string();
}

ChatRoom ChatRoom::from_json(const std::string& json_str) {
    try {
        JsonValue json = JsonValue::parse(json_str);
        ChatRoom room;

        room.id = json["id"].as_string();
        room.name = json["name"].as_string();
        room.protocol = json["protocol"].as_string();

        if (json.has_key("participants") && json["participants"].is_array()) {
            for (size_t i = 0; i < json["participants"].size(); ++i) {
                room.participants.push_back(json["participants"][i].as_string());
            }
        }

        room.type = static_cast<RoomType>(json["type"].as_int());
        room.status = static_cast<RoomStatus>(json["status"].as_int());
        room.is_encrypted = json["is_encrypted"].as_bool();

        if (json.has_key("last_message_id")) {
            room.last_message_id = json["last_message_id"].as_string();
        }

        if (json.has_key("last_activity")) {
            int64_t last_activity_ms = json["last_activity"].as_int64();
            room.last_activity = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(last_activity_ms));
        }

        if (json.has_key("created_at")) {
            int64_t created_at_ms = json["created_at"].as_int64();
            room.created_at = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(created_at_ms));
        }

        room.generate_display_name();
        return room;
    } catch (const std::exception& e) {
        return ChatRoom();
    }
}

void ChatRoom::add_participant(const std::string& user_id) {
    if (!has_participant(user_id)) {
        participants.push_back(user_id);
        update_activity();
    }
}

void ChatRoom::remove_participant(const std::string& user_id) {
    auto it = std::find(participants.begin(), participants.end(), user_id);
    if (it != participants.end()) {
        participants.erase(it);
        update_activity();
    }
}

bool ChatRoom::has_participant(const std::string& user_id) const {
    return std::find(participants.begin(), participants.end(), user_id) != participants.end();
}

std::string ChatRoom::get_display_name() const {
    if (!name.empty()) {
        return name;
    }

    if (type == RoomType::DIRECT && !participants.empty()) {
        if (participants.size() == 1) {
            return participants[0];
        }
    }

    return "Unnamed Room";
}

void ChatRoom::update_activity() {
    last_activity = std::chrono::system_clock::now();
}

void ChatRoom::generate_display_name() {
    if (name.empty()) {
        if (type == RoomType::DIRECT) {
            name = "Direct Message";
        } else if (type == RoomType::GROUP) {
            name = "Group Chat";
        } else {
            name = "Channel";
        }
    }
}
