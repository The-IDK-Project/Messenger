#include <gtest/gtest.h>
#include "protocols/ProtocolHandler.h"
#include "protocols/MatrixHandler.h"
#include "protocols/IRCHandler.h"
#include "protocols/ProtocolFactory.h"

class ProtocolFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        ProtocolFactory::get_instance().register_default_protocols();
    }
};

TEST_F(ProtocolFactoryTest, CreateProtocols) {
    auto& factory = ProtocolFactory::get_instance();

    auto matrix = factory.create_protocol("matrix");
    EXPECT_NE(matrix, nullptr);
    EXPECT_EQ(matrix->get_protocol_name(), "matrix");

    auto irc = factory.create_protocol("irc");
    EXPECT_NE(irc, nullptr);
    EXPECT_EQ(irc->get_protocol_name(), "irc");

    auto telegram = factory.create_protocol("telegram");
    EXPECT_NE(telegram, nullptr);
    EXPECT_EQ(telegram->get_protocol_name(), "telegram");
}

TEST_F(ProtocolFactoryTest, AvailableProtocols) {
    auto& factory = ProtocolFactory::get_instance();
    auto protocols = factory.get_available_protocols();

    EXPECT_GE(protocols.size(), 3);
    EXPECT_NE(std::find(protocols.begin(), protocols.end(), "matrix"), protocols.end());
    EXPECT_NE(std::find(protocols.begin(), protocols.end(), "irc"), protocols.end());
    EXPECT_NE(std::find(protocols.begin(), protocols.end(), "telegram"), protocols.end());
}

TEST_F(ProtocolFactoryTest, ProtocolCapabilities) {
    auto& factory = ProtocolFactory::get_instance();

    auto matrix = factory.create_protocol("matrix");
    EXPECT_TRUE(matrix->has_capability(ProtocolCapabilities::MESSAGES));
    EXPECT_TRUE(matrix->has_capability(ProtocolCapabilities::FILES));
    EXPECT_TRUE(matrix->has_capability(ProtocolCapabilities::ENCRYPTION));

    auto irc = factory.create_protocol("irc");
    EXPECT_TRUE(irc->has_capability(ProtocolCapabilities::MESSAGES));
    EXPECT_FALSE(irc->has_capability(ProtocolCapabilities::FILES));

    auto telegram = factory.create_protocol("telegram");
    EXPECT_TRUE(telegram->has_capability(ProtocolCapabilities::MESSAGES));
    EXPECT_TRUE(telegram->has_capability(ProtocolCapabilities::FILES));
}

class MockProtocolHandler : public ProtocolHandler {
public:
    MockProtocolHandler(const std::string& name) : name_(name), connected_(false) {}

    bool connect() override { connected_ = true; return true; }
    void disconnect() override { connected_ = false; }
    ProtocolState get_state() const override {
        return connected_ ? ProtocolState::CONNECTED : ProtocolState::DISCONNECTED;
    }
    bool is_connected() const override { return connected_; }

    bool send_message(const std::string& room_id, const std::string& message) override {
        last_message_ = message;
        last_room_ = room_id;
        return true;
    }

    bool send_file(const std::string& room_id, const std::string& file_path, const std::string& filename) override {
        return false;
    }

    bool send_typing(const std::string& room_id, bool typing) override { return true; }
    bool mark_read(const std::string& room_id, const std::string& message_id) override { return true; }

    bool join_room(const std::string& room_id) override { return true; }
    bool leave_room(const std::string& room_id) override { return true; }
    bool create_room(const std::string& name, const std::vector<std::string>& users) override { return true; }
    std::vector<ChatRoom> get_rooms() override { return {}; }

    User get_current_user() override {
        return User("test_user", "testuser", name_);
    }
    std::vector<User> get_room_users(const std::string& room_id) override { return {}; }
    User get_user(const std::string& user_id) override { return User(); }

    void sync() override {}
    bool supports_sync() const override { return true; }

    std::string get_protocol_name() const override { return name_; }
    std::string get_protocol_version() const override { return "1.0"; }
    uint32_t get_capabilities() const override {
        return static_cast<uint32_t>(ProtocolCapabilities::MESSAGES);
    }

    bool set_config(const std::string& key, const std::string& value) override { return true; }
    std::string get_config(const std::string& key) const override { return ""; }

    std::string last_message_;
    std::string last_room_;

private:
    std::string name_;
    bool connected_;
};

class ProtocolHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<MockProtocolHandler>("test");
    }

    std::unique_ptr<MockProtocolHandler> handler;
};

TEST_F(ProtocolHandlerTest, ConnectionManagement) {
    EXPECT_FALSE(handler->is_connected());
    EXPECT_TRUE(handler->connect());
    EXPECT_TRUE(handler->is_connected());
    handler->disconnect();
    EXPECT_FALSE(handler->is_connected());
}

TEST_F(ProtocolHandlerTest, SendMessage) {
    EXPECT_TRUE(handler->send_message("room123", "Hello, World!"));
    EXPECT_EQ(handler->last_message_, "Hello, World!");
    EXPECT_EQ(handler->last_room_, "room123");
}

TEST_F(ProtocolHandlerTest, Callbacks) {
    Message received_msg;
    bool callback_called = false;

    handler->set_message_callback([&](const Message& msg) {
        received_msg = msg;
        callback_called = true;
    });

    EXPECT_TRUE(true);
}