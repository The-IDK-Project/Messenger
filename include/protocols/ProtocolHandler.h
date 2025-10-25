#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../core/Message.h"
#include "../core/User.h"
#include "../core/ChatRoom.h"

using MessageCallback = std::function<void(const Message&)>;
using RoomCallback = std::function<void(const ChatRoom&)>;
using UserCallback = std::function<void(const User&)>;
using ErrorCallback = std::function<void(const std::string&)>;

enum class ProtocolState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

enum class ProtocolCapabilities {
    MESSAGES      = 1 << 0,
    FILES         = 1 << 1,
    ENCRYPTION    = 1 << 2,
    TYPING        = 1 << 3,
    READ_RECEIPTS = 1 << 4,
    VOICE         = 1 << 5,
    VIDEO         = 1 << 6
};

class ProtocolHandler {
public:
    virtual ~ProtocolHandler() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual ProtocolState get_state() const = 0;
    virtual bool is_connected() const = 0;

    virtual bool send_message(const std::string& room_id,
                            const std::string& message) = 0;
    virtual bool send_file(const std::string& room_id,
                          const std::string& file_path,
                          const std::string& filename = "") = 0;
    virtual bool send_typing(const std::string& room_id, bool typing) = 0;
    virtual bool mark_read(const std::string& room_id,
                          const std::string& message_id) = 0;

    virtual bool join_room(const std::string& room_id) = 0;
    virtual bool leave_room(const std::string& room_id) = 0;
    virtual bool create_room(const std::string& name,
                           const std::vector<std::string>& users) = 0;
    virtual std::vector<ChatRoom> get_rooms() = 0;

    virtual User get_current_user() = 0;
    virtual std::vector<User> get_room_users(const std::string& room_id) = 0;
    virtual User get_user(const std::string& user_id) = 0;

    virtual void sync() = 0;
    virtual bool supports_sync() const = 0;

    virtual void set_message_callback(MessageCallback callback) = 0;
    virtual void set_room_callback(RoomCallback callback) = 0;
    virtual void set_user_callback(UserCallback callback) = 0;
    virtual void set_error_callback(ErrorCallback callback) = 0;

    virtual std::string get_protocol_name() const = 0;
    virtual std::string get_protocol_version() const = 0;
    virtual uint32_t get_capabilities() const = 0;
    virtual bool has_capability(ProtocolCapabilities capability) const = 0;

    virtual bool set_config(const std::string& key, const std::string& value) = 0;
    virtual std::string get_config(const std::string& key) const = 0;

protected:
    MessageCallback message_callback_;
    RoomCallback room_callback_;
    UserCallback user_callback_;
    ErrorCallback error_callback_;
};