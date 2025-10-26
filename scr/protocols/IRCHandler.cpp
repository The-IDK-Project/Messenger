#include "protocols/IRCHandler.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <sstream>

IRCHandler::IRCHandler()
    : server_("irc.libera.chat")
    , port_(6667)
    , state_(ProtocolState::DISCONNECTED)
    , sockfd_(-1) {
}

IRCHandler::IRCHandler(const std::string& server, int port)
    : server_(server)
    , port_(port)
    , state_(ProtocolState::DISCONNECTED)
    , sockfd_(-1) {
}

IRCHandler::~IRCHandler() {
    disconnect();
}

bool IRCHandler::connect() {
    if (state_ != ProtocolState::DISCONNECTED) {
        LOG_WARNING("IRCHandler already connected or connecting");
        return false;
    }

    state_ = ProtocolState::CONNECTING;

    if (!setup_socket()) {
        state_ = ProtocolState::ERROR;
        return false;
    }
    if (!password_.empty()) {
        send_raw("PASS " + password_);
    }

    send_raw("NICK " + nickname_);
    send_raw("USER " + username_ + " 0 * :" + realname_);

    state_ = ProtocolState::CONNECTED;
    LOG_INFO("IRCHandler connected to " + server_ + ":" + std::to_string(port_));
    std::thread([this]() { receive_loop(); }).detach();

    return true;
}

void IRCHandler::disconnect() {
    if (sockfd_ != -1) {
        if (state_ == ProtocolState::CONNECTED) {
            send_raw("QUIT :Unified Messenger shutdown");
        }
        close(sockfd_);
        sockfd_ = -1;
    }
    state_ = ProtocolState::DISCONNECTED;
    LOG_INFO("IRCHandler disconnected");
}

bool IRCHandler::setup_socket() {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ == -1) {
        LOG_ERROR("Failed to create socket");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, server_.c_str(), &server_addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(server_.c_str());
        if (he == nullptr) {
            LOG_ERROR("Failed to resolve server: " + server_);
            return false;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to connect to IRC server");
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

    return true;
}

bool IRCHandler::send_raw(const std::string& message) {
    if (sockfd_ == -1) return false;

    std::string full_message = message + "\r\n";
    ssize_t sent = ::send(sockfd_, full_message.c_str(), full_message.length(), 0);

    if (sent < 0) {
        LOG_ERROR("Failed to send IRC message");
        return false;
    }

    LOG_DEBUG("IRC >> " + message);
    return true;
}

bool IRCHandler::send_message(const std::string& room_id, const std::string& message) {
    if (!is_connected()) {
        LOG_ERROR("IRCHandler not connected");
        return false;
    }

    return send_raw("PRIVMSG " + room_id + " :" + message);
}

bool IRCHandler::join_room(const std::string& room_id) {
    if (!is_connected()) {
        LOG_ERROR("IRCHandler not connected");
        return false;
    }

    bool result = send_raw("JOIN " + room_id);
    if (result) {
        channels_.push_back(room_id);
    }
    return result;
}

bool IRCHandler::leave_room(const std::string& room_id) {
    if (!is_connected()) {
        return false;
    }

    bool result = send_raw("PART " + room_id);
    if (result) {
        auto it = std::find(channels_.begin(), channels_.end(), room_id);
        if (it != channels_.end()) {
            channels_.erase(it);
        }
    }
    return result;
}

std::vector<ChatRoom> IRCHandler::get_rooms() {
    std::vector<ChatRoom> rooms;

    for (const auto& channel : channels_) {
        ChatRoom room;
        room.id = channel;
        room.name = channel;
        room.protocol = "irc";
        room.type = RoomType::CHANNEL;
        rooms.push_back(room);
    }

    return rooms;
}

User IRCHandler::get_current_user() {
    User user;
    user.id = nickname_;
    user.username = nickname_;
    user.display_name = realname_;
    user.protocols = {"irc"};
    return user;
}

uint32_t IRCHandler::get_capabilities() const {
    return static_cast<uint32_t>(ProtocolCapabilities::MESSAGES);
}

void IRCHandler::receive_loop() {
    char buffer[4096];
    std::string incomplete_line;

    while (state_ == ProtocolState::CONNECTED && sockfd_ != -1) {
        ssize_t received = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);

        if (received > 0) {
            buffer[received] = '\0';
            std::string data = incomplete_line + std::string(buffer);
            incomplete_line.clear();

            std::istringstream stream(data);
            std::string line;

            while (std::getline(stream, line)) {
                if (line.back() == '\r') {
                    line.pop_back();
                }

                if (!line.empty()) {
                    LOG_DEBUG("IRC << " + line);
                    parse_message(line);
                }
            }

            if (!data.empty() && data.back() != '\n') {
                incomplete_line = data.substr(data.find_last_of('\n') + 1);
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

    if (state_ == ProtocolState::CONNECTED) {
        state_ = ProtocolState::DISCONNECTED;
        if (error_callback_) {
            error_callback_("IRC connection lost");
        }
    }
}

void IRCHandler::parse_message(const std::string& message) {
    IRCMessage irc_msg;
    irc_msg.raw = message;
    std::string rest = message;
    if (message[0] == ':') {
        size_t space_pos = message.find(' ');
        if (space_pos != std::string::npos) {
            irc_msg.prefix = message.substr(1, space_pos - 1);
            rest = message.substr(space_pos + 1);
        }
    }
    size_t colon_pos = rest.find(':');
    std::string before_colon;

    if (colon_pos != std::string::npos) {
        before_colon = rest.substr(0, colon_pos);
        irc_msg.trailing = rest.substr(colon_pos + 1);
    } else {
        before_colon = rest;
    }
    std::istringstream before_stream(before_colon);
    std::string token;

    if (before_stream >> token) {
        irc_msg.command = token;

        while (before_stream >> token) {
            irc_msg.params.push_back(token);
        }
    }
    if (irc_msg.command == "PING") {
        send_raw("PONG :" + irc_msg.trailing);
        return;
    }
    if (irc_msg.command == "PRIVMSG") {
        handle_privmsg(irc_msg.prefix, irc_msg.params[0], irc_msg.trailing);
    } else if (irc_msg.command == "JOIN") {
        handle_join(irc_msg.prefix, irc_msg.trailing);
    }
}

void IRCHandler::handle_privmsg(const std::string& from, const std::string& target, const std::string& message) {
    std::string nickname = from;
    size_t exclamation_pos = from.find('!');
    if (exclamation_pos != std::string::npos) {
        nickname = from.substr(0, exclamation_pos);
    }

    Message msg;
    msg.content = message;
    msg.sender_id = nickname;
    msg.sender_name = nickname;
    msg.room_id = target;
    msg.protocol = "irc";
    msg.type = MessageType::TEXT;
    msg.status = MessageStatus::DELIVERED;
    msg.timestamp = std::chrono::system_clock::now();

    if (message_callback_) {
        message_callback_(msg);
    }
}

void IRCHandler::handle_join(const std::string& user, const std::string& channel) {
    std::string nickname = user;
    size_t exclamation_pos = user.find('!');
    if (exclamation_pos != std::string::npos) {
        nickname = user.substr(0, exclamation_pos);
    }

    channel_users_[channel].push_back(nickname);

    if (nickname != nickname_) { // Don't show our own joins
        Message msg;
        msg.content = nickname + " joined " + channel;
        msg.sender_id = "system";
        msg.sender_name = "System";
        msg.room_id = channel;
        msg.protocol = "irc";
        msg.type = MessageType::SYSTEM;
        msg.status = MessageStatus::DELIVERED;
        msg.timestamp = std::chrono::system_clock::now();

        if (message_callback_) {
            message_callback_(msg);
        }
    }
}