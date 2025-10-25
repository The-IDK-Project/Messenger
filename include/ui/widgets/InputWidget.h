#pragma once

#include <string>
#include <vector>
#include <functional>

class InputWidget {
public:
    using SubmitHandler = std::function<void(const std::string& text)>;
    using ChangeHandler = std::function<void(const std::string& text)>;
    using TabCompleteHandler = std::function<std::string(const std::string& prefix)>;
    using HistoryHandler = std::function<std::vector<std::string>()>;

    InputWidget();
    ~InputWidget();

    void set_text(const std::string& text);
    std::string get_text() const;
    void clear();
    void insert_text(const std::string& text);

    void set_visible(bool visible);
    void set_position(int x, int y);
    void set_size(int width, int height);
    void refresh();
    void focus();
    void blur();

    void set_submit_handler(SubmitHandler handler);
    void set_change_handler(ChangeHandler handler);
    void set_tab_complete_handler(TabCompleteHandler handler);
    void set_history_handler(HistoryHandler handler);

    void set_placeholder(const std::string& placeholder);
    void set_max_length(size_t max);
    void set_multiline(bool multiline);
    void set_theme(const std::string& theme);
    void set_font_size(int size);

    void add_to_history(const std::string& text);
    void clear_history();
    void navigate_history_up();
    void navigate_history_down();

    std::vector<std::string> get_history() const;
    void set_history(const std::vector<std::string>& history);

    void set_completion_list(const std::vector<std::string>& completions);
    void clear_completion_list();
    void trigger_completion();
    void next_completion();
    void previous_completion();

    void move_cursor_left();
    void move_cursor_right();
    void move_cursor_home();
    void move_cursor_end();
    void delete_left();
    void delete_right();
    void delete_word_left();
    void delete_word_right();

    bool is_visible() const;
    bool has_focus() const;
    size_t get_cursor_position() const;
    void set_cursor_position(size_t pos);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};