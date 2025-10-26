#include "ui/widgets/ChatWidget.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <algorithm>
#include <sstream>

class ChatWidget::Impl {
public:
    std::vector<Message> messages;
    std::vector<Message> filtered_messages;
    std::string filter_text;
    std::string show_only_user_id;
    MessageType show_only_type = MessageType::TEXT;
    bool show_timestamps = true;
    bool show_avatars = false;
    bool compact_mode = false;
    size_t max_messages = 1000;
    int scroll_position = 0;
    std::string selected_message_id;
    bool visible = true;
    int x = 0, y = 0, width = 80, height = 24;

    MessageClickHandler message_click_handler;
    ScrollHandler scroll_handler;

    void apply_filter() {
        filtered_messages.clear();

        for (const auto& msg : messages) {
            bool matches = true;

            if (!filter_text.empty()) {
                if (!StringUtils::contains_ignore_case(msg.content, filter_text) &&
                    !StringUtils::contains_ignore_case(msg.sender_name, filter_text)) {
                    matches = false;
                }
            }

            if (!show_only_user_id.empty() && msg.sender_id != show_only_user_id) {
                matches = false;
            }
            if (show_only_type != MessageType::TEXT && msg.type != show_only_type) {
                matches = false;
            }

            if (matches) {
                filtered_messages.push_back(msg);
            }
        }
        if (filtered_messages.size() > max_messages) {
            filtered_messages.erase(filtered_messages.begin(),
                                   filtered_messages.begin() + (filtered_messages.size() - max_messages));
        }
    }

    std::vector<std::string> format_message(const Message& msg) const {
        std::vector<std::string> lines;
        std::string prefix;

        if (show_timestamps) {
            prefix = "[" + msg.get_display_time() + "] ";
        }

        if (show_avatars && !compact_mode) {
            prefix += msg.sender_name + " ";
        }

        std::string content = prefix + msg.content;

        if (compact_mode) {
            lines.push_back(content);
        } else {
            lines = StringUtils::wrap_text(content, width - 4); // 4 for borders/padding
        }

        return lines;
    }
};

ChatWidget::ChatWidget() : impl_(std::make_unique<Impl>()) {}
ChatWidget::~ChatWidget() = default;

void ChatWidget::add_message(const Message& message) {
    impl_->messages.push_back(message);
    impl_->apply_filter();
    if (impl_->scroll_position == 0) {
        impl_->scroll_position = 0;
    }

    if (impl_->scroll_handler) {
        impl_->scroll_handler(impl_->scroll_position);
    }
}

void ChatWidget::update_message(const std::string& message_id, const Message& message) {
    for (auto& msg : impl_->messages) {
        if (msg.id == message_id) {
            msg = message;
            break;
        }
    }
    impl_->apply_filter();
}

void ChatWidget::remove_message(const std::string& message_id) {
    impl_->messages.erase(
        std::remove_if(impl_->messages.begin(), impl_->messages.end(),
                      [&](const Message& msg) { return msg.id == message_id; }),
        impl_->messages.end()
    );
    impl_->apply_filter();
}

void ChatWidget::clear_messages() {
    impl_->messages.clear();
    impl_->filtered_messages.clear();
    impl_->scroll_position = 0;
}

std::vector<Message> ChatWidget::get_messages() const {
    return impl_->messages;
}

Message ChatWidget::get_message(const std::string& message_id) const {
    for (const auto& msg : impl_->messages) {
        if (msg.id == message_id) {
            return msg;
        }
    }
    return Message();
}

bool ChatWidget::has_message(const std::string& message_id) const {
    return std::any_of(impl_->messages.begin(), impl_->messages.end(),
                      [&](const Message& msg) { return msg.id == message_id; });
}

void ChatWidget::set_visible(bool visible) {
    impl_->visible = visible;
}

void ChatWidget::set_position(int x, int y) {
    impl_->x = x;
    impl_->y = y;
}

void ChatWidget::set_size(int width, int height) {
    impl_->width = width;
    impl_->height = height;
}

void ChatWidget::refresh() {
    LOG_DEBUG("ChatWidget refreshed");
}

void ChatWidget::scroll_to_bottom() {
    impl_->scroll_position = 0;
    if (impl_->scroll_handler) {
        impl_->scroll_handler(impl_->scroll_position);
    }
}

void ChatWidget::scroll_to_message(const std::string& message_id) {
    for (size_t i = 0; i < impl_->filtered_messages.size(); ++i) {
        if (impl_->filtered_messages[i].id == message_id) {
            int visible_messages = impl_->height - 2; // Account for borders
            if (i < static_cast<size_t>(visible_messages)) {
                impl_->scroll_position = 0;
            } else {
                impl_->scroll_position = static_cast<int>(i - visible_messages + 1);
            }

            if (impl_->scroll_handler) {
                impl_->scroll_handler(impl_->scroll_position);
            }
            break;
        }
    }
}

void ChatWidget::set_filter(const std::string& filter) {
    impl_->filter_text = filter;
    impl_->apply_filter();
    impl_->scroll_position = 0;
}

void ChatWidget::clear_filter() {
    impl_->filter_text.clear();
    impl_->show_only_user_id.clear();
    impl_->show_only_type = MessageType::TEXT;
    impl_->apply_filter();
}

void ChatWidget::show_only_user(const std::string& user_id) {
    impl_->show_only_user_id = user_id;
    impl_->apply_filter();
    impl_->scroll_position = 0;
}

void ChatWidget::show_only_type(MessageType type) {
    impl_->show_only_type = type;
    impl_->apply_filter();
    impl_->scroll_position = 0;
}

void ChatWidget::set_message_click_handler(MessageClickHandler handler) {
    impl_->message_click_handler = std::move(handler);
}

void ChatWidget::set_scroll_handler(ScrollHandler handler) {
    impl_->scroll_handler = std::move(handler);
}

void ChatWidget::set_show_timestamps(bool show) {
    impl_->show_timestamps = show;
}

void ChatWidget::set_show_avatars(bool show) {
    impl_->show_avatars = show;
}

void ChatWidget::set_compact_mode(bool compact) {
    impl_->compact_mode = compact;
}

void ChatWidget::set_max_messages(size_t max) {
    impl_->max_messages = max;
    impl_->apply_filter();
}

void ChatWidget::set_theme(const std::string& theme) {
    LOG_INFO("ChatWidget theme set to: " + theme);
}

bool ChatWidget::can_scroll_up() const {
    return impl_->scroll_position > 0;
}

bool ChatWidget::can_scroll_down() const {
    int visible_messages = impl_->height - 2;
    return impl_->scroll_position < static_cast<int>(impl_->filtered_messages.size()) - visible_messages;
}

void ChatWidget::scroll_up(int lines) {
    impl_->scroll_position = std::max(0, impl_->scroll_position - lines);
    if (impl_->scroll_handler) {
        impl_->scroll_handler(impl_->scroll_position);
    }
}

void ChatWidget::scroll_down(int lines) {
    int max_scroll = std::max(0, static_cast<int>(impl_->filtered_messages.size()) - (impl_->height - 2));
    impl_->scroll_position = std::min(max_scroll, impl_->scroll_position + lines);
    if (impl_->scroll_handler) {
        impl_->scroll_handler(impl_->scroll_position);
    }
}

void ChatWidget::page_up() {
    scroll_up(impl_->height - 2);
}

void ChatWidget::page_down() {
    scroll_down(impl_->height - 2);
}

void ChatWidget::search(const std::string& query) {
    set_filter(query);
}

void ChatWidget::clear_search() {
    clear_filter();
}

bool ChatWidget::is_visible() const {
    return impl_->visible;
}

int ChatWidget::get_scroll_position() const {
    return impl_->scroll_position;
}

int ChatWidget::get_visible_message_count() const {
    return std::min(impl_->height - 2, static_cast<int>(impl_->filtered_messages.size()));
}

std::string ChatWidget::get_selected_message_id() const {
    return impl_->selected_message_id;
}