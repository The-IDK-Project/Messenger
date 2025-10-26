cmake_minimum_required(VERSION 3.15)
project(UnifiedMessenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_GUI "Build with Qt GUI" OFF)
option(BUILD_TUI "Build with terminal UI" ON)
option(ENABLE_TELEGRAM "Enable Telegram support" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(INSTALL_APP "Create install target" ON)
option(USE_BUNDLED_DEPS "Use bundled third-party dependencies" OFF)
option(BUILD_TELEGRAM_DEPS "Build Telegram dependencies" OFF)

# ============================================================================
# Third-party dependencies configuration
# ============================================================================

if(USE_BUNDLED_DEPS)
    message(STATUS "Using bundled third-party dependencies")

    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/json")
        message(STATUS "Using bundled nlohmann/json")
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE
                ${CMAKE_SOURCE_DIR}/third_party/json/include)
        set(JSONCPP_LIBRARIES nlohmann_json)
        set(JSONCPP_FOUND TRUE)
    else()
        message(WARNING "Bundled nlohmann/json not found")
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/build_third_party/lib/libsqlite3.a")
        message(STATUS "Using bundled SQLite3")
        add_library(sqlite3_bundled STATIC IMPORTED)
        set_target_properties(sqlite3_bundled PROPERTIES
                IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/build_third_party/lib/libsqlite3.a
                INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/build_third_party/include
        )
        set(SQLite3_LIBRARIES sqlite3_bundled)
        set(SQLite3_FOUND TRUE)
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/build_third_party/lib/sqlite3.lib")
        message(STATUS "Using bundled SQLite3 (Windows)")
        add_library(sqlite3_bundled STATIC IMPORTED)
        set_target_properties(sqlite3_bundled PROPERTIES
                IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/build_third_party/lib/sqlite3.lib
                INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/build_third_party/include
        )
        set(SQLite3_LIBRARIES sqlite3_bundled)
        set(SQLite3_FOUND TRUE)
    else()
        message(WARNING "Bundled SQLite3 not found")
    endif()

    if(BUILD_TELEGRAM_DEPS AND ENABLE_TELEGRAM)
        if(EXISTS "${CMAKE_SOURCE_DIR}/build_third_party/lib/libtdjson.so")
            message(STATUS "Using bundled TDLib")
            add_library(tdjson_bundled SHARED IMPORTED)
            set_target_properties(tdjson_bundled PROPERTIES
                    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/build_third_party/lib/libtdjson.so
                    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/build_third_party/include
            )
            set(TDLIB_LIBRARIES tdjson_bundled)
            set(TDLIB_FOUND TRUE)
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/build_third_party/bin/tdjson.dll")
            message(STATUS "Using bundled TDLib (Windows)")
            add_library(tdjson_bundled SHARED IMPORTED)
            set_target_properties(tdjson_bundled PROPERTIES
                    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/build_third_party/bin/tdjson.dll
                    IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/build_third_party/lib/tdjson.lib
                    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/build_third_party/include
            )
            set(TDLIB_LIBRARIES tdjson_bundled)
            set(TDLIB_FOUND TRUE)
        else()
            message(WARNING "Bundled TDLib not found")
        endif()
    endif()

else()
    message(STATUS "Using system dependencies")
endif()

# ============================================================================
# Find system packages (fallback if not using bundled deps)
# ============================================================================

include_directories(include)
find_package(PkgConfig REQUIRED)

if(NOT TARGET nlohmann_json)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
    if(NOT JSONCPP_FOUND)
        message(FATAL_ERROR "jsoncpp not found")
    endif()
endif()

if(NOT SQLite3_FOUND)
    pkg_check_modules(SQLite3 REQUIRED sqlite3)
    if(NOT SQLite3_FOUND)
        message(FATAL_ERROR "SQLite3 not found")
    endif()
endif()

if(BUILD_TUI AND NOT WIN32)
    pkg_check_modules(NCURSES REQUIRED ncurses)
    if(NOT NCURSES_FOUND)
        message(FATAL_ERROR "ncurses not found")
    endif()
endif()

if(BUILD_GUI)
    find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)
endif()

find_package(Threads REQUIRED)
