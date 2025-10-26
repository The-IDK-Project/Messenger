#include <gtest/gtest.h>
#include "protocols/IRCHandler.h"

class IRCIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<IRCHandler>("irc.libera.chat", 6667);
        handler->set_nickname("testbot123");
        handler->set_username("testuser", "Test User");
    }

    void TearDown() override {
        if (handler->is_connected()) {
            handler->disconnect();
        }
    }

    std::unique_ptr<IRCHandler> handler;
};

TEST_F(IRCIntegrationTest, DISABLED_ConnectionTest) {


    bool connected = handler->connect();
    if (connected) {

        bool registered = handler->register_user();
        EXPECT_TRUE(registered);

        EXPECT_TRUE(handler->is_connected());
        EXPECT_TRUE(handler->is_registered());
    } else {
        GTEST_SKIP() << "Could not connect to IRC server";
    }
}

TEST_F(IRCIntegrationTest, DISABLED_ChannelOperations) {
    GTEST_SKIP() << "IRC integration tests require manual validation";
}