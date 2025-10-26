#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include "../core/User.h"
#include "../core/Config.h"
#include "../database/DatabaseManager.h"

struct Session {
    std::string protocol;
    std::string user_id;
    std::string access_token;
    std::string refresh_token;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_used;
    bool is_valid;
    std::map<std::string, std::string> server_config;

    bool is_expired() const;
    bool needs_refresh() const;
    int64_t get_remaining_seconds() const;
};

class SessionManager {
public:
    static SessionManager& get_instance();

    bool create_session(const Session& session);
    bool update_session(const Session& session);
    bool delete_session(const std::string& protocol);
    bool refresh_session(const std::string& protocol);

    Session get_session(const std::string& protocol) const;
    std::vector<Session> get_all_sessions() const;
    bool has_session(const std::string& protocol) const;
    bool is_session_valid(const std::string& protocol) const;

    std::string get_access_token(const std::string& protocol) const;
    std::string get_refresh_token(const std::string& protocol) const;
    bool set_access_token(const std::string& protocol, const std::string& token);
    bool set_refresh_token(const std::string& protocol, const std::string& token);

    User get_session_user(const std::string& protocol) const;
    bool set_session_user(const std::string& protocol, const User& user);
    std::string get_session_user_id(const std::string& protocol) const;

    bool set_server_config(const std::string& protocol,
                          const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> get_server_config(const std::string& protocol) const;
    std::string get_server_config_value(const std::string& protocol,
                                      const std::string& key) const;

    bool validate_all_sessions();
    void invalidate_expired_sessions();
    std::vector<std::string> get_expired_sessions() const;
    std::vector<std::string> get_sessions_needing_refresh() const;

    bool encrypt_sessions();
    bool decrypt_sessions();
    void clear_sensitive_data();
    bool change_encryption_key(const std::string& new_key);

    bool load_sessions();
    bool save_sessions();
    bool import_sessions(const std::string& file_path);
    bool export_sessions(const std::string& file_path);

    size_t get_session_count() const;
    size_t get_valid_session_count() const;
    std::chrono::system_clock::time_point get_oldest_session() const;
    std::chrono::system_clock::time_point get_newest_session() const;

    static std::string generate_session_id();
    static bool validate_token_format(const std::string& token);
    static std::chrono::system_clock::time_point calculate_expiry(int expires_in_seconds);

private:
    SessionManager();
    ~SessionManager();
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    void initialize();
    void cleanup();
    bool load_from_database();
    bool save_to_database();

    std::map<std::string, Session> sessions_;
    std::unique_ptr<DatabaseManager> database_;
    std::unique_ptr<Config> config_;
    std::string encryption_key_;
    bool sessions_loaded_ = false;

    mutable std::mutex sessions_mutex_;
};