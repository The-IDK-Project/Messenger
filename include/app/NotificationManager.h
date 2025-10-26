#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <chrono>
#include "../core/Message.h"
#include "../core/User.h"
#include "../core/ChatRoom.h"

enum class NotificationType {
    MESSAGE,
    ROOM_INVITE,
    USER_JOIN,
    USER_LEAVE,
    CONNECTION_STATUS,
    ERROR,
    SYSTEM
};

enum class NotificationPriority {
    LOW,
    NORMAL,
    HIGH,
    URGENT
};

struct Notification {
    std::string id;
    NotificationType type;
    NotificationPriority priority;
    std::string title;
    std::string message;
    std::string source_protocol;
    std::string source_room;
    std::string source_user;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::seconds timeout;
    bool is_read;
    std::map<std::string, std::string> metadata;

    Notification();
    Notification(NotificationType type,
                const std::string& title,
                const std::string& message);

    bool is_expired() const;
    std::string get_display_time() const;
};

class NotificationManager {
public:
    using NotificationCallback = std::function<void(const Notification&)>;
    using ActionCallback = std::function<void(const std::string& notification_id,
                                            const std::string& action)>;

    static NotificationManager& get_instance();

    std::string show_notification(const Notification& notification);
    bool dismiss_notification(const std::string& notification_id);
    bool dismiss_all_notifications();
    bool mark_as_read(const std::string& notification_id);
    bool mark_all_as_read();

    Notification get_notification(const std::string& notification_id) const;
    std::vector<Notification> get_all_notifications() const;
    std::vector<Notification> get_unread_notifications() const;
    std::vector<Notification> get_notifications_by_type(NotificationType type) const;
    std::vector<Notification> get_notifications_by_priority(NotificationPriority priority) const;

    void set_enabled(bool enabled);
    bool is_enabled() const;
    void set_sound_enabled(bool enabled);
    bool is_sound_enabled() const;
    void set_popup_enabled(bool enabled);
    bool is_popup_enabled() const;
    void set_max_notifications(size_t max);
    size_t get_max_notifications() const;
    void set_default_timeout(std::chrono::seconds timeout);
    std::chrono::seconds get_default_timeout() const;

    void set_desktop_notifications(bool enable);
    bool get_desktop_notifications() const;
    void set_tray_notifications(bool enable);
    bool get_tray_notifications() const;
    void set_sound_file(const std::string& sound_file);
    std::string get_sound_file() const;

    void add_filter_rule(const std::string& rule_name,
                        const std::function<bool(const Notification&)>& filter);
    void remove_filter_rule(const std::string& rule_name);
    void set_muted_rooms(const std::vector<std::string>& room_ids);
    void set_muted_users(const std::vector<std::string>& user_ids);
    void set_quiet_hours(int start_hour, int end_hour);

    void set_notification_callback(NotificationCallback callback);
    void set_action_callback(ActionCallback callback);

    size_t get_notification_count() const;
    size_t get_unread_count() const;
    size_t get_today_count() const;
    void clear_statistics();

    static std::string generate_notification_id();
    static Notification create_message_notification(const Message& message);
    static Notification create_room_invite_notification(const ChatRoom& room,
                                                       const std::string& inviter);
    static Notification create_connection_notification(const std::string& protocol,
                                                      bool connected);
    static Notification create_error_notification(const std::string& error,
                                                const std::string& source = "");

    void register_action(const std::string& notification_id,
                        const std::string& action_name,
                        const std::string& action_label);
    bool trigger_action(const std::string& notification_id,
                       const std::string& action_name);

private:
    NotificationManager();
    ~NotificationManager();
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    void initialize();
    void cleanup();
    void play_sound() const;
    void show_desktop_notification(const Notification& notification) const;
    void show_tray_notification(const Notification& notification) const;
    void cleanup_expired_notifications();

    std::vector<Notification> notifications_;
    std::map<std::string, std::map<std::string, std::string>> notification_actions_;
    std::map<std::string, std::function<bool(const Notification&)>> filter_rules_;

    NotificationCallback notification_callback_;
    ActionCallback action_callback_;

    bool enabled_ = true;
    bool sound_enabled_ = true;
    bool popup_enabled_ = true;
    bool desktop_notifications_ = true;
    bool tray_notifications_ = true;
    size_t max_notifications_ = 100;
    std::chrono::seconds default_timeout_{30};
    std::string sound_file_;

    std::vector<std::string> muted_rooms_;
    std::vector<std::string> muted_users_;
    int quiet_start_hour_ = 0;
    int quiet_end_hour_ = 0;

    mutable std::mutex notifications_mutex_;
};