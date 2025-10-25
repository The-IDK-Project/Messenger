#pragma once

#include "Interface.h"
#include <QMainWindow>
#include <QString>
#include <memory>

class QTextEdit;
class QListWidget;
class QLineEdit;
class QSplitter;
class QStatusBar;
class QSystemTrayIcon;

class GUI : public Interface, public QMainWindow {
    Q_OBJECT

public:
    GUI(QWidget* parent = nullptr);
    ~GUI() override;

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

private slots:
    void on_send_message();
    void on_room_selected(QListWidgetItem* item);
    void on_input_return_pressed();
    void on_show_settings();
    void on_toggle_tray();
    void on_quit_application();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setup_ui();
    void setup_menus();
    void setup_tray_icon();
    void apply_theme(const std::string& theme);

    QSplitter* main_splitter_;
    QTextEdit* chat_display_;
    QListWidget* room_list_;
    QListWidget* user_list_;
    QLineEdit* input_field_;
    QStatusBar* status_bar_;
    QSystemTrayIcon* tray_icon_;

    std::string active_room_id_;
    std::map<std::string, ChatRoom> rooms_;
    std::map<std::string, User> users_;
    std::map<std::string, std::vector<Message>> room_messages_;

    bool use_tray_ = false;
};