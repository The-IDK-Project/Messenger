#pragma once

#include <string>
#include <vector>
#include <chrono>

enum class RoomType {
    DIRECT = 0,
    GROUP = 1,
    CHANNEL = 2
};

enum class RoomStatus {
    ACTIVE = 0,
    ARCHIVED = 1,
    INVITE_ONLY = 2
};

class ChatRoom {
public:
    std::string id;
    std::string name;
    std::string protocol;
    std::vector<std::string> participants;
    RoomType type;
    RoomStatus status;
    bool is_encrypted;
    std::string last_message_id;
    std::chrono::system_clock::time_point last_activity;
    std::chrono::system_clock::time_point created_at;

    ChatRoom();
    ChatRoom(const std::string& id,
             const std::string& name,
             const std::string& protocol,
             RoomType type = RoomType::GROUP);

    std::string to_json() const;
    static ChatRoom from_json(const std::string& json_str);

    bool is_direct_message() const { return type == RoomType::DIRECT; }
    bool is_group_chat() const { return type == RoomType::GROUP; }
    bool is_channel() const { return type == RoomType::CHANNEL; }

    void add_participant(const std::string& user_id);
    void remove_participant(const std::string& user_id);
    bool has_participant(const std::string& user_id) const;
    size_t participant_count() const { return participants.size(); }

    std::string get_display_name() const;
    void update_activity();

    void set_encryption(bool encrypted) { is_encrypted = encrypted; }
    bool get_encryption() const { return is_encrypted; }

private:
    void generate_display_name();
};