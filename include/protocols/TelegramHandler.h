#pragma once

#include "ProtocolHandler.h"
#include <string>
#include <memory>

namespace td {
    class Client;
}

class TelegramHandler : public ProtocolHandler {
public:
    TelegramHandler();
    explicit TelegramHandler(const std::string& api_id,
                           const std::string& api_hash);
    ~TelegramHandler() override;

    bool connect() override;
    void disconnect() override;
    ProtocolState get_state() const override;
    bool is_connected() const override;

    bool set_phone_number(const std::string& phone_number);
    bool send_code();
    bool sign_in(const std::string& code);
    bool sign_in_with_password(const std::string& password);

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

    bool create_secret_chat(const std::string& user_id);
    bool download_file(const std::string& file_id,
                      const std::string& local_path);
    bool set_online(bool online);
    bool search_contacts(const std::string& query);

    void sync() override;
    bool supports_sync() const override { return true; }

    void set_message_callback(MessageCallback callback) override;
    void set_room_callback(RoomCallback callback) override;
    void set_user_callback(UserCallback callback) override;
    void set_error_callback(ErrorCallback callback) override;

    std::string get_protocol_name() const override { return "telegram"; }
    std::string get_protocol_version() const override { return "1.0"; }
    uint32_t get_capabilities() const override;
    bool has_capability(ProtocolCapabilities capability) const override;

    bool set_config(const std::string& key, const std::string& value) override;
    std::string get_config(const std::string& key) const override;

private:
    std::string api_id_;
    std::string api_hash_;
    std::string phone_number_;
    ProtocolState state_;

    std::unique_ptr<td::Client> td_client_;

    void process_updates();
    void handle_update(const std::string& update);
    void handle_message(const std::string& message_data);
    void handle_chat_list(const std::string& chat_list_data);
    void handle_user_status(const std::string& user_data);

    bool td_send(const std::string& query);
    std::string td_receive(double timeout = 1.0);

    int auth_state_;
    bool is_authorized_;
    std::string current_user_id_;
};