cmake_minimum_required(VERSION 3.16)
project(UnifiedMessenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_GUI "Build with Qt GUI" ON)
option(BUILD_TUI "Build with terminal UI" OFF)
option(ENABLE_TELEGRAM "Enable Telegram support" OFF)
option(USE_BUNDLED_DEPS "Use bundled third-party dependencies" OFF)

# ============================================================================
# vcpkg
# ============================================================================
include_directories(${CMAKE_SOURCE_DIR}/include)
find_package(Threads REQUIRED)

if(NOT USE_BUNDLED_DEPS)
    find_package(jsoncpp CONFIG REQUIRED)
    find_package(SQLite3 REQUIRED)

    set(JSONCPP_LIBRARIES jsoncpp_lib)
    set(SQLite3_LIBRARIES SQLite::SQLite3)
endif()

if(BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
endif()

# ============================================================================
# Core
# ============================================================================
file(GLOB_RECURSE CORE_SOURCES "scr/core/*.cpp" "scr/utils/*.cpp" "scr/database/*.cpp" "scr/network/*.cpp" "scr/protocols/*.cpp" "scr/app/*.cpp")
add_library(UnifiedMessengerCore STATIC ${CORE_SOURCES})
target_include_directories(UnifiedMessengerCore PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(UnifiedMessengerCore PUBLIC ${JSONCPP_LIBRARIES} ${SQLite3_LIBRARIES} Threads::Threads)

# ============================================================================
# GUI Qt6
# ============================================================================
if(BUILD_GUI)
    file(GLOB_RECURSE GUI_SOURCES "scr/ui/GUI.cpp" "scr/ui/widgets/*.cpp" "scr/ui/Interface.cpp")
    add_executable(UnifiedMessenger_GUI scr/main.cpp ${GUI_SOURCES})

    set_target_properties(UnifiedMessenger_GUI PROPERTIES
            AUTOMOC ON
            AUTOUIC ON
            AUTORCC ON
    )

    target_compile_definitions(UnifiedMessenger_GUI PRIVATE USE_GUI)

    target_link_libraries(UnifiedMessenger_GUI PRIVATE
            UnifiedMessengerCore
            Qt6::Core
            Qt6::Widgets
            Qt6::Network
    )
endif()

# ============================================================================
# TUI
# ============================================================================
if(BUILD_TUI)
    file(GLOB_RECURSE TUI_SOURCES "scr/ui/TUI.cpp" "scr/ui/Interface.cpp")
    add_executable(UnifiedMessenger_TUI scr/main.cpp ${TUI_SOURCES})
    target_compile_definitions(UnifiedMessenger_TUI PRIVATE USE_TUI)
    target_link_libraries(UnifiedMessenger_TUI PRIVATE UnifiedMessengerCore)
    if(NOT WIN32)
        target_link_libraries(UnifiedMessenger_TUI PRIVATE ${NCURSES_LIBRARIES})
    endif()
endif()