#include "ui/widgets/InputWidget.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <algorithm>

class InputWidget::Impl {
public:
    std::string text;
    std::string placeholder = "Type a message...";
    size_t max_length = 2000;
    size_t cursor_position = 0;
    bool multiline = false;
    bool visible = true;
    bool has_focus = false;
    int x = 0, y = 0, width = 80, height = 1;

    std::vector<std::string> history;
    int history_position = -1;
    std::vector<std::string> completion_list;
    int completion_position = -1;
    std::string current_completion_prefix;

    SubmitHandler submit_handler;
    ChangeHandler change_handler;
    TabCompleteHandler tab_complete_handler;
    HistoryHandler history_handler;

    void add_to_history_internal(const std::string& text) {
        if (text.empty()) return;
        history.erase(
            std::remove(history.begin(), history.end(), text),
            history.end()
        );
        history.insert(history.begin(), text);
        if (history.size() > 100) {
            history.pop_back();
        }
    }

    void trigger_completion_internal() {
        if (!tab_complete_handler) return;
        size_t word_start = text.find_last_of(" \t\n", cursor_position - 1);
        if (word_start == std::string::npos) {
            word_start = 0;
        } else {
            word_start++;
        }

        current_completion_prefix = text.substr(word_start, cursor_position - word_start);
        std::string completion = tab_complete_handler(current_completion_prefix);

        if (!completion.empty()) {
            text = text.substr(0, word_start) + completion + text.substr(cursor_position);
            cursor_position = word_start + completion.length();

            if (change_handler) {
                change_handler(text);
            }
        }
    }
};

InputWidget::InputWidget() : impl_(std::make_unique<Impl>()) {}
InputWidget::~InputWidget() = default;

void InputWidget::set_text(const std::string& text) {
    impl_->text = text;
    impl_->cursor_position = text.length();

    if (impl_->change_handler) {
        impl_->change_handler(text);
    }
}

std::string InputWidget::get_text() const {
    return impl_->text;
}

void InputWidget::clear() {
    impl_->text.clear();
    impl_->cursor_position = 0;
    impl_->history_position = -1;
    impl_->completion_position = -1;

    if (impl_->change_handler) {
        impl_->change_handler(impl_->text);
    }
}

void InputWidget::insert_text(const std::string& text) {
    if (impl_->text.length() + text.length() > impl_->max_length) {
        return;
    }

    impl_->text.insert(impl_->cursor_position, text);
    impl_->cursor_position += text.length();

    if (impl_->change_handler) {
        impl_->change_handler(impl_->text);
    }
}

void InputWidget::set_visible(bool visible) {
    impl_->visible = visible;
}

void InputWidget::set_position(int x, int y) {
    impl_->x = x;
    impl_->y = y;
}

void InputWidget::set_size(int width, int height) {
    impl_->width = width;
    impl_->height = height;
}

void InputWidget::refresh() {
    LOG_DEBUG("InputWidget refreshed");
}

void InputWidget::focus() {
    impl_->has_focus = true;
    LOG_DEBUG("InputWidget focused");
}

void InputWidget::blur() {
    impl_->has_focus = false;
    impl_->completion_position = -1;
    LOG_DEBUG("InputWidget blurred");
}

void InputWidget::set_submit_handler(SubmitHandler handler) {
    impl_->submit_handler = std::move(handler);
}

void InputWidget::set_change_handler(ChangeHandler handler) {
    impl_->change_handler = std::move(handler);
}

void InputWidget::set_tab_complete_handler(TabCompleteHandler handler) {
    impl_->tab_complete_handler = std::move(handler);
}

void InputWidget::set_history_handler(HistoryHandler handler) {
    impl_->history_handler = std::move(handler);
}

void InputWidget::set_placeholder(const std::string& placeholder) {
    impl_->placeholder = placeholder;
}

void InputWidget::set_max_length(size_t max) {
    impl_->max_length = max;
    if (impl_->text.length() > max) {
        impl_->text = impl_->text.substr(0, max);
        impl_->cursor_position = std::min(impl_->cursor_position, max);
    }
}

void InputWidget::set_multiline(bool multiline) {
    impl_->multiline = multiline;
    impl_->height = multiline ? 3 : 1;
}

void InputWidget::set_theme(const std::string& theme) {
    LOG_INFO("InputWidget theme set to: " + theme);
}

void InputWidget::set_font_size(int size) {
    LOG_DEBUG("InputWidget font size set to: " + std::to_string(size));
}

void InputWidget::add_to_history(const std::string& text) {
    impl_->add_to_history_internal(text);
}

void InputWidget::clear_history() {
    impl_->history.clear();
    impl_->history_position = -1;
}

void InputWidget::navigate_history_up() {
    if (impl_->history.empty()) return;

    if (impl_->history_position == -1) {
        impl_->history_position = 0;
        impl_->history.insert(impl_->history.begin(), impl_->text);
    } else if (impl_->history_position < static_cast<int>(impl_->history.size()) - 1) {
        impl_->history_position++;
    }

    if (impl_->history_position >= 0 && impl_->history_position < static_cast<int>(impl_->history.size())) {
        impl_->text = impl_->history[impl_->history_position];
        impl_->cursor_position = impl_->text.length();

        if (impl_->change_handler) {
            impl_->change_handler(impl_->text);
        }
    }
}

void InputWidget::navigate_history_down() {
    if (impl_->history.empty() || impl_->history_position == -1) return;

    if (impl_->history_position > 0) {
        impl_->history_position--;
        impl_->text = impl_->history[impl_->history_position];
        impl_->cursor_position = impl_->text.length();
    } else {
        impl_->history_position = -1;
        impl_->text.clear();
    }

    if (impl_->change_handler) {
        impl_->change_handler(impl_->text);
    }
}

std::vector<std::string> InputWidget::get_history() const {
    return impl_->history;
}

void InputWidget::set_history(const std::vector<std::string>& history) {
    impl_->history = history;
}

void InputWidget::set_completion_list(const std::vector<std::string>& completions) {
    impl_->completion_list = completions;
}

void InputWidget::clear_completion_list() {
    impl_->completion_list.clear();
    impl_->completion_position = -1;
}

void InputWidget::trigger_completion() {
    impl_->trigger_completion_internal();
}

void InputWidget::next_completion() {
    if (impl_->completion_list.empty()) return;

    if (impl_->completion_position == -1) {
        impl_->completion_position = 0;
    } else {
        impl_->completion_position = (impl_->completion_position + 1) % impl_->completion_list.size();
    }

}

void InputWidget::previous_completion() {
    if (impl_->completion_list.empty()) return;

    if (impl_->completion_position == -1) {
        impl_->completion_position = impl_->completion_list.size() - 1;
    } else {
        impl_->completion_position = (impl_->completion_position - 1 + impl_->completion_list.size()) % impl_->completion_list.size();
    }

}

void InputWidget::move_cursor_left() {
    if (impl_->cursor_position > 0) {
        impl_->cursor_position--;
    }
}

void InputWidget::move_cursor_right() {
    if (impl_->cursor_position < impl_->text.length()) {
        impl_->cursor_position++;
    }
}

void InputWidget::move_cursor_home() {
    impl_->cursor_position = 0;
}

void InputWidget::move_cursor_end() {
    impl_->cursor_position = impl_->text.length();
}

void InputWidget::delete_left() {
    if (impl_->cursor_position > 0) {
        impl_->text.erase(impl_->cursor_position - 1, 1);
        impl_->cursor_position--;

        if (impl_->change_handler) {
            impl_->change_handler(impl_->text);
        }
    }
}

void InputWidget::delete_right() {
    if (impl_->cursor_position < impl_->text.length()) {
        impl_->text.erase(impl_->cursor_position, 1);

        if (impl_->change_handler) {
            impl_->change_handler(impl_->text);
        }
    }
}

void InputWidget::delete_word_left() {
    if (impl_->cursor_position == 0) return;

    size_t word_start = impl_->text.find_last_of(" \t\n", impl_->cursor_position - 1);
    if (word_start == std::string::npos) {
        word_start = 0;
    } else {
        word_start++;
    }

    impl_->text.erase(word_start, impl_->cursor_position - word_start);
    impl_->cursor_position = word_start;

    if (impl_->change_handler) {
        impl_->change_handler(impl_->text);
    }
}

void InputWidget::delete_word_right() {
    if (impl_->cursor_position >= impl_->text.length()) return;

    size_t word_end = impl_->text.find_first_of(" \t\n", impl_->cursor_position);
    if (word_end == std::string::npos) {
        word_end = impl_->text.length();
    }

    impl_->text.erase(impl_->cursor_position, word_end - impl_->cursor_position);

    if (impl_->change_handler) {
        impl_->change_handler(impl_->text);
    }
}

bool InputWidget::is_visible() const {
    return impl_->visible;
}

bool InputWidget::has_focus() const {
    return impl_->has_focus;
}

size_t InputWidget::get_cursor_position() const {
    return impl_->cursor_position;
}

void InputWidget::set_cursor_position(size_t pos) {
    impl_->cursor_position = std::min(pos, impl_->text.length());
}