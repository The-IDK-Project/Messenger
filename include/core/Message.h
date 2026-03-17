#pragma once

#include <string>
#include <chrono>
#include <vector>

enum class MessageType {
    TEXT = 0,
    IMAGE = 1,
    FILE = 2,
    SYSTEM = 3,
    VIDEO_MESSAGE = 4, // Кружки
    VOICE_CALL = 5,    // Звонки
    VIDEO_CALL = 6
};

enum class MessageStatus {
    SENDING = 0,
    SENT = 1,
    DELIVERED = 2,
    READ = 3,
    ERROR = 4
};

class Message {
public:
    std::string id;
    std::string content;
    std::string sender_id;
    std::string sender_name;
    std::string room_id;
    std::string protocol;
    MessageType type;
    MessageStatus status;
    std::chrono::system_clock::time_point timestamp;
    std::string reply_to_id;
    std::string metadata;

    Message();
    Message(const std::string& content,
            const std::string& sender_id,
            const std::string& sender_name,
            const std::string& protocol,
            const std::string& room_id,
            MessageType type = MessageType::TEXT);

    std::string to_json() const;
    static Message from_json(const std::string& json_str);

    bool is_from_me(const std::string& my_user_id) const;
    std::string get_display_time() const;
    bool is_system_message() const { return type == MessageType::SYSTEM; }

    std::string get_file_url() const;
    std::string get_image_dimensions() const;

    // New helper methods
    bool is_call() const { return type == MessageType::VOICE_CALL || type == MessageType::VIDEO_CALL; }
    bool is_video_message() const { return type == MessageType::VIDEO_MESSAGE; }

private:
    void generate_id();
};