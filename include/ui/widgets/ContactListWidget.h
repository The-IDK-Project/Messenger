#pragma once

#include "../../core/User.h"
#include <string>
#include <vector>
#include <functional>

class ContactListWidget {
public:
    using UserClickHandler = std::function<void(const User& user)>;
    using UserContextHandler = std::function<void(const User& user, int x, int y)>;

    ContactListWidget();
    ~ContactListWidget();

    void add_user(const User& user);
    void update_user(const User& user);
    void remove_user(const std::string& user_id);
    void clear_users();

    std::vector<User> get_users() const;
    User get_user(const std::string& user_id) const;
    bool has_user(const std::string& user_id) const;

    void group_by_protocol(bool enable);
    void group_by_status(bool enable);
    void set_custom_groups(const std::vector<std::string>& groups);

    void set_filter(const std::string& filter);
    void clear_filter();
    void show_only_online(bool only_online);
    void show_only_protocol(const std::string& protocol);

    void set_visible(bool visible);
    void set_position(int x, int y);
    void set_size(int width, int height);
    void refresh();

    void set_user_click_handler(UserClickHandler handler);
    void set_user_context_handler(UserContextHandler handler);

    void set_show_avatars(bool show);
    void set_show_status(bool show);
    void set_show_protocol(bool show);
    void set_compact_mode(bool compact);
    void set_theme(const std::string& theme);

    void sort_by_name();
    void sort_by_status();
    void sort_by_last_seen();
    void set_custom_sort(std::function<bool(const User&, const User&)> comparator);

    void search(const std::string& query);
    void clear_search();

    bool is_visible() const;
    std::string get_selected_user_id() const;
    int get_visible_user_count() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};