#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include "../core/Message.h"
#include "../core/User.h"
#include "../core/ChatRoom.h"

class DatabaseManager {
public:
    static DatabaseManager& get_instance();

    bool initialize(const std::string& db_path = "");
    void shutdown();
    bool is_initialized() const { return db_ != nullptr; }

    bool store_message(const Message& message);
    std::vector<Message> get_messages(const std::string& room_id,
                                    int limit = 50,
                                    int offset = 0);
    std::vector<Message> search_messages(const std::string& query,
                                       const std::string& room_id = "",
                                       int limit = 100);
    bool update_message_status(const std::string& message_id,
                             MessageStatus status);
    bool delete_message(const std::string& message_id);
    int get_message_count(const std::string& room_id = "");

    bool store_user(const User& user);
    User get_user(const std::string& user_id);
    std::vector<User> search_users(const std::string& query, int limit = 50);
    std::vector<User> get_users_by_protocol(const std::string& protocol);
    std::vector<User> get_users_by_room(const std::string& room_id);
    bool update_user_presence(const std::string& user_id, bool online);
    bool delete_user(const std::string& user_id);

    bool store_room(const ChatRoom& room);
    ChatRoom get_room(const std::string& room_id);
    std::vector<ChatRoom> get_rooms_by_protocol(const std::string& protocol);
    std::vector<ChatRoom> get_recent_rooms(int limit = 20);
    bool update_room_activity(const std::string& room_id,
                            const std::string& last_message_id);
    bool delete_room(const std::string& room_id);

    bool store_session(const std::string& protocol,
                      const std::string& access_token,
                      const std::string& user_id,
                      const std::string& server_config = "");
    std::string get_session_token(const std::string& protocol);
    std::string get_session_user_id(const std::string& protocol);
    bool delete_session(const std::string& protocol);

    bool store_setting(const std::string& key, const std::string& value);
    std::string get_setting(const std::string& key,
                          const std::string& default_value = "");

    bool backup_database(const std::string& backup_path);
    bool vacuum_database();
    int get_database_size();
    std::string get_database_path() const { return db_path_; }

    bool begin_transaction();
    bool commit_transaction();
    bool rollback_transaction();

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    sqlite3* db_ = nullptr;
    std::string db_path_;
    mutable std::mutex mutex_;

    bool execute_sql(const std::string& sql);
    bool run_migrations();
    void setup_prepared_statements();

    sqlite3_stmt* stmt_insert_message_ = nullptr;
    sqlite3_stmt* stmt_select_messages_ = nullptr;
    sqlite3_stmt* stmt_update_message_status_ = nullptr;
    sqlite3_stmt* stmt_insert_user_ = nullptr;
    sqlite3_stmt* stmt_select_user_ = nullptr;
    sqlite3_stmt* stmt_insert_room_ = nullptr;
    sqlite3_stmt* stmt_select_room_ = nullptr;

    static int message_callback(void* data, int argc, char** argv, char** col_name);
    static int user_callback(void* data, int argc, char** argv, char** col_name);
    static int room_callback(void* data, int argc, char** argv, char** col_name);
};
