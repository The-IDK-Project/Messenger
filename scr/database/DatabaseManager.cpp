#include "database/DatabaseManager.h"
#include "database/DatabaseSchema.h"
#include "database/Migrations.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <sqlite3.h>
#include <filesystem>

DatabaseManager& DatabaseManager::get_instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() : db_(nullptr) {
}

DatabaseManager::~DatabaseManager() {
    shutdown();
}

bool DatabaseManager::initialize(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (db_) {
        LOG_WARNING("Database already initialized");
        return true;
    }

    db_path_ = db_path.empty() ?
        std::string(std::getenv("HOME")) + "/.unified-messenger/messages.db" :
        db_path;

    std::filesystem::path fs_path(db_path_);
    std::filesystem::create_directories(fs_path.parent_path());

    int result = sqlite3_open(db_path_.c_str(), &db_);
    if (result != SQLITE_OK) {
        LOG_ERROR("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    execute_sql("PRAGMA foreign_keys = ON;");
    execute_sql("PRAGMA journal_mode = WAL;");
    execute_sql("PRAGMA synchronous = NORMAL;");

    if (!run_migrations()) {
        LOG_ERROR("Failed to run database migrations");
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    setup_prepared_statements();

    LOG_INFO("Database initialized: " + db_path_);
    return true;
}

void DatabaseManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!db_) return;

    if (stmt_insert_message_) sqlite3_finalize(stmt_insert_message_);
    if (stmt_select_messages_) sqlite3_finalize(stmt_select_messages_);
    if (stmt_update_message_status_) sqlite3_finalize(stmt_update_message_status_);
    if (stmt_insert_user_) sqlite3_finalize(stmt_insert_user_);
    if (stmt_select_user_) sqlite3_finalize(stmt_select_user_);
    if (stmt_insert_room_) sqlite3_finalize(stmt_insert_room_);
    if (stmt_select_room_) sqlite3_finalize(stmt_select_room_);

    sqlite3_close(db_);
    db_ = nullptr;

    LOG_INFO("Database shutdown");
}

bool DatabaseManager::store_message(const Message& message) {
    if (!db_) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_reset(stmt_insert_message_);

    sqlite3_bind_text(stmt_insert_message_, 1, message.id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_insert_message_, 2, message.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_insert_message_, 3, message.sender_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_insert_message_, 4, message.sender_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_insert_message_, 5, message.room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_insert_message_, 6, message.protocol.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt_insert_message_, 7, static_cast<int>(message.type));
    sqlite3_bind_int(stmt_insert_message_, 8, static_cast<int>(message.status));
    sqlite3_bind_int64(stmt_insert_message_, 9,
        std::chrono::duration_cast<std::chrono::seconds>(message.timestamp.time_since_epoch()).count());

    if (message.reply_to_id.empty()) {
        sqlite3_bind_null(stmt_insert_message_, 10);
    } else {
        sqlite3_bind_text(stmt_insert_message_, 10, message.reply_to_id.c_str(), -1, SQLITE_STATIC);
    }

    if (message.metadata.empty()) {
        sqlite3_bind_null(stmt_insert_message_, 11);
    } else {
        sqlite3_bind_text(stmt_insert_message_, 11, message.metadata.c_str(), -1, SQLITE_STATIC);
    }

    int result = sqlite3_step(stmt_insert_message_);
    if (result != SQLITE_DONE) {
        LOG_ERROR("Failed to store message: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    return true;
}

std::vector<Message> DatabaseManager::get_messages(const std::string& room_id, int limit, int offset) {
    std::vector<Message> messages;

    if (!db_) return messages;

    std::lock_guard<std::mutex> lock(mutex_);

    std::string sql = "SELECT * FROM messages WHERE room_id = ? ORDER BY timestamp DESC LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        return messages;
    }

    sqlite3_bind_text(stmt, 1, room_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message msg;
        msg.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        msg.sender_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.sender_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.room_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        msg.protocol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        msg.type = static_cast<MessageType>(sqlite3_column_int(stmt, 6));
        msg.status = static_cast<MessageStatus>(sqlite3_column_int(stmt, 7));

        int64_t timestamp = sqlite3_column_int64(stmt, 8);
        msg.timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(timestamp));

        if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
            msg.reply_to_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        }

        if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) {
            msg.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        }

        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);

    std::reverse(messages.begin(), messages.end());

    return messages;
}

bool DatabaseManager::store_user(const User& user) {
    if (!db_) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    User existing = get_user(user.id);
    if (!existing.id.empty()) {
        std::string sql = "UPDATE users SET username = ?, display_name = ?, protocols = ?, "
                         "avatar_url = ?, last_seen = ? WHERE id = ?";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user.display_name.c_str(), -1, SQLITE_STATIC);

        std::string protocols_json = user.to_json(); // This contains protocols array
        sqlite3_bind_text(stmt, 3, protocols_json.c_str(), -1, SQLITE_STATIC);

        sqlite3_bind_text(stmt, 4, user.avatar_url.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 5,
            std::chrono::duration_cast<std::chrono::seconds>(user.last_seen.time_since_epoch()).count());
        sqlite3_bind_text(stmt, 6, user.id.c_str(), -1, SQLITE_STATIC);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return result == SQLITE_DONE;
    } else {
        sqlite3_reset(stmt_insert_user_);

        sqlite3_bind_text(stmt_insert_user_, 1, user.id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_insert_user_, 2, user.username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_insert_user_, 3, user.display_name.c_str(), -1, SQLITE_STATIC);

        std::string protocols_json = user.to_json();
        sqlite3_bind_text(stmt_insert_user_, 4, protocols_json.c_str(), -1, SQLITE_STATIC);

        sqlite3_bind_text(stmt_insert_user_, 5, user.avatar_url.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt_insert_user_, 6,
            std::chrono::duration_cast<std::chrono::seconds>(user.last_seen.time_since_epoch()).count());

        int result = sqlite3_step(stmt_insert_user_);
        return result == SQLITE_DONE;
    }
}

std::vector<User> DatabaseManager::search_users(const std::string& query, int limit) {
    std::vector<User> users;
    if (!db_) return users;

    std::lock_guard<std::mutex> lock(mutex_);

    std::string sql = "SELECT * FROM users WHERE username LIKE ? OR display_name LIKE ? LIMIT ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR("Failed to prepare search_users statement: " + std::string(sqlite3_errmsg(db_)));
        return users;
    }

    std::string like_query = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, like_query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, like_query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.display_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        // protocols (column 3) are stored as JSON, would need a JSON parser to properly fill
        user.avatar_url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        int64_t last_seen_ts = sqlite3_column_int64(stmt, 5);
        user.last_seen = std::chrono::system_clock::time_point(std::chrono::seconds(last_seen_ts));
        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users;
}

bool DatabaseManager::run_migrations() {
    return Migrations::run_migrations(db_);
}

void DatabaseManager::setup_prepared_statements() {
    const char* insert_message_sql =
        "INSERT INTO messages (id, content, sender_id, sender_name, room_id, protocol, "
        "type, status, timestamp, reply_to_id, metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(db_, insert_message_sql, -1, &stmt_insert_message_, nullptr);
    const char* insert_user_sql =
        "INSERT INTO users (id, username, display_name, protocols, avatar_url, last_seen) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(db_, insert_user_sql, -1, &stmt_insert_user_, nullptr);
}

bool DatabaseManager::execute_sql(const std::string& sql) {
    if (!db_) return false;

    char* error_msg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_msg);

    if (result != SQLITE_OK) {
        LOG_ERROR("SQL error: " + std::string(error_msg));
        sqlite3_free(error_msg);
        return false;
    }

    return true;
}