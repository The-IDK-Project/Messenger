#include "ui/GUI.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QSplitter>
#include <QPushButton>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTextCursor>

GUI::GUI(QWidget* parent) : QMainWindow(parent) {
    setup_ui();
    setup_menus();
    setup_tray_icon();
}

GUI::~GUI() {
    shutdown();
}

bool GUI::initialize() {
    show();
    LOG_INFO("GUI initialized successfully");
    return true;
}

void GUI::shutdown() {
    LOG_INFO("GUI shutdown");
}

int GUI::run() {
    return QApplication::exec();
}

void GUI::display_message(const Message& message) {
    QMetaObject::invokeMethod(this, [this, message]() {
        auto& room_messages = room_messages_[message.room_id];
        room_messages.push_back(message);

        if (room_messages.size() > 1000) {
            room_messages.erase(room_messages.begin(), room_messages.begin() + 100);
        }

        if (message.room_id == active_room_id_) {
            update_chat_display();
        }

        if (message.room_id != active_room_id_ && message.type == MessageType::TEXT) {
            show_notification("New message", message.sender_name + ": " + message.content);
        }
    }, Qt::QueuedConnection);
}

void GUI::set_rooms(const std::vector<ChatRoom>& rooms) {
    QMetaObject::invokeMethod(this, [this, rooms]() {
        rooms_.clear();
        for (const auto& room : rooms) {
            rooms_[room.id] = room;
        }
        update_room_list();
    }, Qt::QueuedConnection);
}

void GUI::set_active_room(const std::string& room_id) {
    QMetaObject::invokeMethod(this, [this, room_id]() {
        active_room_id_ = room_id;
        update_chat_display();
        update_window_title();
    }, Qt::QueuedConnection);
}

void GUI::set_users(const std::vector<User>& users) {
    QMetaObject::invokeMethod(this, [this, users]() {
        users_.clear();
        for (const auto& user : users) {
            users_[user.id] = user;
        }
        update_user_list();
    }, Qt::QueuedConnection);
}

void GUI::set_connection_status(const std::string& protocol, bool connected) {
    QMetaObject::invokeMethod(this, [this, protocol, connected]() {
        QString status = connected ? "Connected to " + QString::fromStdString(protocol)
                                  : "Disconnected from " + QString::fromStdString(protocol);
        status_bar_->showMessage(status, 3000);
    }, Qt::QueuedConnection);
}

void GUI::show_error(const std::string& error) {
    QMetaObject::invokeMethod(this, [this, error]() {
        status_bar_->showMessage("Error: " + QString::fromStdString(error), 5000);
    }, Qt::QueuedConnection);
}

void GUI::show_notification(const std::string& title, const std::string& message) {
    if (tray_icon_ && tray_icon_->isVisible()) {
        tray_icon_->showMessage(
            QString::fromStdString(title),
            QString::fromStdString(message),
            QSystemTrayIcon::Information,
            3000
        );
    }
}

void GUI::show_incoming_call(const std::string& room_id, const std::string& caller_name, bool is_video) {
    QMetaObject::invokeMethod(this, [this, room_id, caller_name, is_video]() {
        QString type = is_video ? "Video" : "Voice";
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Incoming Call",
            type + " call from " + QString::fromStdString(caller_name) + ".\nAccept?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // Logic to accept call
            LOG_INFO("Call accepted");
        } else {
            // Logic to reject call
            LOG_INFO("Call rejected");
        }
    }, Qt::QueuedConnection);
}

void GUI::setup_ui() {
    setWindowTitle("Unified Messenger");
    setMinimumSize(800, 600);

    QWidget* central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QHBoxLayout* main_layout = new QHBoxLayout(central_widget);

    main_splitter_ = new QSplitter(Qt::Horizontal, central_widget);
    main_layout->addWidget(main_splitter_);
    room_list_ = new QListWidget(main_splitter_);
    room_list_->setMaximumWidth(200);
    main_splitter_->addWidget(room_list_);

    QWidget* chat_widget = new QWidget(main_splitter_);
    QVBoxLayout* chat_layout = new QVBoxLayout(chat_widget);

    // Call buttons layout
    QHBoxLayout* call_layout = new QHBoxLayout();
    QPushButton* voice_call_btn = new QPushButton("📞 Voice Call", chat_widget);
    QPushButton* video_call_btn = new QPushButton("📹 Video Call", chat_widget);
    call_layout->addWidget(voice_call_btn);
    call_layout->addWidget(video_call_btn);
    call_layout->addStretch();
    chat_layout->addLayout(call_layout);

    chat_display_ = new QTextEdit(chat_widget);
    chat_display_->setReadOnly(true);
    chat_layout->addWidget(chat_display_);

    QHBoxLayout* input_layout = new QHBoxLayout();
    input_field_ = new QLineEdit(chat_widget);
    QPushButton* send_button = new QPushButton("Send", chat_widget);
    QPushButton* circle_button = new QPushButton("⏺ Circle", chat_widget); // Video note button

    input_layout->addWidget(input_field_);
    input_layout->addWidget(send_button);
    input_layout->addWidget(circle_button);
    chat_layout->addLayout(input_layout);

    main_splitter_->addWidget(chat_widget);
    user_list_ = new QListWidget(main_splitter_);
    user_list_->setMaximumWidth(150);
    main_splitter_->addWidget(user_list_);
    main_splitter_->setSizes({200, 400, 150});

    status_bar_ = new QStatusBar(this);
    setStatusBar(status_bar_);
    status_bar_->showMessage("Ready");

    connect(room_list_, &QListWidget::itemClicked, this, &GUI::on_room_selected);
    connect(input_field_, &QLineEdit::returnPressed, this, &GUI::on_input_return_pressed);
    connect(send_button, &QPushButton::clicked, this, &GUI::on_send_message);

    // Call handlers
    connect(voice_call_btn, &QPushButton::clicked, this, [this]() {
        if (!active_room_id_.empty() && call_handler_) {
            call_handler_(active_room_id_, false);
        }
    });

    connect(video_call_btn, &QPushButton::clicked, this, [this]() {
        if (!active_room_id_.empty() && call_handler_) {
            call_handler_(active_room_id_, true);
        }
    });

    // Circle (Video note) handler
    connect(circle_button, &QPushButton::clicked, this, [this]() {
        if (!active_room_id_.empty() && video_message_handler_) {
            video_message_handler_(active_room_id_, "");
        }
    });
}

void GUI::setup_menus() {
    QMenuBar* menu_bar = new QMenuBar(this);
    setMenuBar(menu_bar);

    QMenu* file_menu = menu_bar->addMenu("File");
    QAction* settings_action = file_menu->addAction("Settings");
    QAction* quit_action = file_menu->addAction("Quit");

    connect(settings_action, &QAction::triggered, this, &GUI::on_show_settings);
    connect(quit_action, &QAction::triggered, this, &GUI::on_quit_application);

    QMenu* view_menu = menu_bar->addMenu("View");
    QAction* toggle_tray_action = view_menu->addAction("Toggle System Tray");

    connect(toggle_tray_action, &QAction::triggered, this, &GUI::on_toggle_tray);
}

void GUI::setup_tray_icon() {
    tray_icon_ = new QSystemTrayIcon(this);

    QMenu* tray_menu = new QMenu(this);
    QAction* show_action = tray_menu->addAction("Show");
    QAction* quit_action = tray_menu->addAction("Quit");

    connect(show_action, &QAction::triggered, this, &QWidget::show);
    connect(quit_action, &QAction::triggered, this, &GUI::on_quit_application);

    tray_icon_->setContextMenu(tray_menu);
    tray_icon_->setIcon(QApplication::windowIcon());
    tray_icon_->setToolTip("Unified Messenger");
}

void GUI::update_room_list() {
    room_list_->clear();

    for (const auto& [room_id, room] : rooms_) {
        QListWidgetItem* item = new QListWidgetItem(
            QString::fromStdString(room.get_display_name())
        );
        item->setData(Qt::UserRole, QString::fromStdString(room_id));
        room_list_->addItem(item);

        if (room_id == active_room_id_) {
            room_list_->setCurrentItem(item);
        }
    }
}

void GUI::update_chat_display() {
    chat_display_->clear();

    if (active_room_id_.empty()) return;

    const auto& messages = room_messages_[active_room_id_];
    for (const auto& message : messages) {
        QString html_message;

        if (message.type == MessageType::SYSTEM) {
            html_message = QString("<p style='color: gray; font-style: italic;'>[%1] * %2</p>")
                .arg(QString::fromStdString(message.get_display_time()))
                .arg(QString::fromStdString(message.content));
        } else {
            QString sender_color = message.is_from_me("current_user") ? "blue" : "black";
            html_message = QString("<p><span style='color: gray;'>[%1]</span> "
                                 "<span style='color: %2; font-weight: bold;'>%3:</span> %4</p>")
                .arg(QString::fromStdString(message.get_display_time()))
                .arg(sender_color)
                .arg(QString::fromStdString(message.sender_name))
                .arg(QString::fromStdString(message.content));
        }

        chat_display_->append(html_message);
    }

    QTextCursor cursor = chat_display_->textCursor();
    cursor.movePosition(QTextCursor::End);
    chat_display_->setTextCursor(cursor);
}

void GUI::update_user_list() {
    user_list_->clear();

    for (const auto& [user_id, user] : users_) {
        QString display_name = QString::fromStdString(user.get_best_name());
        if (user.is_online()) {
            display_name = "● " + display_name;
        } else {
            display_name = "○ " + display_name;
        }

        user_list_->addItem(display_name);
    }
}

void GUI::update_window_title() {
    if (active_room_id_.empty()) {
        setWindowTitle("Unified Messenger");
    } else {
        auto it = rooms_.find(active_room_id_);
        if (it != rooms_.end()) {
            setWindowTitle(QString::fromStdString(it->second.get_display_name() + " - Unified Messenger"));
        }
    }
}

// Slots
void GUI::on_send_message() {
    QString text = input_field_->text().trimmed();
    if (text.isEmpty()) return;

    if (input_handler_) {
        input_handler_(text.toStdString());
    }

    input_field_->clear();
}

void GUI::on_room_selected(QListWidgetItem* item) {
    QString room_id = item->data(Qt::UserRole).toString();

    if (room_select_handler_) {
        room_select_handler_(room_id.toStdString());
    }
}

void GUI::on_input_return_pressed() {
    on_send_message();
}

void GUI::on_show_settings() {
    LOG_INFO("Showing settings dialog");
}

void GUI::on_toggle_tray() {
    use_tray_ = !use_tray_;
    if (use_tray_) {
        tray_icon_->show();
    } else {
        tray_icon_->hide();
    }
}

void GUI::on_quit_application() {
    if (quit_handler_) {
        quit_handler_();
    }
    QApplication::quit();
}

void GUI::closeEvent(QCloseEvent* event) {
    if (use_tray_ && tray_icon_->isVisible()) {
        hide();
        event->ignore();
    } else {
        if (quit_handler_) {
            quit_handler_();
        }
        event->accept();
    }
}

void GUI::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (use_tray_ && tray_icon_->isVisible()) {
            hide();
        }
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void GUI::update_message_status(const std::string& message_id, MessageStatus status) {}
void GUI::clear_messages() {}
void GUI::add_room(const ChatRoom& room) {}
void GUI::remove_room(const std::string& room_id) {}
void GUI::update_user_presence(const std::string& user_id, bool online) {}
void GUI::set_input_text(const std::string& text) {}
std::string GUI::get_input_text() { return ""; }
void GUI::clear_input() {}
void GUI::focus_input() {}
void GUI::refresh() {}
void GUI::redraw() {}
void GUI::set_title(const std::string& title) {}
void GUI::set_theme(const std::string& theme) {}
void GUI::set_font_size(int size) {}
void GUI::show_help() {}
void GUI::apply_theme(const std::string& theme) {}
void GUI::set_input_handler(InputHandler handler) { Interface::set_input_handler(std::move(handler)); }
void GUI::set_command_handler(CommandHandler handler) { Interface::set_command_handler(std::move(handler)); }
void GUI::set_room_select_handler(RoomSelectHandler handler) { Interface::set_room_select_handler(std::move(handler)); }
void GUI::set_quit_handler(QuitHandler handler) { Interface::set_quit_handler(std::move(handler)); }
