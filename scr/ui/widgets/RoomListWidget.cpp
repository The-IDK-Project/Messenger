#include "ui/widgets/RoomListWidget.h"
#include "utils/Logger.h"
#include <format>

// PImpl
class RoomListWidget::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    std::vector<ChatRoom> rooms;
    RoomClickHandler room_click_handler;
    RoomContextHandler room_context_handler;
    std::string selected_room_id;
};

RoomListWidget::RoomListWidget() : impl_(std::make_unique<Impl>()) {
    LOG_INFO("RoomListWidget created");
}

RoomListWidget::~RoomListWidget() {
    LOG_INFO("RoomListWidget destroyed");
}

void RoomListWidget::add_room(const ChatRoom& room) {
    impl_->rooms.push_back(room);
    LOG_DEBUG(std::format("Added room: {}", room.name));
}

void RoomListWidget::update_room(const ChatRoom& room) {
    for (auto& r : impl_->rooms) {
        if (r.id == room.id) {
            r = room;
            LOG_DEBUG(std::format("Updated room: {}", room.name));
            return;
        }
    }
}

void RoomListWidget::remove_room(const std::string& room_id) {
    std::erase_if(impl_->rooms, [&](const ChatRoom& r) { return r.id == room_id; });
    LOG_DEBUG(std::format("Removed room: {}", room_id));
}

void RoomListWidget::clear_rooms() {
    impl_->rooms.clear();
    LOG_DEBUG("Cleared all rooms");
}

void RoomListWidget::set_rooms(const std::vector<ChatRoom>& rooms) {
    impl_->rooms = rooms;
}

std::vector<ChatRoom> RoomListWidget::get_rooms() const {
    return impl_->rooms;
}

ChatRoom RoomListWidget::get_room(const std::string& room_id) const {
    for (const auto& r : impl_->rooms) {
        if (r.id == room_id) {
            return r;
        }
    }
    return ChatRoom{};
}

bool RoomListWidget::has_room(const std::string& room_id) const {
    for (const auto& r : impl_->rooms) {
        if (r.id == room_id) {
            return true;
        }
    }
    return false;
}

void RoomListWidget::set_room_click_handler(RoomClickHandler handler) {
    impl_->room_click_handler = handler;
}

void RoomListWidget::set_room_context_handler(RoomContextHandler handler) {
    impl_->room_context_handler = handler;
}

std::string RoomListWidget::get_selected_room_id() const {
    return impl_->selected_room_id;
}

// Other methods are just stubs for now
void RoomListWidget::group_by_protocol(bool enable) { LOG_DEBUG("group_by_protocol"); }
void RoomListWidget::set_filter(const std::string& filter) { LOG_DEBUG("set_filter"); }
void RoomListWidget::clear_filter() { LOG_DEBUG("clear_filter"); }
void RoomListWidget::set_visible(bool visible) { LOG_DEBUG("set_visible"); }
void RoomListWidget::set_position(int x, int y) { LOG_DEBUG("set_position"); }
void RoomListWidget::set_size(int width, int height) { LOG_DEBUG("set_size"); }
void RoomListWidget::refresh() { LOG_DEBUG("refresh"); }
void RoomListWidget::set_show_avatars(bool show) { LOG_DEBUG("set_show_avatars"); }
void RoomListWidget::set_show_unread_count(bool show) { LOG_DEBUG("set_show_unread_count"); }
void RoomListWidget::set_compact_mode(bool compact) { LOG_DEBUG("set_compact_mode"); }
void RoomListWidget::set_theme(const std::string& theme) { LOG_DEBUG("set_theme"); }
void RoomListWidget::sort_by_name() { LOG_DEBUG("sort_by_name"); }
void RoomListWidget::sort_by_activity() { LOG_DEBUG("sort_by_activity"); }
void RoomListWidget::set_custom_sort(std::function<bool(const ChatRoom&, const ChatRoom&)> comparator) { LOG_DEBUG("set_custom_sort"); }
void RoomListWidget::search(const std::string& query) { LOG_DEBUG("search"); }
void RoomListWidget::clear_search() { LOG_DEBUG("clear_search"); }
bool RoomListWidget::is_visible() const { return true; }
int RoomListWidget::get_visible_room_count() const { return impl_->rooms.size(); }
