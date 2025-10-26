#include "app/NotificationManager.h"
#include "utils/Logger.h"
#include "utils/Crypto.h"
#include <algorithm>

NotificationManager& NotificationManager::get_instance() {
    static NotificationManager instance;
    return instance;
}

NotificationManager::NotificationManager() {
    initialize();
}

NotificationManager::~NotificationManager() {
    cleanup();
}

void NotificationManager::initialize() {
    LOG_INFO("NotificationManager initialized");
}

void NotificationManager::cleanup() {
    {
        std::lock_guard<std::mutex> lock(notifications_mutex_);
        notifications_.clear();
        notification_actions_.clear();
    }

    LOG_INFO("NotificationManager cleaned up");
}

std::string NotificationManager::show_notification(const Notification& notification) {
    if (!enabled_) {
        return "";
    }

    for (const auto& [_, filter] : filter_rules_) {
        if (!filter(notification)) {
            return "";
        }
    }

    if (std::find(muted_rooms_.begin(), muted_rooms_.end(), notification.source_room) != muted_rooms_.end()) {
        return "";
    }

    if (std::find(muted_users_.begin(), muted_users_.end(), notification.source_user) != muted_users_.end()) {
        return "";
    }

    if (is_quiet_hours()) {
        return "";
    }

    Notification notif = notification;
    notif.id = generate_notification_id();
    notif.timestamp = std::chrono::system_clock::now();

    if (notif.timeout.count() == 0) {
        notif.timeout = default_timeout_;
    }

    {
        std::lock_guard<std::mutex> lock(notifications_mutex_);

        if (notifications_.size() >= max_notifications_) {
            notifications_.erase(notifications_.begin());
        }

        notifications_.push_back(notif);
    }

    if (sound_enabled_ && !sound_file_.empty()) {
        play_sound();
    }

    if (desktop_notifications_) {
        show_desktop_notification(notif);
    }

    if (tray_notifications_) {
        show_tray_notification(notif);
    }

    if (notification_callback_) {
        notification_callback_(notif);
    }

    LOG_DEBUG("Notification shown: " + notif.title + " - " + notif.message);
    return notif.id;
}

bool NotificationManager::dismiss_notification(const std::string& notification_id) {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    auto it = std::find_if(notifications_.begin(), notifications_.end(),
                          [&](const Notification& n) { return n.id == notification_id; });

    if (it != notifications_.end()) {
        notifications_.erase(it);
        notification_actions_.erase(notification_id);
        LOG_DEBUG("Notification dismissed: " + notification_id);
        return true;
    }

    return false;
}

bool NotificationManager::dismiss_all_notifications() {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    size_t count = notifications_.size();
    notifications_.clear();
    notification_actions_.clear();

    LOG_INFO("All notifications dismissed (" + std::to_string(count) + " notifications)");
    return count > 0;
}

bool NotificationManager::mark_as_read(const std::string& notification_id) {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    auto it = std::find_if(notifications_.begin(), notifications_.end(),
                          [&](const Notification& n) { return n.id == notification_id; });

    if (it != notifications_.end()) {
        it->is_read = true;
        LOG_DEBUG("Notification marked as read: " + notification_id);
        return true;
    }

    return false;
}

bool NotificationManager::mark_all_as_read() {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    size_t count = 0;
    for (auto& notification : notifications_) {
        if (!notification.is_read) {
            notification.is_read = true;
            count++;
        }
    }

    LOG_INFO("All notifications marked as read (" + std::to_string(count) + " notifications)");
    return count > 0;
}

Notification NotificationManager::get_notification(const std::string& notification_id) const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    auto it = std::find_if(notifications_.begin(), notifications_.end(),
                          [&](const Notification& n) { return n.id == notification_id; });

    if (it != notifications_.end()) {
        return *it;
    }

    return Notification{};
}

std::vector<Notification> NotificationManager::get_all_notifications() const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    return notifications_;
}

std::vector<Notification> NotificationManager::get_unread_notifications() const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    std::vector<Notification> unread;
    std::copy_if(notifications_.begin(), notifications_.end(), std::back_inserter(unread),
                [](const Notification& n) { return !n.is_read; });
    return unread;
}

std::vector<Notification> NotificationManager::get_notifications_by_type(NotificationType type) const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    std::vector<Notification> result;
    std::copy_if(notifications_.begin(), notifications_.end(), std::back_inserter(result),
                [type](const Notification& n) { return n.type == type; });
    return result;
}

std::vector<Notification> NotificationManager::get_notifications_by_priority(NotificationPriority priority) const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    std::vector<Notification> result;
    std::copy_if(notifications_.begin(), notifications_.end(), std::back_inserter(result),
                [priority](const Notification& n) { return n.priority == priority; });
    return result;
}

void NotificationManager::set_enabled(bool enabled) {
    enabled_ = enabled;
    LOG_INFO("Notifications " + std::string(enabled ? "enabled" : "disabled"));
}

void NotificationManager::set_sound_enabled(bool enabled) {
    sound_enabled_ = enabled;
    LOG_INFO("Notification sound " + std::string(enabled ? "enabled" : "disabled"));
}

void NotificationManager::set_popup_enabled(bool enabled) {
    popup_enabled_ = enabled;
    LOG_INFO("Notification popups " + std::string(enabled ? "enabled" : "disabled"));
}

void NotificationManager::set_max_notifications(size_t max) {
    max_notifications_ = max;
    LOG_INFO("Max notifications set to: " + std::to_string(max));
}

void NotificationManager::set_default_timeout(std::chrono::seconds timeout) {
    default_timeout_ = timeout;
    LOG_INFO("Default notification timeout set to: " + std::to_string(timeout.count()) + " seconds");
}

void NotificationManager::set_desktop_notifications(bool enable) {
    desktop_notifications_ = enable;
    LOG_INFO("Desktop notifications " + std::string(enable ? "enabled" : "disabled"));
}

void NotificationManager::set_tray_notifications(bool enable) {
    tray_notifications_ = enable;
    LOG_INFO("Tray notifications " + std::string(enable ? "enabled" : "disabled"));
}

void NotificationManager::set_sound_file(const std::string& sound_file) {
    sound_file_ = sound_file;
    LOG_INFO("Notification sound file set to: " + sound_file);
}

void NotificationManager::add_filter_rule(const std::string& rule_name,
                                         const std::function<bool(const Notification&)>& filter) {
    filter_rules_[rule_name] = filter;
    LOG_INFO("Filter rule added: " + rule_name);
}

void NotificationManager::remove_filter_rule(const std::string& rule_name) {
    filter_rules_.erase(rule_name);
    LOG_INFO("Filter rule removed: " + rule_name);
}

void NotificationManager::set_muted_rooms(const std::vector<std::string>& room_ids) {
    muted_rooms_ = room_ids;
    LOG_INFO("Muted rooms updated: " + std::to_string(room_ids.size()) + " rooms");
}

void NotificationManager::set_muted_users(const std::vector<std::string>& user_ids) {
    muted_users_ = user_ids;
    LOG_INFO("Muted users updated: " + std::to_string(user_ids.size()) + " users");
}

void NotificationManager::set_quiet_hours(int start_hour, int end_hour) {
    quiet_start_hour_ = start_hour;
    quiet_end_hour_ = end_hour;
    LOG_INFO("Quiet hours set: " + std::to_string(start_hour) + ":00 - " + std::to_string(end_hour) + ":00");
}

void NotificationManager::set_notification_callback(NotificationCallback callback) {
    notification_callback_ = std::move(callback);
}

void NotificationManager::set_action_callback(ActionCallback callback) {
    action_callback_ = std::move(callback);
}

size_t NotificationManager::get_notification_count() const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    return notifications_.size();
}

size_t NotificationManager::get_unread_count() const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    return std::count_if(notifications_.begin(), notifications_.end(),
                        [](const Notification& n) { return !n.is_read; });
}

size_t NotificationManager::get_today_count() const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    auto today = std::chrono::system_clock::now();
    auto today_start = std::chrono::floor<std::chrono::days>(today);

    return std::count_if(notifications_.begin(), notifications_.end(),
                        [&](const Notification& n) {
                            return n.timestamp >= today_start;
                        });
}

void NotificationManager::clear_statistics() {
    LOG_INFO("Notification statistics cleared");
}

void NotificationManager::register_action(const std::string& notification_id,
                                        const std::string& action_name,
                                        const std::string& action_label) {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    notification_actions_[notification_id][action_name] = action_label;
    LOG_DEBUG("Action registered: " + action_name + " for notification " + notification_id);
}

bool NotificationManager::trigger_action(const std::string& notification_id,
                                       const std::string& action_name) {
    if (action_callback_) {
        action_callback_(notification_id, action_name);
        LOG_DEBUG("Action triggered: " + action_name + " for notification " + notification_id);
        return true;
    }

    return false;
}

void NotificationManager::play_sound() const {
    LOG_DEBUG("Playing notification sound: " + sound_file_);
}

void NotificationManager::show_desktop_notification(const Notification& notification) const {
    LOG_DEBUG("Showing desktop notification: " + notification.title);
}

void NotificationManager::show_tray_notification(const Notification& notification) const {
    LOG_DEBUG("Showing tray notification: " + notification.title);
}

void NotificationManager::cleanup_expired_notifications() {
    std::lock_guard<std::mutex> lock(notifications_mutex_);

    auto now = std::chrono::system_clock::now();
    auto it = std::remove_if(notifications_.begin(), notifications_.end(),
                            [&](const Notification& n) {
                                return n.is_expired();
                            });

    if (it != notifications_.end()) {
        notifications_.erase(it, notifications_.end());
    }
}

bool NotificationManager::is_quiet_hours() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    int current_hour = tm.tm_hour;

    if (quiet_start_hour_ <= quiet_end_hour_) {
        return current_hour >= quiet_start_hour_ && current_hour < quiet_end_hour_;
    } else {
        return current_hour >= quiet_start_hour_ || current_hour < quiet_end_hour_;
    }
}
Notification::Notification()
    : type(NotificationType::MESSAGE)
    , priority(NotificationPriority::NORMAL)
    , timeout(0)
    , is_read(false) {
}

Notification::Notification(NotificationType type, const std::string& title, const std::string& message)
    : type(type)
    , priority(NotificationPriority::NORMAL)
    , title(title)
    , message(message)
    , timeout(0)
    , is_read(false) {
}

bool Notification::is_expired() const {
    if (timeout.count() == 0) return false;

    auto now = std::chrono::system_clock::now();
    return now > timestamp + timeout;
}

std::string Notification::get_display_time() const {
    auto now = std::chrono::system_clock::now();
    auto diff = now - timestamp;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count();

    if (minutes < 1) return "Just now";
    if (minutes < 60) return std::to_string(minutes) + "m ago";

    auto hours = minutes / 60;
    if (hours < 24) return std::to_string(hours) + "h ago";

    auto days = hours / 24;
    return std::to_string(days) + "d ago";
}
std::string NotificationManager::generate_notification_id() {
    return "notif_" + Crypto::generate_random_bytes(8);
}

Notification NotificationManager::create_message_notification(const Message& message) {
    Notification notif(NotificationType::MESSAGE,
                      "New message from " + message.sender_name,
                      message.content);

    notif.source_protocol = message.protocol;
    notif.source_room = message.room_id;
    notif.source_user = message.sender_id;
    notif.priority = message.type == MessageType::SYSTEM ?
                    NotificationPriority::LOW : NotificationPriority::NORMAL;

    return notif;
}

Notification NotificationManager::create_room_invite_notification(const ChatRoom& room,
                                                                 const std::string& inviter) {
    Notification notif(NotificationType::ROOM_INVITE,
                      "Room invitation",
                      "You've been invited to " + room.name + " by " + inviter);

    notif.source_protocol = room.protocol;
    notif.source_room = room.id;
    notif.source_user = inviter;
    notif.priority = NotificationPriority::HIGH;

    return notif;
}

Notification NotificationManager::create_connection_notification(const std::string& protocol,
                                                                bool connected) {
    std::string status = connected ? "connected" : "disconnected";
    Notification notif(NotificationType::CONNECTION_STATUS,
                      "Connection " + status,
                      protocol + " is now " + status);

    notif.source_protocol = protocol;
    notif.priority = connected ? NotificationPriority::LOW : NotificationPriority::HIGH;

    return notif;
}

Notification NotificationManager::create_error_notification(const std::string& error,
                                                           const std::string& source) {
    Notification notif(NotificationType::ERROR,
                      "Error" + (source.empty() ? "" : " in " + source),
                      error);

    notif.priority = NotificationPriority::HIGH;
    return notif;
}