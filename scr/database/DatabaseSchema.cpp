#include "database/DatabaseSchema.h"
#include <sqlite3.h>
#include <vector>

static std::vector<TableDefinition> tables;
static std::vector<std::string> migrations;

const std::vector<TableDefinition>& DatabaseSchema::get_tables() {
    if (tables.empty()) {
        initialize_tables();
    }
    return tables;
}

const std::vector<std::string>& DatabaseSchema::get_migrations() {
    if (migrations.empty()) {
        initialize_migrations();
    }
    return migrations;
}

void DatabaseSchema::initialize_tables() {
    TableDefinition messages_table;
    messages_table.name = TABLE_MESSAGES;
    messages_table.columns = {
        "id TEXT PRIMARY KEY",
        "content TEXT NOT NULL",
        "sender_id TEXT NOT NULL",
        "sender_name TEXT NOT NULL",
        "room_id TEXT NOT NULL",
        "protocol TEXT NOT NULL",
        "type INTEGER DEFAULT 0",
        "status INTEGER DEFAULT 0",
        "timestamp INTEGER NOT NULL",
        "reply_to_id TEXT",
        "metadata TEXT",
        "created_at INTEGER DEFAULT (strftime('%s','now'))"
    };
    messages_table.indexes = {
        "CREATE INDEX idx_messages_room_timestamp ON messages(room_id, timestamp DESC)",
        "CREATE INDEX idx_messages_protocol ON messages(protocol)",
        "CREATE INDEX idx_messages_sender ON messages(sender_id)",
        "CREATE INDEX idx_messages_timestamp ON messages(timestamp DESC)"
    };
    tables.push_back(messages_table);

    TableDefinition users_table;
    users_table.name = TABLE_USERS;
    users_table.columns = {
        "id TEXT PRIMARY KEY",
        "username TEXT NOT NULL",
        "display_name TEXT",
        "protocols TEXT",
        "avatar_url TEXT",
        "last_seen INTEGER",
        "created_at INTEGER DEFAULT (strftime('%s','now'))"
    };
    users_table.indexes = {
        "CREATE INDEX idx_users_username ON users(username)",
        "CREATE INDEX idx_users_protocols ON users(protocols)"
    };
    tables.push_back(users_table);

    TableDefinition rooms_table;
    rooms_table.name = TABLE_ROOMS;
    rooms_table.columns = {
        "id TEXT PRIMARY KEY",
        "name TEXT NOT NULL",
        "protocol TEXT NOT NULL",
        "participants TEXT",
        "type INTEGER DEFAULT 0",
        "is_encrypted INTEGER DEFAULT 0",
        "last_message_id TEXT",
        "last_activity INTEGER",
        "created_at INTEGER DEFAULT (strftime('%s','now'))"
    };
    rooms_table.indexes = {
        "CREATE INDEX idx_rooms_protocol ON rooms(protocol)",
        "CREATE INDEX idx_rooms_activity ON rooms(last_activity DESC)",
        "CREATE INDEX idx_rooms_type ON rooms(type)"
    };
    tables.push_back(rooms_table);

    TableDefinition sessions_table;
    sessions_table.name = TABLE_SESSIONS;
    sessions_table.columns = {
        "protocol TEXT PRIMARY KEY",
        "access_token TEXT",
        "user_id TEXT",
        "server_config TEXT",
        "last_sync_token TEXT",
        "is_connected INTEGER DEFAULT 0",
        "last_connected INTEGER",
        "created_at INTEGER DEFAULT (strftime('%s','now'))"
    };
    sessions_table.indexes = {
        "CREATE INDEX idx_sessions_user ON sessions(user_id)",
        "CREATE INDEX idx_sessions_connected ON sessions(is_connected)"
    };
    tables.push_back(sessions_table);

    TableDefinition settings_table;
    settings_table.name = TABLE_SETTINGS;
    settings_table.columns = {
        "key TEXT PRIMARY KEY",
        "value TEXT NOT NULL",
        "updated_at INTEGER DEFAULT (strftime('%s','now'))"
    };
    tables.push_back(settings_table);
}

void DatabaseSchema::initialize_migrations() {
    migrations = {
        R"(
        CREATE TABLE messages (
            id TEXT PRIMARY KEY,
            content TEXT NOT NULL,
            sender_id TEXT NOT NULL,
            sender_name TEXT NOT NULL,
            room_id TEXT NOT NULL,
            protocol TEXT NOT NULL,
            type INTEGER DEFAULT 0,
            status INTEGER DEFAULT 0,
            timestamp INTEGER NOT NULL,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE TABLE users (
            id TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            display_name TEXT,
            protocols TEXT,
            avatar_url TEXT,
            last_seen INTEGER,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE TABLE rooms (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            protocol TEXT NOT NULL,
            participants TEXT,
            type INTEGER DEFAULT 0,
            is_encrypted INTEGER DEFAULT 0,
            last_activity INTEGER,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE TABLE settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            updated_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE INDEX idx_messages_room_timestamp ON messages(room_id, timestamp DESC);
        CREATE INDEX idx_messages_protocol ON messages(protocol);
        CREATE INDEX idx_rooms_protocol ON rooms(protocol);
        )",

        R"(
        ALTER TABLE messages ADD COLUMN reply_to_id TEXT;
        ALTER TABLE messages ADD COLUMN metadata TEXT;
        CREATE INDEX idx_messages_reply ON messages(reply_to_id);
        )",
        R"(
        CREATE TABLE sessions (
            protocol TEXT PRIMARY KEY,
            access_token TEXT,
            user_id TEXT,
            server_config TEXT,
            last_sync_token TEXT,
            is_connected INTEGER DEFAULT 0,
            last_connected INTEGER,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE INDEX idx_sessions_user ON sessions(user_id);
        ALTER TABLE rooms ADD COLUMN last_message_id TEXT;
        )"
    };
}

std::vector<std::string> DatabaseSchema::get_index_definitions() {
    std::vector<std::string> indexes;

    for (const auto& table : get_tables()) {
        indexes.insert(indexes.end(), table.indexes.begin(), table.indexes.end());
    }

    return indexes;
}

bool DatabaseSchema::validate_schema(sqlite3* db) {
    if (!db) return false;

    for (const auto& table : get_tables()) {
        std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_text(stmt, 1, table.name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
    }

    return true;
}