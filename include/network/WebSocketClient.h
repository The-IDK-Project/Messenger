#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>

class WebSocketClient {
public:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        CLOSING,
        ERROR
    };

    enum class MessageType {
        TEXT,
        BINARY,
        PING,
        PONG,
        CLOSE
    };

    using MessageCallback = std::function<void(MessageType type,
                                             const std::vector<uint8_t>& data)>;
    using StateCallback = std::function<void(State state)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    WebSocketClient();
    ~WebSocketClient();

    bool connect(const std::string& url,
                const std::map<std::string, std::string>& headers = {});
    void disconnect();
    bool reconnect();
    State get_state() const;

    bool send_text(const std::string& text);
    bool send_binary(const std::vector<uint8_t>& data);
    bool send_ping(const std::vector<uint8_t>& data = {});
    bool send_pong(const std::vector<uint8_t>& data = {});
    bool send_close(uint16_t code = 1000, const std::string& reason = "");

    void set_message_callback(MessageCallback callback);
    void set_state_callback(StateCallback callback);
    void set_error_callback(ErrorCallback callback);

    void set_timeout(int timeout_ms);
    void set_max_message_size(size_t max_size);
    void set_auto_reconnect(bool auto_reconnect, int max_attempts = 5);
    void set_ssl_verify(bool verify);

    std::string get_url() const;
    size_t get_bytes_sent() const;
    size_t get_bytes_received() const;
    bool is_connected() const;

    static std::string state_to_string(State state);
    static std::string message_type_to_string(MessageType type);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
};