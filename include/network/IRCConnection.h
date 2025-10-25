#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

class IRCConnection {
public:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        REGISTERING,
        ERROR
    };

    enum class MessageType {
        UNKNOWN,
        PRIVMSG,
        NOTICE,
        JOIN,
        PART,
        QUIT,
        NICK,
        MODE,
        TOPIC,
        INVITE,
        KICK,
        PING,
        PONG,
        NUMERIC,
        ERROR
    };

    struct IRCMessage {
        MessageType type = MessageType::UNKNOWN;
        std::string prefix;
        std::string command;
        std::vector<std::string> params;
        std::string trailing;
        std::string raw;
        int numeric = 0;
    };

    using MessageCallback = std::function<void(const IRCMessage& message)>;
    using StateCallback = std::function<void(State state)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    using RawCallback = std::function<void(const std::string& line)>;

    IRCConnection();
    ~IRCConnection();

    bool connect(const std::string& server,
                int port = 6667,
                bool ssl = false);
    void disconnect();
    bool reconnect();
    State get_state() const;

    void set_nickname(const std::string& nickname);
    void set_username(const std::string& username,
                     const std::string& realname = "");
    void set_password(const std::string& password);
    bool register_user();

    bool send_raw(const std::string& message);
    bool send_privmsg(const std::string& target, const std::string& message);
    bool send_notice(const std::string& target, const std::string& message);
    bool send_action(const std::string& target, const std::string& action);
    bool send_ctcp(const std::string& target, const std::string& message);
    bool send_ctcp_reply(const std::string& target, const std::string& message);

    bool join_channel(const std::string& channel);
    bool part_channel(const std::string& channel, const std::string& reason = "");
    bool set_topic(const std::string& channel, const std::string& topic);
    bool set_mode(const std::string& target, const std::string& mode);
    bool invite(const std::string& user, const std::string& channel);
    bool kick(const std::string& channel, const std::string& user,
              const std::string& reason = "");

    bool whois(const std::string& nickname);
    bool whowas(const std::string& nickname);
    bool list_channels(const std::string& filter = "");

    bool quit(const std::string& reason = "");
    bool pong(const std::string& server);

    void set_message_callback(MessageCallback callback);
    void set_state_callback(StateCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_raw_callback(RawCallback callback);

    void set_timeout(int timeout_ms);
    void set_ssl_verify(bool verify);
    void set_encoding(const std::string& encoding);
    void set_flood_protection(bool enable, int delay_ms = 1000);

    std::string get_server() const;
    int get_port() const;
    std::string get_nickname() const;
    bool is_connected() const;
    bool is_registered() const;

    static IRCMessage parse_message(const std::string& line);
    static std::string build_message(const std::string& command,
                                   const std::vector<std::string>& params = {});
    static std::string state_to_string(State state);
    static std::string message_type_to_string(MessageType type);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    IRCConnection(const IRCConnection&) = delete;
    IRCConnection& operator=(const IRCConnection&) = delete;

    void process_line(const std::string& line);
    void handle_numeric(const IRCMessage& msg);
    void handle_ping(const IRCMessage& msg);
};