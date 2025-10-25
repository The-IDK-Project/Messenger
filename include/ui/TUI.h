#pragma once

#include "Interface.h"
#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>

class TUI : public Interface {
public:
    TUI();
    ~TUI() override;

    bool initialize() override;
    void shutdown() override;
    int run() override;

    void display_message(const Message& message) override;
    void update_message_status(const std::string& message_id,
                             MessageStatus status) override;
    void clear_messages() override;

    void set_rooms(const std::vector<ChatRoom>& rooms) override;
    void add_room(const ChatRoom& room) override;
    void remove_room(const std::string& room_id) override;
    void set_active_room(const std::string& room_id) override;

    void set_users(const std::vector<User>& users) override;
    void update_user_presence(const std::string& user_id, bool online) override;

    void set_connection_status(const std::string& protocol,
                             bool connected) override;
    void show_error(const std::string& error) override;
    void show_notification(const std::string& title,
                         const std::string& message) override;

    void set_input_text(const std::string& text) override;
    std::string get_input_text() override;
    void clear_input() override;
    void focus_input() override;

    void set_input_handler(InputHandler handler) override;
    void set_command_handler(CommandHandler handler) override;
    void set_room_select_handler(RoomSelectHandler handler) override;
    void set_quit_handler(QuitHandler handler) override;

    void refresh() override;
    void redraw() override;
    void set_title(const std::string& title) override;

    void set_theme(const std::string& theme) override;
    void set_font_size(int size) override;
    void show_help() override;

    void handle_resize();
    void show_command_palette();
    void show_room_selector();
    void toggle_user_list();

private:
    struct Panel {
        WINDOW* win;
        int height;
        int width;
        int y;
        int x;
    };

    Panel chat_panel_;
    Panel room_list_panel_;
    Panel user_list_panel_;
    Panel input_panel_;
    Panel status_panel_;

    std::string active_room_id_;
    std::vector<ChatRoom> rooms_;
    std::vector<User> users_;
    std::vector<Message> messages_;
    std::map<std::string, bool> connection_status_;

    bool should_quit_ = false;
    bool show_users_ = true;

    int color_system_;
    int color_error_;
    int color_warning_;
    int color_success_;
    int color_highlight_;

    void setup_colors();
    void create_panels();
    void delete_panels();
    void resize_panels();

    void draw_chat();
    void draw_room_list();
    void draw_user_list();
    void draw_input();
    void draw_status();

    void process_input();
    void handle_key(int key);
    void handle_command(const std::string& input);
    void scroll_chat(int lines);

    std::string truncate_string(const std::string& str, size_t width);
    std::string format_message_time(const Message& message);
    std::string get_message_display(const Message& message);

    int chat_scroll_offset_ = 0;
    int room_list_scroll_offset_ = 0;
    int user_list_scroll_offset_ = 0;
};