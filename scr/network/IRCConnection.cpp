#include "network/IRCConnection.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <sstream>

class IRCConnection::Impl {
public:
    Impl() : state_(State::DISCONNECTED), sockfd_(-1) {}

    ~Impl() {
        disconnect();
    }

    bool connect(const std::string& server, int port, bool ssl) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != State::DISCONNECTED) {
            LOG_WARNING("IRC already connected or connecting");
            return false;
        }

        state_ = State::CONNECTING;
        server_ = server;
        port_ = port;

        if (!setup_socket()) {
            state_ = State::ERROR;
            return false;
        }

        receive_thread_ = std::thread([this]() { receive_loop(); });

        return true;
    }

    void disconnect() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (state_ == State::DISCONNECTED) return;

            state_ = State::DISCONNECTED;
        }

        if (sockfd_ != -1) {
            ::close(sockfd_);
            sockfd_ = -1;
        }

        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        if (state_callback_) {
            state_callback_(State::DISCONNECTED);
        }

        LOG_INFO("IRC disconnected");
    }

    bool send_raw(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != State::CONNECTED && state_ != State::REGISTERING) {
            LOG_ERROR("IRC not connected");
            return false;
        }

        if (sockfd_ == -1) return false;

        std::string full_message = message + "\r\n";
        ssize_t sent = ::send(sockfd_, full_message.c_str(), full_message.length(), 0);

        if (sent < 0) {
            LOG_ERROR("Failed to send IRC message");
            return false;
        }

        if (raw_callback_) {
            raw_callback_(">> " + message);
        }

        return true;
    }

    bool send_privmsg(const std::string& target, const std::string& message) {
        return send_raw("PRIVMSG " + target + " :" + message);
    }

    bool send_notice(const std::string& target, const std::string& message) {
        return send_raw("NOTICE " + target + " :" + message);
    }

    bool join_channel(const std::string& channel) {
        return send_raw("JOIN " + channel);
    }

    bool part_channel(const std::string& channel, const std::string& reason) {
        std::string command = "PART " + channel;
        if (!reason.empty()) {
            command += " :" + reason;
        }
        return send_raw(command);
    }

    void set_nickname(const std::string& nickname) {
        std::lock_guard<std::mutex> lock(mutex_);
        nickname_ = nickname;
    }

    void set_username(const std::string& username, const std::string& realname) {
        std::lock_guard<std::mutex> lock(mutex_);
        username_ = username;
        realname_ = realname.empty() ? username : realname;
    }

    bool register_user() {
        if (nickname_.empty() || username_.empty()) {
            LOG_ERROR("Nickname and username must be set before registration");
            return false;
        }

        if (!send_raw("NICK " + nickname_)) return false;
        if (!send_raw("USER " + username_ + " 0 * :" + realname_)) return false;

        state_ = State::REGISTERING;
        return true;
    }

    void set_message_callback(MessageCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        message_callback_ = std::move(callback);
    }

    void set_state_callback(StateCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_callback_ = std::move(callback);
    }

    void set_raw_callback(RawCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        raw_callback_ = std::move(callback);
    }

    State get_state() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    std::string get_server() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return server_;
    }

    int get_port() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return port_;
    }

    std::string get_nickname() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nickname_;
    }

    bool is_connected() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == State::CONNECTED || state_ == State::REGISTERING;
    }

    bool is_registered() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == State::CONNECTED;
    }

private:
    State state_;
    std::string server_;
    int port_ = 6667;
    std::string nickname_;
    std::string username_;
    std::string realname_;
    int sockfd_ = -1;

    MessageCallback message_callback_;
    StateCallback state_callback_;
    RawCallback raw_callback_;
    ErrorCallback error_callback_;

    std::thread receive_thread_;
    mutable std::mutex mutex_;

    bool setup_socket() {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ == -1) {
            LOG_ERROR("Failed to create socket");
            return false;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        struct hostent* he = gethostbyname(server_.c_str());
        if (he == nullptr) {
            LOG_ERROR("Failed to resolve server: " + server_);
            ::close(sockfd_);
            sockfd_ = -1;
            return false;
        }

        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

        if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            LOG_ERROR("Failed to connect to IRC server: " + std::string(strerror(errno)));
            ::close(sockfd_);
            sockfd_ = -1;
            return false;
        }

        int flags = fcntl(sockfd_, F_GETFL, 0);
        fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

        LOG_INFO("IRC connected to " + server_ + ":" + std::to_string(port_));
        return true;
    }

    void receive_loop() {
        char buffer[4096];
        std::string incomplete_line;

        while (true) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (state_ == State::DISCONNECTED) break;
            }

            ssize_t received = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);

            if (received > 0) {
                buffer[received] = '\0';
                std::string data = incomplete_line + std::string(buffer);
                incomplete_line.clear();

                std::vector<std::string> lines;
                std::istringstream stream(data);
                std::string line;

                while (std::getline(stream, line)) {
                    if (line.back() == '\r') {
                        line.pop_back();
                    }
                    if (!line.empty()) {
                        lines.push_back(line);
                    }
                }

                for (const auto& complete_line : lines) {
                    process_line(complete_line);
                }

                if (!data.empty() && data.back() != '\n') {
                    size_t last_newline = data.find_last_of('\n');
                    if (last_newline != std::string::npos) {
                        incomplete_line = data.substr(last_newline + 1);
                    } else {
                        incomplete_line = data;
                    }
                }
            } else if (received == 0) {
                LOG_INFO("IRC connection closed by server");
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("IRC receive error: " + std::string(strerror(errno)));
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        disconnect();
    }

    void process_line(const std::string& line) {
        if (raw_callback_) {
            raw_callback_("<< " + line);
        }

        IRCMessage msg = parse_message(line);

        if (msg.command == "PING") {
            send_raw("PONG :" + msg.trailing);
            return;
        }

        if (std::isdigit(msg.command[0])) {
            msg.numeric = std::stoi(msg.command);
            handle_numeric(msg);
        }

        if (message_callback_) {
            message_callback_(msg);
        }
    }

    void handle_numeric(const IRCMessage& msg) {
        switch (msg.numeric) {
            case 001:
            case 002:
            case 003:
            case 004:
            case 005:
                if (state_ == State::REGISTERING) {
                    state_ = State::CONNECTED;
                    if (state_callback_) {
                        state_callback_(State::CONNECTED);
                    }
                    LOG_INFO("IRC registration completed");
                }
                break;

            case 433:
                if (!nickname_.empty()) {
                    std::string new_nick = nickname_ + "_";
                    set_nickname(new_nick);
                    send_raw("NICK " + new_nick);
                }
                break;
        }
    }
};

IRCConnection::IRCConnection() : impl_(std::make_unique<Impl>()) {}
IRCConnection::~IRCConnection() = default;

bool IRCConnection::connect(const std::string& server, int port, bool ssl) {
    return impl_->connect(server, port, ssl);
}

void IRCConnection::disconnect() {
    impl_->disconnect();
}

bool IRCConnection::send_raw(const std::string& message) {
    return impl_->send_raw(message);
}

bool IRCConnection::send_privmsg(const std::string& target, const std::string& message) {
    return impl_->send_privmsg(target, message);
}

bool IRCConnection::send_notice(const std::string& target, const std::string& message) {
    return impl_->send_notice(target, message);
}

bool IRCConnection::join_channel(const std::string& channel) {
    return impl_->join_channel(channel);
}

bool IRCConnection::part_channel(const std::string& channel, const std::string& reason) {
    return impl_->part_channel(channel, reason);
}

void IRCConnection::set_nickname(const std::string& nickname) {
    impl_->set_nickname(nickname);
}

void IRCConnection::set_username(const std::string& username, const std::string& realname) {
    impl_->set_username(username, realname);
}

bool IRCConnection::register_user() {
    return impl_->register_user();
}

void IRCConnection::set_message_callback(MessageCallback callback) {
    impl_->set_message_callback(std::move(callback));
}

void IRCConnection::set_state_callback(StateCallback callback) {
    impl_->set_state_callback(std::move(callback));
}

void IRCConnection::set_raw_callback(RawCallback callback) {
    impl_->set_raw_callback(std::move(callback));
}

IRCConnection::State IRCConnection::get_state() const {
    return impl_->get_state();
}

std::string IRCConnection::get_server() const {
    return impl_->get_server();
}

int IRCConnection::get_port() const {
    return impl_->get_port();
}

std::string IRCConnection::get_nickname() const {
    return impl_->get_nickname();
}

bool IRCConnection::is_connected() const {
    return impl_->is_connected();
}

bool IRCConnection::is_registered() const {
    return impl_->is_registered();
}

IRCConnection::IRCMessage IRCConnection::parse_message(const std::string& line) {
    IRCMessage msg;
    msg.raw = line;

    std::string rest = line;

    if (!rest.empty() && rest[0] == ':') {
        size_t space_pos = rest.find(' ');
        if (space_pos != std::string::npos) {
            msg.prefix = rest.substr(1, space_pos - 1);
            rest = rest.substr(space_pos + 1);
        }
    }

    size_t colon_pos = rest.find(':');
    std::string before_colon;

    if (colon_pos != std::string::npos) {
        before_colon = StringUtils::trim(rest.substr(0, colon_pos));
        msg.trailing = rest.substr(colon_pos + 1);
    } else {
        before_colon = StringUtils::trim(rest);
    }

    std::vector<std::string> tokens;
    std::istringstream token_stream(before_colon);
    std::string token;

    while (token_stream >> token) {
        tokens.push_back(token);
    }

    if (!tokens.empty()) {
        msg.command = tokens[0];
        for (size_t i = 1; i < tokens.size(); ++i) {
            msg.params.push_back(tokens[i]);
        }
    }

    if (msg.command == "PRIVMSG") {
        msg.type = MessageType::PRIVMSG;
    } else if (msg.command == "NOTICE") {
        msg.type = MessageType::NOTICE;
    } else if (msg.command == "JOIN") {
        msg.type = MessageType::JOIN;
    } else if (msg.command == "PART") {
        msg.type = MessageType::PART;
    } else if (msg.command == "QUIT") {
        msg.type = MessageType::QUIT;
    } else if (msg.command == "NICK") {
        msg.type = MessageType::NICK;
    } else if (msg.command == "PING") {
        msg.type = MessageType::PING;
    } else if (msg.command == "PONG") {
        msg.type = MessageType::PONG;
    } else if (std::isdigit(msg.command[0])) {
        msg.type = MessageType::NUMERIC;
    }

    return msg;
}

std::string IRCConnection::state_to_string(State state) {
    switch (state) {
        case State::DISCONNECTED: return "DISCONNECTED";
        case State::CONNECTING: return "CONNECTING";
        case State::CONNECTED: return "CONNECTED