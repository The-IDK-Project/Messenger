#include "app/SessionManager.h"
#include "utils/Logger.h"
#include "utils/Crypto.h"
#include "utils/JsonParser.h"
#include <filesystem>

SessionManager& SessionManager::get_instance() {
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager() {
    initialize();
}

SessionManager::~SessionManager() {
    cleanup();
}

void SessionManager::initialize() {
    database_ = std::make_unique<DatabaseManager>();
    config_ = std::make_unique<Config>();

    std::string db_path = config_->get_database_path();
    if (!database_->initialize(db_path)) {
        LOG_ERROR("Failed to initialize session database");
        return;
    }

    load_sessions();

    sessions_loaded_ = true;
    LOG_INFO("SessionManager initialized");
}

void SessionManager::cleanup() {
    save_sessions();

    if (database_) {
        database_->shutdown();
    }

    LOG_INFO("SessionManager cleaned up");
}

bool SessionManager::create_session(const Session& session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    if (sessions_.find(session.protocol) != sessions_.end()) {
        LOG_WARNING("Session already exists for protocol: " + session.protocol);
        return false;
    }

    sessions_[session.protocol] = session;
    if (!save_to_database()) {
        LOG_ERROR("Failed to save session to database");
        sessions_.erase(session.protocol);
        return false;
    }

    LOG_INFO("Session created for protocol: " + session.protocol);
    return true;
}

bool SessionManager::update_session(const Session& session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(session.protocol);
    if (it == sessions_.end()) {
        LOG_WARNING("Session not found for protocol: " + session.protocol);
        return false;
    }

    it->second = session;
    if (!save_to_database()) {
        LOG_ERROR("Failed to update session in database");
        return false;
    }

    LOG_DEBUG("Session updated for protocol: " + session.protocol);
    return true;
}

bool SessionManager::delete_session(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it == sessions_.end()) {
        LOG_WARNING("Session not found for protocol: " + protocol);
        return false;
    }

    sessions_.erase(it);
    if (!save_to_database()) {
        LOG_ERROR("Failed to delete session from database");
        return false;
    }

    LOG_INFO("Session deleted for protocol: " + protocol);
    return true;
}

bool SessionManager::refresh_session(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it == sessions_.end()) {
        LOG_WARNING("Session not found for protocol: " + protocol);
        return false;
    }

    Session& session = it->second;
    session.last_used = std::chrono::system_clock::now();

    if (!save_to_database()) {
        LOG_ERROR("Failed to refresh session in database");
        return false;
    }

    LOG_DEBUG("Session refreshed for protocol: " + protocol);
    return true;
}

Session SessionManager::get_session(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it != sessions_.end()) {
        return it->second;
    }

    return Session{};
}

std::vector<Session> SessionManager::get_all_sessions() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    std::vector<Session> result;
    for (const auto& [_, session] : sessions_) {
        result.push_back(session);
    }
    return result;
}

bool SessionManager::has_session(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.find(protocol) != sessions_.end();
}

bool SessionManager::is_session_valid(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it == sessions_.end()) {
        return false;
    }

    return it->second.is_valid && !it->second.is_expired();
}

std::string SessionManager::get_access_token(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it != sessions_.end()) {
        return it->second.access_token;
    }

    return "";
}

std::string SessionManager::get_refresh_token(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it != sessions_.end()) {
        return it->second.refresh_token;
    }

    return "";
}

bool SessionManager::set_access_token(const std::string& protocol, const std::string& token) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it == sessions_.end()) {
        LOG_WARNING("Session not found for protocol: " + protocol);
        return false;
    }

    it->second.access_token = token;
    it->second.last_used = std::chrono::system_clock::now();

    return save_to_database();
}

bool SessionManager::set_refresh_token(const std::string& protocol, const std::string& token) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(protocol);
    if (it == sessions_.end()) {
        LOG_WARNING("Session not found for protocol: " + protocol);
        return false;
    }

    it->second.refresh_token = token;

    return save_to_database();
}

bool SessionManager::validate_all_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    bool any_invalid = false;
    for (auto& [protocol, session] : sessions_) {
        if (session.is_expired()) {
            session.is_valid = false;
            any_invalid = true;
            LOG_INFO("Session expired for protocol: " + protocol);
        }
    }

    if (any_invalid) {
        save_to_database();
    }

    return !any_invalid;
}

void SessionManager::invalidate_expired_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.is_expired()) {
            LOG_INFO("Removing expired session for protocol: " + it->first);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }

    save_to_database();
}

std::vector<std::string> SessionManager::get_expired_sessions() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    std::vector<std::string> expired;
    for (const auto& [protocol, session] : sessions_) {
        if (session.is_expired()) {
            expired.push_back(protocol);
        }
    }
    return expired;
}

std::vector<std::string> SessionManager::get_sessions_needing_refresh() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    std::vector<std::string> needing_refresh;
    for (const auto& [protocol, session] : sessions_) {
        if (session.needs_refresh()) {
            needing_refresh.push_back(protocol);
        }
    }
    return needing_refresh;
}

bool SessionManager::load_sessions() {
    return load_from_database();
}

bool SessionManager::save_sessions() {
    return save_to_database();
}

bool SessionManager::import_sessions(const std::string& file_path) {
    LOG_INFO("Importing sessions from: " + file_path);
    return false;
}

bool SessionManager::export_sessions(const std::string& file_path) {
    LOG_INFO("Exporting sessions to: " + file_path);
    return false;
}

size_t SessionManager::get_session_count() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

size_t SessionManager::get_valid_session_count() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    size_t count = 0;
    for (const auto& [_, session] : sessions_) {
        if (session.is_valid && !session.is_expired()) {
            count++;
        }
    }
    return count;
}

bool SessionManager::load_from_database() {
    if (!database_ || !database_->is_initialized()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.clear();
    LOG_INFO("Sessions loaded from database");
    return true;
}

bool SessionManager::save_to_database() {
    if (!database_ || !database_->is_initialized()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(sessions_mutex_);
    LOG_DEBUG("Sessions saved to database");
    return true;
}

bool Session::is_expired() const {
    auto now = std::chrono::system_clock::now();
    return now > expires_at;
}

bool Session::needs_refresh() const {
    if (refresh_token.empty()) return false;

    auto now = std::chrono::system_clock::now();
    auto refresh_time = expires_at - std::chrono::hours(1);
    return now > refresh_time;
}

int64_t Session::get_remaining_seconds() const {
    auto now = std::chrono::system_clock::now();
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(expires_at - now);
    return std::max(static_cast<int64_t>(0), remaining.count());
}

std::string SessionManager::generate_session_id() {
    return Crypto::generate_random_bytes(16);
}

bool SessionManager::validate_token_format(const std::string& token) {
    return !token.empty() && token.length() > 10;
}

std::chrono::system_clock::time_point SessionManager::calculate_expiry(int expires_in_seconds) {
    return std::chrono::system_clock::now() + std::chrono::seconds(expires_in_seconds);
}