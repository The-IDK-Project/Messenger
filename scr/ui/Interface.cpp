#include "ui/Interface.h"

void Interface::set_input_handler(InputHandler handler) {
    input_handler_ = std::move(handler);
}

void Interface::set_command_handler(CommandHandler handler) {
    command_handler_ = std::move(handler);
}

void Interface::set_room_select_handler(RoomSelectHandler handler) {
    room_select_handler_ = std::move(handler);
}

void Interface::set_quit_handler(QuitHandler handler) {
    quit_handler_ = std::move(handler);
}

void Interface::set_call_handler(CallHandler handler) {
    call_handler_ = std::move(handler);
}

void Interface::set_voice_message_handler(VoiceMessageHandler handler) {
    voice_message_handler_ = std::move(handler);
}

void Interface::set_video_message_handler(VideoMessageHandler handler) {
    video_message_handler_ = std::move(handler);
}

void Interface::set_screen_share_handler(ScreenShareHandler handler) {
    screen_share_handler_ = std::move(handler);
}

void Interface::show_incoming_call(const std::string& room_id, const std::string& caller_name, bool is_video) {
    (void)room_id;
    (void)caller_name;
    (void)is_video;
}

void Interface::show_screen_selection(const std::vector<ScreenCapturer::Screen>& screens) {
    (void)screens;
}
