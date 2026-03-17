#include "ui/Interface.h"
#include "utils/Logger.h"

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

void Interface::set_video_circle_handler(VideoCircleHandler handler) {
    video_circle_handler_ = std::move(handler);
}