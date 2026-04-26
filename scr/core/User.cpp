#include "core/User.h"
#include "utils/StringUtils.h"
#include "utils/JsonParser.h"
#include <algorithm>

User::User()
    : last_seen(std::chrono::system_clock::now())
    , created_at(std::chrono::system_clock::now())
    , online_(false) {
}

User::User(const std::string& id,
           const std::string& username,
           const std::string& protocol)
    : id(id)
    , username(username)
    , last_seen(std::chrono::system_clock::now())
    , created_at(std::chrono::system_clock::now())
    , online_(false) {
    protocols.push_back(protocol);
}

std::string User::to_json() const {
    JsonValue json = JsonValue::object();
    json["id"] = id;
    json["username"] = username;
    json["display_name"] = display_name;

    JsonValue protocols_array = JsonValue::array();
    for (const auto& protocol : protocols) {
        protocols_array.push_back(protocol);
    }
    json["protocols"] = protocols_array;

    if (!avatar_url.empty()) {
        json["avatar_url"] = avatar_url;
    }

    json["last_seen"] = JsonValue(static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        last_seen.time_since_epoch()).count()));
    json["created_at"] = JsonValue(static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        created_at.time_since_epoch()).count()));
    json["online"] = online_;

    return json.to_string();
}

User User::from_json(const std::string& json_str) {
    try {
        JsonValue json = JsonValue::parse(json_str);
        User user;

        user.id = json["id"].as_string();
        user.username = json["username"].as_string();
        user.display_name = json["display_name"].as_string();

        if (json.has_key("protocols") && json["protocols"].is_array()) {
            for (size_t i = 0; i < json["protocols"].size(); ++i) {
                user.protocols.push_back(json["protocols"][i].as_string());
            }
        }

        if (json.has_key("avatar_url")) {
            user.avatar_url = json["avatar_url"].as_string();
        }

        if (json.has_key("last_seen")) {
            int64_t last_seen_ms = json["last_seen"].as_int64();
            user.last_seen = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(last_seen_ms));
        }

        if (json.has_key("created_at")) {
            int64_t created_at_ms = json["created_at"].as_int64();
            user.created_at = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(created_at_ms));
        }

        if (json.has_key("online")) {
            user.online_ = json["online"].as_bool();
        }

        return user;
    } catch (const std::exception& e) {
        return User();
    }
}

std::string User::get_best_name() const {
    if (!display_name.empty()) {
        return display_name;
    }
    return username;
}

bool User::is_online() const {
    return online_;
}

bool User::supports_protocol(const std::string& protocol) const {
    return std::find(protocols.begin(), protocols.end(), protocol) != protocols.end();
}

void User::add_protocol(const std::string& protocol) {
    if (!supports_protocol(protocol)) {
        protocols.push_back(protocol);
    }
}

void User::remove_protocol(const std::string& protocol) {
    auto it = std::find(protocols.begin(), protocols.end(), protocol);
    if (it != protocols.end()) {
        protocols.erase(it);
    }
}

void User::update_presence(bool online) {
    online_ = online;
    last_seen = std::chrono::system_clock::now();
}

std::string User::get_presence_status() const {
    if (online_) {
        return "online";
    }

    auto now = std::chrono::system_clock::now();
    auto diff = now - last_seen;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count();

    if (minutes < 5) {
        return "just now";
    } else if (minutes < 60) {
        return std::to_string(minutes) + " minutes ago";
    } else if (minutes < 1440) { // 24 hours
        auto hours = minutes / 60;
        return std::to_string(hours) + " hours ago";
    } else {
        auto days = minutes / 1440;
        return std::to_string(days) + " days ago";
    }
}
