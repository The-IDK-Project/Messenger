#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../core/Message.h"
#include "../core/User.h"
#include "../core/ChatRoom.h"
#include "../platform/ScreenCapturer.h"

class Interface {
public:
    using InputHandler = std::function<void(const std::string& input)>;
    using CommandHandler = std::function<void(const std::string& command,
                                            const std::vector<std::string>& args)>;
    using RoomSelectHandler = std::function<void(const std::string& room_id)>;
    using QuitHandler = std::function<void()>;
    using CallHandler = std::function<void(const std::string& room_id, bool is_video)>;
    using VoiceMessageHandler = std::function<void(const std::string& room_id, const std::string& file_path)>;
    using VideoMessageHandler = std::function<void(const std::string& room_id, const std::string& file_path)>;
    using ScreenShareHandler = std::function<void(const std::string& call_id, int screen_id)>;

    virtual ~Interface() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual int run() = 0;

    virtual void display_message(const Message& message) = 0;
    virtual void update_message_status(const std::string& message_id,
                                     MessageStatus status) = 0;
    virtual void clear_messages() = 0;

    virtual void set_rooms(const std::vector<ChatRoom>& rooms) = 0;
    virtual void add_room(const ChatRoom& room) = 0;
    virtual void remove_room(const std::string& room_id) = 0;
    virtual void set_active_room(const std::string& room_id) = 0;

    virtual void set_users(const std::vector<User>& users) = 0;
    virtual void update_user_presence(const std::string& user_id, bool online) = 0;

    virtual void set_connection_status(const std::string& protocol,
                                     bool connected) = 0;
    virtual void show_error(const std::string& error) = 0;
    virtual void show_notification(const std::string& title,
                                 const std::string& message) = 0;

    virtual void set_input_text(const std::string& text) = 0;
    virtual std::string get_input_text() = 0;
    virtual void clear_input() = 0;
    virtual void focus_input() = 0;

    virtual void set_input_handler(InputHandler handler) = 0;
    virtual void set_command_handler(CommandHandler handler) = 0;
    virtual void set_room_select_handler(RoomSelectHandler handler) = 0;
    virtual void set_quit_handler(QuitHandler handler) = 0;
    virtual void set_call_handler(CallHandler handler) = 0;
    virtual void set_voice_message_handler(VoiceMessageHandler handler) = 0;
    virtual void set_video_message_handler(VideoMessageHandler handler) = 0;
    virtual void set_screen_share_handler(ScreenShareHandler handler) = 0;

    virtual void show_incoming_call(const std::string& room_id, const std::string& caller_name, bool is_video) = 0;
    virtual void show_screen_selection(const std::vector<ScreenCapturer::Screen>& screens) = 0;

    virtual void refresh() = 0;
    virtual void redraw() = 0;
    virtual void set_title(const std::string& title) = 0;

    virtual void set_theme(const std::string& theme) = 0;
    virtual void set_font_size(int size) = 0;
    virtual void show_help() = 0;

protected:
    InputHandler input_handler_;
    CommandHandler command_handler_;
    RoomSelectHandler room_select_handler_;
    QuitHandler quit_handler_;
    CallHandler call_handler_;
    VoiceMessageHandler voice_message_handler_;
    VideoMessageHandler video_message_handler_;
    ScreenShareHandler screen_share_handler_;
};