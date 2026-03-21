#include "ui/TUI.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <ncurses.h>
#include <thread>
#include <sstream>

TUI::TUI() {
    setup_colors();
}

TUI::~TUI() {
    shutdown();
}

bool TUI::initialize() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    if (!has_colors()) {
        endwin();
        LOG_ERROR("Terminal does not support colors");
        return false;
    }

    start_color();
    setup_colors();
    create_panels();
    refresh();

    LOG_INFO("TUI initialized successfully");
    return true;
}

void TUI::shutdown() {
    delete_panels();
    endwin();
    LOG_INFO("TUI shutdown");
}

int TUI::run() {
    if (!initialize()) {
        return 1;
    }

    while (!should_quit_) {
        process_input();
        redraw();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    shutdown();
    return 0;
}

void TUI::display_message(const Message& message) {
    messages_.push_back(message);

    if (messages_.size() > 1000) {
        messages_.erase(messages_.begin(), messages_.begin() + 100);
    }

    if (chat_scroll_offset_ == 0) {
        refresh();
    }
}

void TUI::set_rooms(const std::vector<ChatRoom>& rooms) {
    rooms_ = rooms;

    if (active_room_id_.empty() && !rooms_.empty()) {
        active_room_id_ = rooms_[0].id;
    }

    refresh();
}

void TUI::set_active_room(const std::string& room_id) {
    active_room_id_ = room_id;
    chat_scroll_offset_ = 0;
    refresh();
}

void TUI::set_users(const std::vector<User>& users) {
    users_ = users;
    refresh();
}

void TUI::set_connection_status(const std::string& protocol, bool connected) {
    connection_status_[protocol] = connected;
    refresh();
}

void TUI::show_error(const std::string& error) {
    LOG_ERROR("TUI Error: " + error);
}

std::string TUI::get_input_text() {
    return "";
}

void TUI::clear_input() {
}

void TUI::focus_input() {
}

void TUI::refresh() {
    if (!stdscr) return;

    clear();
    draw_room_list();
    draw_chat();
    draw_user_list();
    draw_input();
    draw_status();

    ::refresh();
}

void TUI::redraw() {
    refresh();
}

void TUI::set_theme(const std::string& theme) {
    LOG_INFO("TUI theme set to: " + theme);
}

void TUI::set_font_size(int size) {
    LOG_INFO("TUI font size set to: " + std::to_string(size));
}

void TUI::show_help() {
    LOG_INFO("Showing TUI help");
}

void TUI::setup_colors() {
    init_pair(1, COLOR_RED, COLOR_BLACK);     // Error
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Warning
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Success
    init_pair(4, COLOR_BLUE, COLOR_BLACK);    // Highlight
    init_pair(5, COLOR_CYAN, COLOR_BLACK);    // System
    init_pair(6, COLOR_WHITE, COLOR_BLUE);    // Selected
    init_pair(7, COLOR_BLACK, COLOR_WHITE);   // Input field
}

void TUI::create_panels() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    room_list_panel_.width = max_x * 0.2;
    room_list_panel_.height = max_y - 2;
    room_list_panel_.y = 0;
    room_list_panel_.x = 0;
    room_list_panel_.win = newwin(room_list_panel_.height, room_list_panel_.width,
                                 room_list_panel_.y, room_list_panel_.x);

    if (show_users_) {
        user_list_panel_.width = max_x * 0.2;
        user_list_panel_.height = max_y - 2;
        user_list_panel_.y = 0;
        user_list_panel_.x = max_x - user_list_panel_.width;
        user_list_panel_.win = newwin(user_list_panel_.height, user_list_panel_.width,
                                     user_list_panel_.y, user_list_panel_.x);
    }

    chat_panel_.width = max_x - room_list_panel_.width - (show_users_ ? user_list_panel_.width : 0);
    chat_panel_.height = max_y - 2;
    chat_panel_.y = 0;
    chat_panel_.x = room_list_panel_.width;
    chat_panel_.win = newwin(chat_panel_.height, chat_panel_.width,
                            chat_panel_.y, chat_panel_.x);

    input_panel_.width = max_x;
    input_panel_.height = 1;
    input_panel_.y = max_y - 1;
    input_panel_.x = 0;
    input_panel_.win = newwin(input_panel_.height, input_panel_.width,
                             input_panel_.y, input_panel_.x);

    status_panel_.width = max_x;
    status_panel_.height = 1;
    status_panel_.y = max_y - 2;
    status_panel_.x = 0;
    status_panel_.win = newwin(status_panel_.height, status_panel_.width,
                              status_panel_.y, status_panel_.x);
}

void TUI::delete_panels() {
    if (room_list_panel_.win) delwin(room_list_panel_.win);
    if (chat_panel_.win) delwin(chat_panel_.win);
    if (user_list_panel_.win) delwin(user_list_panel_.win);
    if (input_panel_.win) delwin(input_panel_.win);
    if (status_panel_.win) delwin(status_panel_.win);
}

void TUI::draw_room_list() {
    if (!room_list_panel_.win) return;

    werase(room_list_panel_.win);
    box(room_list_panel_.win, 0, 0);
    mvwprintw(room_list_panel_.win, 0, 2, " Rooms ");

    int y = 1;
    for (const auto& room : rooms_) {
        if (y >= room_list_panel_.height - 1) break;

        if (room.id == active_room_id_) {
            wattron(room_list_panel_.win, COLOR_PAIR(6));
        }

        std::string display_name = truncate_string(room.get_display_name(), room_list_panel_.width - 4);
        mvwprintw(room_list_panel_.win, y, 2, "%s", display_name.c_str());

        if (room.id == active_room_id_) {
            wattroff(room_list_panel_.win, COLOR_PAIR(6));
        }

        y++;
    }

    wrefresh(room_list_panel_.win);
}

void TUI::draw_chat() {
    if (!chat_panel_.win) return;

    werase(chat_panel_.win);
    box(chat_panel_.win, 0, 0);

    std::string room_name = "Chat";
    for (const auto& room : rooms_) {
        if (room.id == active_room_id_) {
            room_name = room.get_display_name();
            break;
        }
    }
    mvwprintw(chat_panel_.win, 0, 2, " %s ", room_name.c_str());

    int y = chat_panel_.height - 2;
    int message_count = 0;

    for (auto it = messages_.rbegin(); it != messages_.rend(); ++it) {
        if (it->room_id != active_room_id_) continue;

        if (y < 1) break;

        std::string display = get_message_display(*it);
        std::vector<std::string> lines = StringUtils::wrap_text(display, chat_panel_.width - 4);

        for (auto line_it = lines.rbegin(); line_it != lines.rend(); ++line_it) {
            if (y < 1) break;

            if (it->type == MessageType::SYSTEM) {
                wattron(chat_panel_.win, COLOR_PAIR(5));
            } else if (it->is_from_me("current_user")) { // Would need actual user ID
                wattron(chat_panel_.win, COLOR_PAIR(4));
            }

            mvwprintw(chat_panel_.win, y, 2, "%s", line_it->c_str());

            if (it->type == MessageType::SYSTEM || it->is_from_me("current_user")) {
                wattroff(chat_panel_.win, A_COLOR);
            }

            y--;
            message_count++;
        }
    }

    wrefresh(chat_panel_.win);
}

void TUI::draw_user_list() {
    if (!user_list_panel_.win || !show_users_) return;

    werase(user_list_panel_.win);
    box(user_list_panel_.win, 0, 0);
    mvwprintw(user_list_panel_.win, 0, 2, " Users ");

    int y = 1;
    for (const auto& user : users_) {
        if (y >= user_list_panel_.height - 1) break;

        std::string display_name = truncate_string(user.get_best_name(), user_list_panel_.width - 4);
        if (user.is_online()) {
            wattron(user_list_panel_.win, COLOR_PAIR(3));
            mvwprintw(user_list_panel_.win, y, 2, "● %s", display_name.c_str());
            wattroff(user_list_panel_.win, COLOR_PAIR(3));
        } else {
            mvwprintw(user_list_panel_.win, y, 2, "○ %s", display_name.c_str());
        }

        y++;
    }

    wrefresh(user_list_panel_.win);
}

void TUI::draw_input() {
    if (!input_panel_.win) return;

    werase(input_panel_.win);
    wattron(input_panel_.win, COLOR_PAIR(7));

    std::string prompt = "> ";
    mvwprintw(input_panel_.win, 0, 0, "%s", prompt.c_str());

    std::string input_text = "Type your message here...";
    mvwprintw(input_panel_.win, 0, prompt.length(), "%s", input_text.c_str());

    wattroff(input_panel_.win, COLOR_PAIR(7));
    wrefresh(input_panel_.win);
}

void TUI::draw_status() {
    if (!status_panel_.win) return;

    werase(status_panel_.win);
    std::string status = "Connected: ";
    for (const auto& [protocol, connected] : connection_status_) {
        if (connected) {
            status += protocol + " ";
        }
    }

    mvwprintw(status_panel_.win, 0, 0, "%s", status.c_str());
    std::string help = "F1: Help | F2: Rooms | F10: Quit";
    mvwprintw(status_panel_.win, 0, status_panel_.width - help.length() - 1, "%s", help.c_str());

    wrefresh(status_panel_.win);
}

void TUI::process_input() {
    int ch = getch();
    if (ch == ERR) return;

    handle_key(ch);
}

void TUI::handle_key(int key) {
    switch (key) {
        case KEY_F(1):
            show_help();
            break;
        case KEY_F(2):
            show_room_selector();
            break;
        case KEY_F(10):
        case 'Q':
        case 'q':
            if (quit_handler_) {
                quit_handler_();
            }
            should_quit_ = true;
            break;
        case KEY_UP:
            scroll_chat(-1);
            break;
        case KEY_DOWN:
            scroll_chat(1);
            break;
        case '\n':
            if (input_handler_) {
                input_handler_("test message");
            }
            break;
        default:
            break;
    }
}

void TUI::scroll_chat(int lines) {
    chat_scroll_offset_ += lines;
    if (chat_scroll_offset_ < 0) chat_scroll_offset_ = 0;
    refresh();
}

std::string TUI::truncate_string(const std::string& str, size_t width) {
    if (str.length() <= width) return str;
    return str.substr(0, width - 3) + "...";
}

std::string TUI::format_message_time(const Message& message) {
    return message.get_display_time();
}

std::string TUI::get_message_display(const Message& message) {
    std::string timestamp = format_message_time(message);
    std::string sender = message.sender_name;

    if (message.type == MessageType::SYSTEM) {
        return "[" + timestamp + "] * " + message.content;
    } else {
        return "[" + timestamp + "] " + sender + ": " + message.content;
    }
}

void TUI::handle_resize() {}
void TUI::show_command_palette() {}
void TUI::show_room_selector() {}
void TUI::toggle_user_list() {}
void TUI::handle_command(const std::string& input) {}
void TUI::update_message_status(const std::string& message_id, MessageStatus status) {}
void TUI::clear_messages() {}
void TUI::add_room(const ChatRoom& room) {}
void TUI::remove_room(const std::string& room_id) {}
void TUI::update_user_presence(const std::string& user_id, bool online) {}
void TUI::show_notification(const std::string& title, const std::string& message) {}
void TUI::set_input_text(const std::string& text) {}
void TUI::set_title(const std::string& title) {}
void TUI::show_incoming_call(const std::string& room_id, const std::string& caller_name, bool is_video) {}
