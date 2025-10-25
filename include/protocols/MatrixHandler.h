#pragma once

#include "ProtocolHandler.h"
#include <string>
#include <map>
#include <vector>

class MatrixHandler : public ProtocolHandler {
public:
    MatrixHandler();
    explicit MatrixHandler(const std::string& homeserver);
    ~MatrixHandler() override;

    bool connect() override;
    void disconnect() override;
    ProtocolState get_state() const override;
    bool is_connected() const override;

    bool login_password(const std::string& username,
                       const std::string& password);
    bool login_token(const std::string& access_token);
    bool logout();

    bool send_message(const std::string& room_id,
                     const std::string& message) override;
    bool send_file(const std::string& room_id,
                  const std::string& file_path,
                  const std::string& filename = "") override;
    bool send_typing(const std::string& room_id, bool typing) override;
    bool mark_read(const std::string& room_id,
                  const std::string& message_id) override;

    bool join_room(const std::string& room_id) override;
    bool leave_room(const std::string& room_id) override;
    bool create_room(const std::string& name,
                    const std::vector<std::string>& users) override;
    std::vector<ChatRoom> get_rooms() override;

    User get_current_user() override;
    std::vector<User> get_room_users(const std::string& room_id) override;
    User get_user(const std::string& user_id) override;

    bool upload_file(const std::string& file_path, std::string& mxc_uri);
    bool set_display_name(const std::string& display_name);
    bool set_avatar(const std::string& avatar_path);
    bool invite_user(const std::string& room_id, const std::string& user_id);
    bool kick_user(const std::string& room_id, const std::string& user_id);

    void sync() override;
    bool supports_sync() const override { return true; }

    void set_message_callback(MessageCallback callback) override;
    void set_room_callback(RoomCallback callback) override;
    void set_user_callback(UserCallback callback) override;
    void set_error_callback(ErrorCallback callback) override;

    std::string get_protocol_name() const override { return "matrix"; }
    std::string get_protocol_version() const override { return "v1"; }
    uint32_t get_capabilities() const override;
    bool has_capability(ProtocolCapabilities capability) const override;

    bool set_config(const std::string& key, const std::string& value) override;
    std::string get_config(const std::string& key) const override;

private:
    std::string homeserver_;
    std::string access_token_;
    std::string user_id_;
    std::string device_id_;
    std::string next_batch_token_;
    ProtocolState state_;

    void* curl_handle_;
    void* ws_handle_;

    bool http_request(const std::string& method,
                     const std::string& endpoint,
                     const std::string& data,
                     std::string& response);
    bool parse_sync_response(const std::string& response);
    void handle_room_event(const std::string& room_id,
                          const std::string& event_type,
                          const std::string& event_content);
    void start_sync_loop();
    void stop_sync_loop();
};