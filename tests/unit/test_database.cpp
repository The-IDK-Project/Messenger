#include <gtest/gtest.h>
#include "database/DatabaseManager.h"
#include "core/Message.h"
#include "core/User.h"
#include "core/ChatRoom.h"
#include <filesystem>

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path = ":memory:";
        db = std::make_unique<DatabaseManager>();
        db->initialize(db_path);
    }

    void TearDown() override {
        db->shutdown();
    }

    std::string db_path;
    std::unique_ptr<DatabaseManager> db;
};

TEST_F(DatabaseTest, InitializeDatabase) {
    EXPECT_TRUE(db->is_initialized());
}

TEST_F(DatabaseTest, StoreAndRetrieveMessage) {
    Message msg("Hello, World!", "user1", "Test User", "matrix", "room1");
    msg.id = "test_message_1";

    EXPECT_TRUE(db->store_message(msg));

    auto messages = db->get_messages("room1");
    EXPECT_EQ(messages.size(), 1);

    if (!messages.empty()) {
        EXPECT_EQ(messages[0].id, "test_message_1");
        EXPECT_EQ(messages[0].content, "Hello, World!");
        EXPECT_EQ(messages[0].sender_id, "user1");
        EXPECT_EQ(messages[0].protocol, "matrix");
    }
}

TEST_F(DatabaseTest, StoreAndRetrieveUser) {
    User user("user123", "testuser", "matrix");
    user.display_name = "Test User";

    EXPECT_TRUE(db->store_user(user));

    User retrieved = db->get_user("user123");
    EXPECT_EQ(retrieved.id, "user123");
    EXPECT_EQ(retrieved.username, "testuser");
    EXPECT_EQ(retrieved.display_name, "Test User");
}

TEST_F(DatabaseTest, StoreAndRetrieveRoom) {
    ChatRoom room("room123", "Test Room", "matrix");
    room.add_participant("user1");
    room.add_participant("user2");

    EXPECT_TRUE(db->store_room(room));

    ChatRoom retrieved = db->get_room("room123");
    EXPECT_EQ(retrieved.id, "room123");
    EXPECT_EQ(retrieved.name, "Test Room");
    EXPECT_EQ(retrieved.protocol, "matrix");
    EXPECT_EQ(retrieved.participant_count(), 2);
}

TEST_F(DatabaseTest, MessageSearch) {
    Message msg1("Hello world", "user1", "User One", "matrix", "room1");
    Message msg2("Test message", "user2", "User Two", "irc", "room1");
    Message msg3("Another hello", "user1", "User One", "matrix", "room2");

    msg1.id = "msg1";
    msg2.id = "msg2";
    msg3.id = "msg3";

    db->store_message(msg1);
    db->store_message(msg2);
    db->store_message(msg3);

    auto results = db->search_messages("hello");
    EXPECT_EQ(results.size(), 2);

    auto room_results = db->search_messages("hello", "room1");
    EXPECT_EQ(room_results.size(), 1);
}

TEST_F(DatabaseTest, UpdateMessageStatus) {
    Message msg("Test message", "user1", "User One", "matrix", "room1");
    msg.id = "status_test";
    msg.status = MessageStatus::SENDING;

    db->store_message(msg);

    EXPECT_TRUE(db->update_message_status("status_test", MessageStatus::DELIVERED));

    auto messages = db->get_messages("room1");
    ASSERT_FALSE(messages.empty());
    EXPECT_EQ(messages[0].status, MessageStatus::DELIVERED);
}

TEST_F(DatabaseTest, GetRoomsByProtocol) {
    ChatRoom room1("room1", "Matrix Room", "matrix");
    ChatRoom room2("room2", "IRC Channel", "irc");
    ChatRoom room3("room3", "Another Matrix", "matrix");

    db->store_room(room1);
    db->store_room(room2);
    db->store_room(room3);

    auto matrix_rooms = db->get_rooms_by_protocol("matrix");
    EXPECT_EQ(matrix_rooms.size(), 2);

    auto irc_rooms = db->get_rooms_by_protocol("irc");
    EXPECT_EQ(irc_rooms.size(), 1);
}

TEST_F(DatabaseTest, RecentRooms) {
    ChatRoom room1("room1", "Room 1", "matrix");
    ChatRoom room2("room2", "Room 2", "matrix");
    ChatRoom room3("room3", "Room 3", "matrix");

    db->store_room(room1);
    db->store_room(room2);
    db->store_room(room3);

    auto recent = db->get_recent_rooms(2);
    EXPECT_LE(recent.size(), 2);
}