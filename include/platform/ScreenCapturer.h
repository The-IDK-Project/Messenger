#pragma once

#include <vector>
#include <cstdint>
#include <string>

class ScreenCapturer {
public:
    struct Screen {
        int id;
        std::string name;
    };

    struct Image {
        int width;
        int height;
        std::vector<uint8_t> data;
    };

    static std::vector<Screen> get_screens();
    static Image capture_screen(int screen_id);
};
