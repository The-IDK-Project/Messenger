#pragma once

#include "../../core/ChatRoom.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

class RoomListWidget {
public:
    using RoomClickHandler = std::function<void(const ChatRoom& room)>;
    using RoomContextHandler = std::function<void(const ChatRoom& room, int x, int y)>;

    RoomListWidget();
    ~RoomListWidget();

    void add_room(const ChatRoom& room);
    void update_room(const ChatRoom& room);
    void remove_room(const std::string& room_id);
    void clear_rooms();

    void set_rooms(const std::vector<ChatRoom>& rooms);
    std::vector<ChatRoom> get_rooms() const;
    ChatRoom get_room(const std::string& room_id) const;
    bool has_room(const std::string& room_id) const;

    void group_by_protocol(bool enable);
    void set_filter(const std::string& filter);
    void clear_filter();

    void set_visible(bool visible);
    void set_position(int x, int y);
    void set_size(int width, int height);
    void refresh();

    void set_room_click_handler(RoomClickHandler handler);
    void set_room_context_handler(RoomContextHandler handler);

    void set_show_avatars(bool show);
    void set_show_unread_count(bool show);
    void set_compact_mode(bool compact);
    void set_theme(const std::string& theme);

    void sort_by_name();
    void sort_by_activity();
    void set_custom_sort(std::function<bool(const ChatRoom&, const ChatRoom&)> comparator);

    void search(const std::string& query);
    void clear_search();

    bool is_visible() const;
    std::string get_selected_room_id() const;
    int get_visible_room_count() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};