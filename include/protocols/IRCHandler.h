#pragma once

#include "ProtocolHandler.h"
#include <string>
#include <vector>
#include <map>

class IRCHandler : public ProtocolHandler {
public:
    IRCHandler();
    explicit IRCHandler(const std::string& server, int port = 6667);
    ~IRCHandler() override;

    bool connect() override;
    void disconnect() override;
    ProtocolState get_state() const override;
    bool is_connected() const override;

    void set_nickname(const std::string& nickname);
    void set_username(const std::string& username);
    void set_realname(const std::string& realname);
    void set_password(const std::string& password);

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

    bool send_ctcp(const std::string& target, const std::string& message);
    bool send_notice(const std::string& target, const std::string& message);
    bool set_mode(const std::string& channel, const std::string& mode);
    bool whois(const std::string& nickname);
    bool list_channels();

    void sync() override;
    bool supports_sync() const override { return false; }

    void set_message_callback(MessageCallback callback) override;
    void set_room_callback(RoomCallback callback) override;
    void set_user_callback(UserCallback callback) override;
    void set_error_callback(ErrorCallback callback) override;

    std::string get_protocol_name() const override { return "irc"; }
    std::string get_protocol_version() const override { return "3.2"; }
    uint32_t get_capabilities() const override;
    bool has_capability(ProtocolCapabilities capability) const override;

    bool set_config(const std::string& key, const std::string& value) override;
    std::string get_config(const std::string& key) const override;

private:
    std::string server_;
    int port_;
    std::string nickname_;
    std::string username_;
    std::string realname_;
    std::string password_;
    ProtocolState state_;

    int sockfd_;
    std::vector<std::string> channels_;
    std::map<std::string, std::vector<std::string>> channel_users_;

    bool setup_socket();
    bool send_raw(const std::string& message);
    void receive_loop();
    void parse_message(const std::string& message);
    void handle_privmsg(const std::string& from,
                       const std::string& target,
                       const std::string& message);
    void handle_join(const std::string& user, const std::string& channel);
    void handle_part(const std::string& user, const std::string& channel);
    void handle_quit(const std::string& user, const std::string& message);
    void handle_nick(const std::string& old_nick, const std::string& new_nick);
};