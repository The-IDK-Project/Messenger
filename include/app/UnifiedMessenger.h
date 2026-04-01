#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <atomic>
#include <thread>
#include "../core/Message.h"
#include "../core/User.h"
#include "../core/ChatRoom.h"
#include "../core/Config.h"
#include "../protocols/ProtocolHandler.h"
#include "../database/DatabaseManager.h"
#include "../platform/ScreenCapturer.h"

class UnifiedMessenger {
public:
    using MessageCallback = std::function<void(const Message&)>;
    using RoomCallback = std::function<void(const ChatRoom&)>;
    using UserCallback = std::function<void(const User&)>;
    using StatusCallback = std::function<void(const std::string& protocol, bool connected)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    using CallIncomingCallback = std::function<void(const std::string& protocol, const std::string& call_id, const std::string& caller_id, bool is_video)>;

    UnifiedMessenger();
    ~UnifiedMessenger();

    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }

    bool add_protocol(const std::string& name, std::unique_ptr<ProtocolHandler> handler);
    bool remove_protocol(const std::string& name);
    bool connect_protocol(const std::string& name);
    bool disconnect_protocol(const std::string& name);
    void connect_all();
    void disconnect_all();

    std::vector<std::string> get_available_protocols() const;
    std::vector<std::string> get_connected_protocols() const;
    bool is_protocol_connected(const std::string& name) const;

    bool send_message(const std::string& protocol,
                     const std::string& room_id,
                     const std::string& message);
    bool send_file(const std::string& protocol,
                  const std::string& room_id,
                  const std::string& file_path);
    bool send_voice_message(const std::string& protocol,
                           const std::string& room_id,
                           const std::string& file_path);
    bool send_video_message(const std::string& protocol,
                           const std::string& room_id,
                           const std::string& file_path);
    bool mark_message_read(const std::string& protocol,
                          const std::string& room_id,
                          const std::string& message_id);

    // Call API
    bool start_call(const std::string& protocol, const std::string& user_id, bool is_video);
    bool accept_call(const std::string& protocol, const std::string& call_id);
    bool end_call(const std::string& protocol, const std::string& call_id);

    // Screen Share API
    bool start_screen_share(const std::string& protocol, const std::string& call_id, int screen_id);
    bool stop_screen_share(const std::string& protocol, const std::string& call_id);
    std::vector<ScreenCapturer::Screen> get_available_screens();

    bool join_room(const std::string& protocol, const std::string& room_id);
    bool leave_room(const std::string& protocol, const std::string& room_id);
    bool create_room(const std::string& protocol,
                    const std::string& name,
                    const std::vector<std::string>& users);

    std::vector<ChatRoom> get_all_rooms() const;
    std::vector<ChatRoom> get_rooms_by_protocol(const std::string& protocol) const;
    void set_active_room(const std::string& room_id);
    std::string get_active_room() const { return active_room_id_; }

    User get_current_user(const std::string& protocol) const;
    std::vector<User> get_room_users(const std::string& room_id) const;
    std::vector<User> search_users(const std::string& query) const;

    std::vector<Message> get_unified_inbox(int limit = 50) const;
    std::vector<Message> get_room_messages(const std::string& room_id,
                                         int limit = 50) const;
    std::vector<Message> search_messages(const std::string& query,
                                       int limit = 100) const;

    void sync_all();
    void request_sync(const std::string& protocol);

    void set_message_callback(MessageCallback callback);
    void set_room_callback(RoomCallback callback);
    void set_user_callback(UserCallback callback);
    void set_status_callback(StatusCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_call_incoming_callback(CallIncomingCallback callback);

    bool load_config(const std::string& config_path = "");
    bool save_config(const std::string& config_path = "") const;
    void set_setting(const std::string& key, const std::string& value);
    std::string get_setting(const std::string& key,
                          const std::string& default_value = "") const;

    void enable_notifications(bool enable);
    void set_notification_sound(bool enable);
    void set_notification_popup(bool enable);

    bool backup_database(const std::string& backup_path);
    bool vacuum_database();
    int get_database_size() const;

    std::string get_version() const;
    std::vector<std::string> get_supported_protocols() const;
    bool is_ready() const;

private:
    void initialize_protocols();
    void initialize_database();
    void setup_protocol_callbacks(const std::string& protocol_name);
    void handle_protocol_message(const std::string& protocol, const Message& message);
    void handle_protocol_room(const std::string& protocol, const ChatRoom& room);
    void handle_protocol_user(const std::string& protocol, const User& user);
    void handle_protocol_error(const std::string& protocol, const std::string& error);
    void handle_protocol_call_incoming(const std::string& protocol, const std::string& call_id, const std::string& caller_id, bool is_video);

    void sync_worker();
    void notification_worker();

    std::map<std::string, std::unique_ptr<ProtocolHandler>> protocols_;
    // DatabaseManager and Config are singletons, access via get_instance()

    std::string active_room_id_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};

    MessageCallback message_callback_;
    RoomCallback room_callback_;
    UserCallback user_callback_;
    StatusCallback status_callback_;
    ErrorCallback error_callback_;
    CallIncomingCallback call_incoming_callback_;

    std::thread sync_thread_;
    std::thread notification_thread_;
    mutable std::mutex data_mutex_;

    UnifiedMessenger(const UnifiedMessenger&) = delete;
    UnifiedMessenger& operator=(const UnifiedMessenger&) = delete;
};