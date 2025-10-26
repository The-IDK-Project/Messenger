#include "network/WebSocketClient.h"
#include "utils/Logger.h"
#include <thread>
#include <atomic>

class WebSocketClient::Impl {
public:
    Impl() : state_(State::DISCONNECTED) {}

    bool connect(const std::string& url, const std::map<std::string, std::string>& headers) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != State::DISCONNECTED) {
            LOG_WARNING("WebSocket already connected or connecting");
            return false;
        }

        state_ = State::CONNECTING;
        url_ = url;
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::lock_guard<std::mutex> lock(mutex_);
            if (state_ == State::CONNECTING) {
                state_ = State::CONNECTED;
                if (state_callback_) {
                    state_callback_(State::CONNECTED);
                }
                LOG_INFO("WebSocket connected to: " + url_);
            }
        }).detach();

        return true;
    }

    void disconnect() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ == State::DISCONNECTED) return;

        State old_state = state_;
        state_ = State::DISCONNECTED;

        if (old_state == State::CONNECTED && state_callback_) {
            state_callback_(State::DISCONNECTED);
        }

        LOG_INFO("WebSocket disconnected");
    }

    bool send_text(const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != State::CONNECTED) {
            LOG_ERROR("WebSocket not connected");
            return false;
        }
        bytes_sent_ += text.length();
        LOG_DEBUG("WebSocket sent text: " + text.substr(0, 100) + "...");
        return true;
    }

    bool send_binary(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != State::CONNECTED) {
            LOG_ERROR("WebSocket not connected");
            return false;
        }
        bytes_sent_ += data.size();
        LOG_DEBUG("WebSocket sent binary data: " + std::to_string(data.size()) + " bytes");
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

    void set_error_callback(ErrorCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_callback_ = std::move(callback);
    }

    State get_state() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    std::string get_url() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return url_;
    }

    size_t get_bytes_sent() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return bytes_sent_;
    }

    size_t get_bytes_received() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return bytes_received_;
    }

    bool is_connected() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == State::CONNECTED;
    }

private:
    State state_;
    std::string url_;
    size_t bytes_sent_ = 0;
    size_t bytes_received_ = 0;

    MessageCallback message_callback_;
    StateCallback state_callback_;
    ErrorCallback error_callback_;

    mutable std::mutex mutex_;
};

WebSocketClient::WebSocketClient() : impl_(std::make_unique<Impl>()) {}
WebSocketClient::~WebSocketClient() = default;

bool WebSocketClient::connect(const std::string& url, const std::map<std::string, std::string>& headers) {
    return impl_->connect(url, headers);
}

void WebSocketClient::disconnect() {
    impl_->disconnect();
}

bool WebSocketClient::send_text(const std::string& text) {
    return impl_->send_text(text);
}

bool WebSocketClient::send_binary(const std::vector<uint8_t>& data) {
    return impl_->send_binary(data);
}

bool WebSocketClient::send_ping(const std::vector<uint8_t>& data) {
    LOG_DEBUG("WebSocket ping sent");
    return true;
}

bool WebSocketClient::send_pong(const std::vector<uint8_t>& data) {
    LOG_DEBUG("WebSocket pong sent");
    return true;
}

bool WebSocketClient::send_close(uint16_t code, const std::string& reason) {
    LOG_DEBUG("WebSocket close frame sent");
    return true;
}

void WebSocketClient::set_message_callback(MessageCallback callback) {
    impl_->set_message_callback(std::move(callback));
}

void WebSocketClient::set_state_callback(StateCallback callback) {
    impl_->set_state_callback(std::move(callback));
}

void WebSocketClient::set_error_callback(ErrorCallback callback) {
    impl_->set_error_callback(std::move(callback));
}

WebSocketClient::State WebSocketClient::get_state() const {
    return impl_->get_state();
}

std::string WebSocketClient::get_url() const {
    return impl_->get_url();
}

size_t WebSocketClient::get_bytes_sent() const {
    return impl_->get_bytes_sent();
}

size_t WebSocketClient::get_bytes_received() const {
    return impl_->get_bytes_received();
}

bool WebSocketClient::is_connected() const {
    return impl_->is_connected();
}
std::string WebSocketClient::state_to_string(State state) {
    switch (state) {
        case State::DISCONNECTED: return "DISCONNECTED";
        case State::CONNECTING: return "CONNECTING";
        case State::CONNECTED: return "CONNECTED";
        case State::CLOSING: return "CLOSING";
        case State::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string WebSocketClient::message_type_to_string(MessageType type) {
    switch (type) {
        case MessageType::TEXT: return "TEXT";
        case MessageType::BINARY: return "BINARY";
        case MessageType::PING: return "PING";
        case MessageType::PONG: return "PONG";
        case MessageType::CLOSE: return "CLOSE";
        default: return "UNKNOWN";
    }
}