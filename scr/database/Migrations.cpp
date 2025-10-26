#include "database/Migrations.h"
#include "utils/Logger.h"
#include <sqlite3.h>

std::vector<Migration> Migrations::migrations_;
bool Migrations::initialized_ = false;

bool Migrations::run_migrations(sqlite3* db) {
    if (!db) return false;

    initialize_migrations();

    if (!create_migrations_table(db)) {
        return false;
    }

    int current_version = get_current_version(db);
    int target_version = migrations_.size();

    for (int version = current_version + 1; version <= target_version; version++) {
        if (version - 1 < static_cast<int>(migrations_.size())) {
            const Migration& migration = migrations_[version - 1];

            LOG_INFO("Running migration v" + std::to_string(version) + ": " + migration.description);

            if (migration.up && !migration.up(db)) {
                LOG_ERROR("Migration v" + std::to_string(version) + " failed");
                return false;
            }

            if (!record_migration(db, version, true)) {
                LOG_ERROR("Failed to record migration v" + std::to_string(version));
                return false;
            }

            LOG_INFO("Migration v" + std::to_string(version) + " completed successfully");
        }
    }

    return true;
}

bool Migrations::migration_v1(sqlite3* db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS messages (
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

        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            display_name TEXT,
            protocols TEXT,
            avatar_url TEXT,
            last_seen INTEGER,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE TABLE IF NOT EXISTS rooms (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            protocol TEXT NOT NULL,
            participants TEXT,
            type INTEGER DEFAULT 0,
            is_encrypted INTEGER DEFAULT 0,
            last_activity INTEGER,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            updated_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE INDEX IF NOT EXISTS idx_messages_room_timestamp ON messages(room_id, timestamp DESC);
        CREATE INDEX IF NOT EXISTS idx_messages_protocol ON messages(protocol);
        CREATE INDEX IF NOT EXISTS idx_rooms_protocol ON rooms(protocol);
    )";

    char* error_msg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &error_msg);

    if (result != SQLITE_OK) {
        if (error_msg) {
            LOG_ERROR("Migration v1 failed: " + std::string(error_msg));
            sqlite3_free(error_msg);
        }
        return false;
    }

    return true;
}

bool Migrations::migration_v2(sqlite3* db) {
    const char* sql = R"(
        ALTER TABLE messages ADD COLUMN reply_to_id TEXT;
        ALTER TABLE messages ADD COLUMN metadata TEXT;
        CREATE INDEX IF NOT EXISTS idx_messages_reply ON messages(reply_to_id);
    )";

    char* error_msg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &error_msg);

    if (result != SQLITE_OK) {
        if (error_msg) {
            LOG_ERROR("Migration v2 failed: " + std::string(error_msg));
            sqlite3_free(error_msg);
        }
        return false;
    }

    return true;
}

bool Migrations::migration_v3(sqlite3* db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            protocol TEXT PRIMARY KEY,
            access_token TEXT,
            user_id TEXT,
            server_config TEXT,
            last_sync_token TEXT,
            is_connected INTEGER DEFAULT 0,
            last_connected INTEGER,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);
        ALTER TABLE rooms ADD COLUMN last_message_id TEXT;
    )";

    char* error_msg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &error_msg);

    if (result != SQLITE_OK) {
        if (error_msg) {
            LOG_ERROR("Migration v3 failed: " + std::string(error_msg));
            sqlite3_free(error_msg);
        }
        return false;
    }

    return true;
}

void Migrations::initialize_migrations() {
    if (initialized_) return;

    migrations_ = {
        {1, "Initial schema", migration_v1, rollback_v1},
        {2, "Add message reply and metadata", migration_v2, rollback_v2},
        {3, "Add session management", migration_v3, rollback_v3}
    };

    initialized_ = true;
}

bool Migrations::create_migrations_table(sqlite3* db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS migrations (
            version INTEGER PRIMARY KEY,
            applied_at INTEGER DEFAULT (strftime('%s','now')),
            description TEXT
        )
    )";

    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool Migrations::record_migration(sqlite3* db, int version, bool applied) {
    if (applied) {
        const char* sql = "INSERT OR REPLACE INTO migrations (version, description) VALUES (?, ?)";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_int(stmt, 1, version);

        std::string description = "Migration v" + std::to_string(version);
        if (version - 1 < static_cast<int>(migrations_.size())) {
            description = migrations_[version - 1].description;
        }

        sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return result == SQLITE_DONE;
    } else {
        const char* sql = "DELETE FROM migrations WHERE version = ?";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_int(stmt, 1, version);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return result == SQLITE_DONE;
    }
}

int Migrations::get_current_version(sqlite3* db) {
    const char* sql = "SELECT MAX(version) FROM migrations";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return version;
}