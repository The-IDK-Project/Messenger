#pragma once

#include "../../core/Message.h"
#include <string>
#include <vector>
#include <functional>

class ChatWidget {
public:
    using MessageClickHandler = std::function<void(const Message& message)>;
    using ScrollHandler = std::function<void(int position)>;

    ChatWidget();
    ~ChatWidget();

    void add_message(const Message& message);
    void update_message(const std::string& message_id, const Message& message);
    void remove_message(const std::string& message_id);
    void clear_messages();

    std::vector<Message> get_messages() const;
    Message get_message(const std::string& message_id) const;
    bool has_message(const std::string& message_id) const;
    void set_visible(bool visible);
    void set_position(int x, int y);
    void set_size(int width, int height);
    void refresh();
    void scroll_to_bottom();
    void scroll_to_message(const std::string& message_id);
    void set_filter(const std::string& filter);
    void clear_filter();
    void show_only_user(const std::string& user_id);
    void show_only_type(MessageType type);
    void set_message_click_handler(MessageClickHandler handler);
    void set_scroll_handler(ScrollHandler handler);
    void set_show_timestamps(bool show);
    void set_show_avatars(bool show);
    void set_compact_mode(bool compact);
    void set_max_messages(size_t max);
    void set_theme(const std::string& theme);
    bool can_scroll_up() const;
    bool can_scroll_down() const;
    void scroll_up(int lines = 1);
    void scroll_down(int lines = 1);
    void page_up();
    void page_down();
    void search(const std::string& query);
    void search_next();
    void search_previous();
    void clear_search();
    bool is_visible() const;
    int get_scroll_position() const;
    int get_visible_message_count() const;
    std::string get_selected_message_id() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};