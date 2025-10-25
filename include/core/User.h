#pragma once

#include <string>
#include <vector>
#include <chrono>

class User {
public:
    std::string id;
    std::string username;
    std::string display_name;
    std::vector<std::string> protocols;
    std::string avatar_url;
    std::chrono::system_clock::time_point last_seen;
    std::chrono::system_clock::time_point created_at;

    User();
    User(const std::string& id,
         const std::string& username,
         const std::string& protocol);

    std::string to_json() const;
    static User from_json(const std::string& json_str);

    std::string get_best_name() const;
    bool is_online() const;
    bool supports_protocol(const std::string& protocol) const;

    void add_protocol(const std::string& protocol);
    void remove_protocol(const std::string& protocol);

    void update_presence(bool online);
    std::string get_presence_status() const;

private:
    bool online_;
};