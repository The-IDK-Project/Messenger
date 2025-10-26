#include "ui/widgets/ContactListWidget.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <algorithm>
#include <map>

class ContactListWidget::Impl {
public:
    std::vector<User> users;
    std::vector<User> filtered_users;
    std::string filter_text;
    bool group_by_protocol = false;
    bool group_by_status = true;
    bool show_only_online = false;
    std::string show_only_protocol;
    bool show_avatars = true;
    bool show_status = true;
    bool show_protocol = false;
    bool compact_mode = false;
    bool visible = true;
    int x = 0, y = 0, width = 200, height = 400;
    std::string selected_user_id;

    UserClickHandler user_click_handler;
    UserContextHandler user_context_handler;

    std::function<bool(const User&, const User&)> sort_comparator = [](const User& a, const User& b) {
        if (a.is_online() != b.is_online()) {
            return a.is_online() > b.is_online();
        }
        return StringUtils::compare_ignore_case(a.get_best_name(), b.get_best_name()) < 0;
    };

    void apply_filter() {
        filtered_users.clear();

        for (const auto& user : users) {
            bool matches = true;

            if (!filter_text.empty()) {
                if (!StringUtils::contains_ignore_case(user.get_best_name(), filter_text) &&
                    !StringUtils::contains_ignore_case(user.username, filter_text)) {
                    matches = false;
                }
            }

            if (show_only_online && !user.is_online()) {
                matches = false;
            }

            if (!show_only_protocol.empty() && !user.supports_protocol(show_only_protocol)) {
                matches = false;
            }

            if (matches) {
                filtered_users.push_back(user);
            }
        }

        std::sort(filtered_users.begin(), filtered_users.end(), sort_comparator);
    }

    std::vector<std::pair<std::string, std::vector<User>>> get_grouped_users() const {
        std::map<std::string, std::vector<User>> groups;

        if (group_by_status) {
            for (const auto& user : filtered_users) {
                std::string group = user.is_online() ? "Online" : "Offline";
                groups[group].push_back(user);
            }
        } else if (group_by_protocol) {
            for (const auto& user : filtered_users) {
                for (const auto& protocol : user.protocols) {
                    groups[protocol].push_back(user);
                }
                if (user.protocols.empty()) {
                    groups["Unknown"].push_back(user);
                }
            }
        } else {
            groups["Contacts"] = filtered_users;
        }

        std::vector<std::pair<std::string, std::vector<User>>> result;
        for (auto& [group_name, group_users] : groups) {
            result.emplace_back(group_name, std::move(group_users));
        }

        return result;
    }
};

ContactListWidget::ContactListWidget() : impl_(std::make_unique<Impl>()) {}
ContactListWidget::~ContactListWidget() = default;

void ContactListWidget::add_user(const User& user) {
    impl_->users.push_back(user);
    impl_->apply_filter();
}

void ContactListWidget::update_user(const User& user) {
    for (auto& u : impl_->users) {
        if (u.id == user.id) {
            u = user;
            break;
        }
    }
    impl_->apply_filter();
}

void ContactListWidget::remove_user(const std::string& user_id) {
    impl_->users.erase(
        std::remove_if(impl_->users.begin(), impl_->users.end(),
                      [&](const User& user) { return user.id == user_id; }),
        impl_->users.end()
    );
    impl_->apply_filter();

    if (impl_->selected_user_id == user_id) {
        impl_->selected_user_id.clear();
    }
}

void ContactListWidget::clear_users() {
    impl_->users.clear();
    impl_->filtered_users.clear();
    impl_->selected_user_id.clear();
}

std::vector<User> ContactListWidget::get_users() const {
    return impl_->users;
}

User ContactListWidget::get_user(const std::string& user_id) const {
    for (const auto& user : impl_->users) {
        if (user.id == user_id) {
            return user;
        }
    }
    return User();
}

bool ContactListWidget::has_user(const std::string& user_id) const {
    return std::any_of(impl_->users.begin(), impl_->users.end(),
                      [&](const User& user) { return user.id == user_id; });
}

void ContactListWidget::group_by_protocol(bool enable) {
    impl_->group_by_protocol = enable;
    if (enable) impl_->group_by_status = false;
}

void ContactListWidget::group_by_status(bool enable) {
    impl_->group_by_status = enable;
    if (enable) impl_->group_by_protocol = false;
}

void ContactListWidget::set_custom_groups(const std::vector<std::string>& groups) {
    LOG_DEBUG("Custom groups set with " + std::to_string(groups.size()) + " groups");
}

void ContactListWidget::set_filter(const std::string& filter) {
    impl_->filter_text = filter;
    impl_->apply_filter();
}

void ContactListWidget::clear_filter() {
    impl_->filter_text.clear();
    impl_->show_only_online = false;
    impl_->show_only_protocol.clear();
    impl_->apply_filter();
}

void ContactListWidget::show_only_online(bool only_online) {
    impl_->show_only_online = only_online;
    impl_->apply_filter();
}

void ContactListWidget::show_only_protocol(const std::string& protocol) {
    impl_->show_only_protocol = protocol;
    impl_->apply_filter();
}

void ContactListWidget::set_visible(bool visible) {
    impl_->visible = visible;
}

void ContactListWidget::set_position(int x, int y) {
    impl_->x = x;
    impl_->y = y;
}

void ContactListWidget::set_size(int width, int height) {
    impl_->width = width;
    impl_->height = height;
}

void ContactListWidget::refresh() {
    LOG_DEBUG("ContactListWidget refreshed");
}

void ContactListWidget::set_user_click_handler(UserClickHandler handler) {
    impl_->user_click_handler = std::move(handler);
}

void ContactListWidget::set_user_context_handler(UserContextHandler handler) {
    impl_->user_context_handler = std::move(handler);
}

void ContactListWidget::set_show_avatars(bool show) {
    impl_->show_avatars = show;
}

void ContactListWidget::set_show_status(bool show) {
    impl_->show_status = show;
}

void ContactListWidget::set_show_protocol(bool show) {
    impl_->show_protocol = show;
}

void ContactListWidget::set_compact_mode(bool compact) {
    impl_->compact_mode = compact;
}

void ContactListWidget::set_theme(const std::string& theme) {
    LOG_INFO("ContactListWidget theme set to: " + theme);
}

void ContactListWidget::sort_by_name() {
    impl_->sort_comparator = [](const User& a, const User& b) {
        return StringUtils::compare_ignore_case(a.get_best_name(), b.get_best_name()) < 0;
    };
    impl_->apply_filter();
}

void ContactListWidget::sort_by_status() {
    impl_->sort_comparator = [](const User& a, const User& b) {
        if (a.is_online() != b.is_online()) {
            return a.is_online() > b.is_online();
        }
        return StringUtils::compare_ignore_case(a.get_best_name(), b.get_best_name()) < 0;
    };
    impl_->apply_filter();
}

void ContactListWidget::sort_by_last_seen() {
    impl_->sort_comparator = [](const User& a, const User& b) {
        return a.last_seen > b.last_seen;
    };
    impl_->apply_filter();
}

void ContactListWidget::set_custom_sort(std::function<bool(const User&, const User&)> comparator) {
    impl_->sort_comparator = std::move(comparator);
    impl_->apply_filter();
}

void ContactListWidget::search(const std::string& query) {
    set_filter(query);
}

void ContactListWidget::clear_search() {
    clear_filter();
}

bool ContactListWidget::is_visible() const {
    return impl_->visible;
}

std::string ContactListWidget::get_selected_user_id() const {
    return impl_->selected_user_id;
}

int ContactListWidget::get_visible_user_count() const {
    return static_cast<int>(impl_->filtered_users.size());
}