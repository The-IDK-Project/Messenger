#pragma once

#include <string>
#include <vector>

struct TableDefinition {
    std::string name;
    std::vector<std::string> columns;
    std::vector<std::string> indexes;
};

class DatabaseSchema {
public:
    static const std::vector<TableDefinition>& get_tables();
    static const std::vector<std::string>& get_migrations();
    static int get_current_version() { return 3; }

    static constexpr const char* TABLE_MESSAGES = "messages";
    static constexpr const char* TABLE_USERS = "users";
    static constexpr const char* TABLE_ROOMS = "rooms";
    static constexpr const char* TABLE_SESSIONS = "sessions";
    static constexpr const char* TABLE_SETTINGS = "settings";

    struct Messages {
        static constexpr const char* ID = "id";
        static constexpr const char* CONTENT = "content";
        static constexpr const char* SENDER_ID = "sender_id";
        static constexpr const char* SENDER_NAME = "sender_name";
        static constexpr const char* ROOM_ID = "room_id";
        static constexpr const char* PROTOCOL = "protocol";
        static constexpr const char* TYPE = "type";
        static constexpr const char* STATUS = "status";
        static constexpr const char* TIMESTAMP = "timestamp";
        static constexpr const char* REPLY_TO_ID = "reply_to_id";
        static constexpr const char* METADATA = "metadata";
        static constexpr const char* CREATED_AT = "created_at";
    };

    struct Users {
        static constexpr const char* ID = "id";
        static constexpr const char* USERNAME = "username";
        static constexpr const char* DISPLAY_NAME = "display_name";
        static constexpr const char* PROTOCOLS = "protocols";
        static constexpr const char* AVATAR_URL = "avatar_url";
        static constexpr const char* LAST_SEEN = "last_seen";
        static constexpr const char* CREATED_AT = "created_at";
    };

    struct Rooms {
        static constexpr const char* ID = "id";
        static constexpr const char* NAME = "name";
        static constexpr const char* PROTOCOL = "protocol";
        static constexpr const char* PARTICIPANTS = "participants";
        static constexpr const char* TYPE = "type";
        static constexpr const char* IS_ENCRYPTED = "is_encrypted";
        static constexpr const char* LAST_MESSAGE_ID = "last_message_id";
        static constexpr const char* LAST_ACTIVITY = "last_activity";
        static constexpr const char* CREATED_AT = "created_at";
    };

    struct Sessions {
        static constexpr const char* PROTOCOL = "protocol";
        static constexpr const char* ACCESS_TOKEN = "access_token";
        static constexpr const char* USER_ID = "user_id";
        static constexpr const char* SERVER_CONFIG = "server_config";
        static constexpr const char* LAST_SYNC_TOKEN = "last_sync_token";
        static constexpr const char* IS_CONNECTED = "is_connected";
        static constexpr const char* LAST_CONNECTED = "last_connected";
        static constexpr const char* CREATED_AT = "created_at";
    };

    struct Settings {
        static constexpr const char* KEY = "key";
        static constexpr const char* VALUE = "value";
        static constexpr const char* UPDATED_AT = "updated_at";
    };

    static std::vector<std::string> get_index_definitions();

    static std::vector<std::string> get_foreign_key_definitions();

    static bool validate_schema(sqlite3* db);
    static std::vector<std::string> get_schema_errors(sqlite3* db);

    static std::string get_create_table_sql(const std::string& table_name);
    static std::string get_drop_table_sql(const std::string& table_name);
    static std::string get_table_info_sql(const std::string& table_name);

private:
    static void initialize_tables();
    static void initialize_migrations();
};