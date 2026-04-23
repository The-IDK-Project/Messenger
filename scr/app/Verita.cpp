#include "app/Verita.h"
#include "protocols/ProtocolFactory.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <format>

// OpenSSL headers
#include <openssl/ssl.h>
#include <openssl/err.h>

UnifiedMessenger::Verita() {
    // Singletons are managed internally
}

UnifiedMessenger::~Verita() {
    shutdown();
}

bool Verita1::initialize() {
    if (initialized_) {
        LOG_WARNING("UnifiedMessenger already initialized");
        return true;
    }

    LOG_INFO("Initializing Unified Messenger...");

    // Initialize OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    LOG_INFO("OpenSSL initialized.");

    auto& config = Config::get_instance();
    auto& database = DatabaseManager::get_instance();

    if (!config.load_from_file()) {
        LOG_WARNING("Failed to load configuration, using defaults");
    }

    std::string db_path = config.get_database_path();
    if (!database.initialize(db_path)) {
        LOG_ERROR("Failed to initialize database");
        return false;
    }

    initialize_protocols();

    sync_thread_ = std::jthread(&UnifiedMessenger::sync_worker, this);
    notification_thread_ = std::jthread(&UnifiedMessenger::notification_worker, this);

    initialized_ = true;
    LOG_INFO("Unified Messenger initialized successfully");
    return true;
}

void Verita::shutdown() {
    if (!initialized_) return;

    LOG_INFO("Shutting down Unified Messenger...");

    sync_thread_.request_stop();
    notification_thread_.request_stop();

    sync_thread_.join();
    notification_thread_.join();

    disconnect_all();

    DatabaseManager::get_instance().shutdown();

    // Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();
    LOG_INFO("OpenSSL cleaned up.");

    initialized_ = false;
    LOG_INFO("Unified Messenger shutdown complete");
}

bool UnifiedMessenger::add_protocol(const std::string& name, std::unique_ptr<ProtocolHandler> handler) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (protocols_.contains(name)) {
        LOG_WARNING(std::format("Protocol already exists: {}", name));
        return false;
    }

    protocols_[name] = std::move(handler);
    setup_protocol_callbacks(name);

    LOG_INFO(std::format("Protocol added: {}", name));
    return true;
}

bool UnifiedMessenger::remove_protocol(const std::string& name) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = protocols_.find(name);
    if (it == protocols_.end()) {
        LOG_WARNING(std::format("Protocol not found: {}", name));
        return false;
    }

    if (it->second->is_connected()) {
        it->second->disconnect();
    }

    protocols_.erase(it);
    LOG_INFO(std::format("Protocol removed: {}", name));
    return true;
}

bool UnifiedMessenger::connect_protocol(const std::string& name) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = protocols_.find(name);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found: {}", name));
        return false;
    }

    if (it->second->is_connected()) {
        LOG_WARNING(std::format("Protocol already connected: {}", name));
        return true;
    }

    bool result = it->second->connect();
    if (result) {
        LOG_INFO(std::format("Protocol connected: {}", name));
        if (status_callback_) {
            status_callback_(name, true);
        }
    } else {
        LOG_ERROR(std::format("Failed to connect protocol: {}", name));
        if (error_callback_) {
            error_callback_(std::format("Failed to connect to {}", name));
        }
    }

    return result;
}

bool UnifiedMessenger::disconnect_protocol(const std::string& name) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = protocols_.find(name);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found: {}", name));
        return false;
    }

    if (!it->second->is_connected()) {
        LOG_WARNING(std::format("Protocol not connected: {}", name));
        return true;
    }

    it->second->disconnect();
    LOG_INFO(std::format("Protocol disconnected: {}", name));

    if (status_callback_) {
        status_callback_(name, false);
    }

    return true;
}

void UnifiedMessenger::connect_all() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    bool auto_connect = Config::get_instance().get_auto_connect();
    if (!auto_connect) {
        LOG_INFO("Auto-connect disabled in configuration");
        return;
    }

    for (auto& [name, handler] : protocols_) {
        if (!handler->is_connected()) {
            LOG_INFO(std::format("Auto-connecting to: {}", name));
            handler->connect();
        }
    }
}

void UnifiedMessenger::disconnect_all() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    for (auto& [name, handler] : protocols_) {
        if (handler->is_connected()) {
            handler->disconnect();
        }
    }
}

std::vector<std::string> UnifiedMessenger::get_available_protocols() const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::vector<std::string> result;
    for (const auto& [name, _] : protocols_) {
        result.push_back(name);
    }
    return result;
}

std::vector<std::string> UnifiedMessenger::get_connected_protocols() const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::vector<std::string> result;
    for (const auto& [name, handler] : protocols_) {
        if (handler->is_connected()) {
            result.push_back(name);
        }
    }
    return result;
}

bool UnifiedMessenger::send_message(const std::string& protocol,
                                   const std::string& room_id,
                                   const std::string& message) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found: {}", protocol));
        return false;
    }

    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected: {}", protocol));
        return false;
    }

    bool result = it->second->send_message(room_id, message);
    if (result) {
        Message local_msg(message, "current_user", "You", protocol, room_id);
        local_msg.status = MessageStatus::SENDING;
        DatabaseManager::get_instance().store_message(local_msg);

        if (message_callback_) {
            message_callback_(local_msg);
        }
    }

    return result;
}

std::vector<ChatRoom> UnifiedMessenger::get_all_rooms() const {
    return DatabaseManager::get_instance().get_recent_rooms(100);
}

std::vector<ChatRoom> UnifiedMessenger::get_rooms_by_protocol(const std::string& protocol) const {
    return DatabaseManager::get_instance().get_rooms_by_protocol(protocol);
}

bool UnifiedMessenger::join_room(const std::string& protocol, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found: {}", protocol));
        return false;
    }

    return it->second->join_room(room_id);
}

std::vector<Message> UnifiedMessenger::get_unified_inbox(int limit) const {
    if (!active_room_id_.empty()) {
        return DatabaseManager::get_instance().get_messages(active_room_id_, limit);
    }
    return {};
}

std::vector<Message> UnifiedMessenger::get_room_messages(const std::string& room_id, int limit) const {
    return DatabaseManager::get_instance().get_messages(room_id, limit);
}

void UnifiedMessenger::sync_all() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    for (auto& [name, handler] : protocols_) {
        if (handler->is_connected() && handler->supports_sync()) {
            handler->sync();
        }
    }
}

void UnifiedMessenger::set_message_callback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void UnifiedMessenger::set_room_callback(RoomCallback callback) {
    room_callback_ = std::move(callback);
}

void UnifiedMessenger::set_status_callback(StatusCallback callback) {
    status_callback_ = std::move(callback);
}

void UnifiedMessenger::set_error_callback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
}

bool UnifiedMessenger::load_config(const std::string& config_path) {
    return Config::get_instance().load_from_file(config_path);
}

bool UnifiedMessenger::save_config(const std::string& config_path) const {
    return Config::get_instance().save_to_file(config_path);
}

void UnifiedMessenger::initialize_protocols() {
    ProtocolFactory& factory = ProtocolFactory::get_instance();
    factory.register_default_protocols();

    auto available_protocols = factory.get_available_protocols();
    auto& config = Config::get_instance();

    for (const auto& protocol_name : available_protocols) {
        auto handler = factory.create_protocol(protocol_name);
        if (handler) {
            std::string server = config.get_protocol_config(protocol_name, "server", "");
            std::string username = config.get_protocol_config(protocol_name, "username", "");
            std::string password = config.get_protocol_config(protocol_name, "password", "");
            handler->set_config("server", server);
            handler->set_config("username", username);
            handler->set_config("password", password);

            protocols_[protocol_name] = std::move(handler);
            LOG_INFO(std::format("Initialized protocol: {}", protocol_name));
        }
    }
}

void UnifiedMessenger::setup_protocol_callbacks(const std::string& protocol_name) {
    auto& handler = protocols_[protocol_name];

    handler->set_message_callback([this, protocol_name](const Message& message) {
        handle_protocol_message(protocol_name, message);
    });

    handler->set_room_callback([this, protocol_name](const ChatRoom& room) {
        handle_protocol_room(protocol_name, room);
    });

    handler->set_user_callback([this, protocol_name](const User& user) {
        handle_protocol_user(protocol_name, user);
    });

    handler->set_error_callback([this, protocol_name](const std::string& error) {
        handle_protocol_error(protocol_name, error);
    });
}

void UnifiedMessenger::handle_protocol_message(const std::string& protocol, const Message& message) {
    DatabaseManager::get_instance().store_message(message);
    if (message_callback_) {
        message_callback_(message);
    }

    LOG_DEBUG(std::format("Message received from {}: {}...", protocol, message.content.substr(0, 50)));
}

void UnifiedMessenger::handle_protocol_room(const std::string& protocol, const ChatRoom& room) {
    DatabaseManager::get_instance().store_room(room);
    if (room_callback_) {
        room_callback_(room);
    }

    LOG_DEBUG(std::format("Room update from {}: {}", protocol, room.name));
}

void UnifiedMessenger::handle_protocol_user(const std::string& protocol, const User& user) {
    DatabaseManager::get_instance().store_user(user);
    if (user_callback_) {
        user_callback_(user);
    }

    LOG_DEBUG(std::format("User update from {}: {}", protocol, user.get_best_name()));
}

void UnifiedMessenger::handle_protocol_error(const std::string& protocol, const std::string& error) {
    LOG_ERROR(std::format("Protocol error ({}): {}", protocol, error));

    if (error_callback_) {
        error_callback_(std::format("[{}] {}", protocol, error));
    }
}

void UnifiedMessenger::sync_worker(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(30));

        if (stop_token.stop_requested()) break;

        try {
            sync_all();
        } catch (const std::exception& e) {
            LOG_ERROR(std::format("Sync worker error: {}", e.what()));
        }
    }
}

void UnifiedMessenger::notification_worker(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (stop_token.stop_requested()) break;
        // Logic for notifications would go here
    }
}

std::string UnifiedMessenger::get_version() const {
    return "1.0.0";
}

std::vector<std::string> UnifiedMessenger::get_supported_protocols() const {
    return {"matrix", "irc"};
}

bool UnifiedMessenger::is_ready() const {
    return initialized_ && DatabaseManager::get_instance().is_initialized();
}

// Screen Share API
bool UnifiedMessenger::start_screen_share(const std::string& protocol, const std::string& call_id, int screen_id) {
    LOG_INFO(std::format("Starting screen share for call {} on screen {}", call_id, screen_id));
    // Platform-specific implementation needed
    return false;
}

bool UnifiedMessenger::stop_screen_share(const std::string& protocol, const std::string& call_id) {
    LOG_INFO(std::format("Stopping screen share for call {}", call_id));
    // Platform-specific implementation needed
    return false;
}

std::vector<ScreenCapturer::Screen> UnifiedMessenger::get_available_screens() {
    return ScreenCapturer::get_screens();
}


// Missing implementations

bool UnifiedMessenger::is_protocol_connected(const std::string& name) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(name);
    if (it != protocols_.end()) {
        return it->second->is_connected();
    }
    return false;
}

bool UnifiedMessenger::send_file(const std::string& protocol, const std::string& room_id, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found for sending file: {}", protocol));
        return false;
    }
    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected for sending file: {}", protocol));
        return false;
    }
    return it->second->send_file(room_id, file_path);
}

bool UnifiedMessenger::send_voice_message(const std::string& protocol, const std::string& room_id, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found for sending voice message: {}", protocol));
        return false;
    }
    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected for sending voice message: {}", protocol));
        return false;
    }
    // Assuming ProtocolHandler has send_voice_message
    return it->second->send_voice_message(room_id, file_path);
}

bool UnifiedMessenger::send_video_message(const std::string& protocol, const std::string& room_id, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found for sending video message: {}", protocol));
        return false;
    }
    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected for sending video message: {}", protocol));
        return false;
    }
    // Assuming ProtocolHandler has send_video_message
    return it->second->send_video_message(room_id, file_path);
}

bool UnifiedMessenger::mark_message_read(const std::string& protocol, const std::string& room_id, const std::string& message_id) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found for marking message as read: {}", protocol));
        return false;
    }
    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected for marking message as read: {}", protocol));
        return false;
    }

    // Notify the protocol handler to send the read receipt
    it->second->mark_message_read(room_id, message_id);

    // Update the local database
    return DatabaseManager::get_instance().update_message_status(message_id, MessageStatus::READ);
}

bool UnifiedMessenger::leave_room(const std::string& protocol, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found for leaving room: {}", protocol));
        return false;
    }
    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected for leaving room: {}", protocol));
        return false;
    }
    return it->second->leave_room(room_id);
}

bool UnifiedMessenger::create_room(const std::string& protocol, const std::string& name, const std::vector<std::string>& users) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it == protocols_.end()) {
        LOG_ERROR(std::format("Protocol not found for creating room: {}", protocol));
        return false;
    }
    if (!it->second->is_connected()) {
        LOG_ERROR(std::format("Protocol not connected for creating room: {}", protocol));
        return false;
    }
    return it->second->create_room(name, users);
}

void UnifiedMessenger::set_active_room(const std::string& room_id) {
    active_room_id_ = room_id;
}

User UnifiedMessenger::get_current_user(const std::string& protocol) const {
    std::string user_id = DatabaseManager::get_instance().get_session_user_id(protocol);
    if (user_id.empty()) {
        LOG_WARNING(std::format("Could not find current user ID for protocol: {}", protocol));
        return User();
    }
    return DatabaseManager::get_instance().get_user(user_id);
}

std::vector<User> UnifiedMessenger::get_room_users(const std::string& room_id) const {
    return DatabaseManager::get_instance().get_users_by_room(room_id);
}

std::vector<User> UnifiedMessenger::search_users(const std::string& query) const {
    return DatabaseManager::get_instance().search_users(query);
}

std::vector<Message> UnifiedMessenger::search_messages(const std::string& query, int limit) const {
    return DatabaseManager::get_instance().search_messages(query, "", limit);
}

void UnifiedMessenger::request_sync(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = protocols_.find(protocol);
    if (it != protocols_.end() && it->second->is_connected()) {
        it->second->sync();
    }
}

void UnifiedMessenger::set_user_callback(UserCallback callback) {
    user_callback_ = std::move(callback);
}

void UnifiedMessenger::set_setting(const std::string& key, const std::string& value) {
    // DatabaseManager has store_setting, assume it works
    DatabaseManager::get_instance().store_setting(key, value);
}

std::string UnifiedMessenger::get_setting(const std::string& key, const std::string& default_value) const {
    // DatabaseManager has get_setting
    // Note: DatabaseManager::get_instance() is not const-correct if used in const method unless it returns reference to non-const or mutable?
    // Actually DatabaseManager::get_instance() returns DatabaseManager&.
    // get_setting on DatabaseManager is probably not const?
    // "std::string get_setting(const std::string& key, const std::string& default_value = "");" - no const.
    // This is a problem for "get_setting(...) const" in UnifiedMessenger.
    // I will use const_cast for the singleton instance since it's a singleton.
    return const_cast<DatabaseManager&>(DatabaseManager::get_instance()).get_setting(key, default_value);
}

void UnifiedMessenger::enable_notifications(bool enable) {
    Config::get_instance().set_notifications_enabled(enable);
}

void UnifiedMessenger::set_notification_sound(bool enable) {
    // Config doesn't have this yet, maybe add to Config or store in DB
    set_setting("notification_sound", enable ? "true" : "false");
}

void UnifiedMessenger::set_notification_popup(bool enable) {
    set_setting("notification_popup", enable ? "true" : "false");
}

bool UnifiedMessenger::backup_database(const std::string& backup_path) {
    return DatabaseManager::get_instance().backup_database(backup_path);
}

bool UnifiedMessenger::vacuum_database() {
    return DatabaseManager::get_instance().vacuum_database();
}

int UnifiedMessenger::get_database_size() const {
    return const_cast<DatabaseManager&>(DatabaseManager::get_instance()).get_database_size();
}

void UnifiedMessenger::initialize_database() {
    // This is called in initialize(), implemented there.
}