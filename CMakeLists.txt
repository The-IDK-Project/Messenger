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

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

include_directories(include)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCURL REQUIRED libcurl)
if(NOT LIBCURL_FOUND)
    message(FATAL_ERROR "libcurl not found")
endif()
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
if(NOT JSONCPP_FOUND)
    message(FATAL_ERROR "jsoncpp not found")
endif()

pkg_check_modules(SQLite3 REQUIRED sqlite3)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 not found")
endif()

if(BUILD_TUI)
    pkg_check_modules(NCURSES REQUIRED ncurses)
    if(NOT NCURSES_FOUND)
        message(FATAL_ERROR "ncurses not found")
    endif()
endif()

if(BUILD_GUI)
    find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)
endif()

find_package(Threads REQUIRED)

set(SOURCES
        src/main.cpp
        src/core/Message.cpp
        src/core/User.cpp
        src/core/ChatRoom.cpp
        src/core/Config.cpp
        src/database/DatabaseManager.cpp
        src/database/DatabaseSchema.cpp
        src/utils/Logger.cpp
        src/utils/StringUtils.cpp
        src/utils/JsonParser.cpp
        src/app/UnifiedMessenger.cpp
        src/app/SessionManager.cpp
        src/app/NotificationManager.cpp
)

set(PROTOCOL_SOURCES
        src/protocols/ProtocolHandler.cpp
        src/protocols/MatrixHandler.cpp
        src/protocols/IRCHandler.cpp
)

set(NETWORK_SOURCES
        src/network/HttpClient.cpp
        src/network/WebSocketClient.cpp
        src/network/IRCConnection.cpp
)

set(UI_SOURCES
        src/ui/Interface.cpp
)

if(BUILD_TUI)
    list(APPEND UI_SOURCES src/ui/TUI.cpp src/ui/widgets/ChatWidget.cpp)
endif()

if(BUILD_GUI)
    list(APPEND UI_SOURCES src/ui/GUI.cpp)
    qt5_wrap_cpp(QT_MOC_SOURCES ${UI_SOURCES})
endif()

if(ENABLE_TELEGRAM)
    list(APPEND PROTOCOL_SOURCES src/protocols/TelegramHandler.cpp)
endif()

set(ALL_SOURCES
        ${SOURCES}
        ${PROTOCOL_SOURCES}
        ${NETWORK_SOURCES}
        ${UI_SOURCES}
        ${QT_MOC_SOURCES}
        include/core/ChatRoom.h
        include/core/Config.h
        include/database/Migrations.h
        include/database/DatabaseManager.h
        include/database/DatabaseSchema.h
        include/protocols/ProtocolHandler.h
        include/protocols/MatrixHandler.h
        include/protocols/IRCHandler.h
        include/protocols/TelegramHandler.h
        include/protocols/ProtocolFactory.h
        include/network/HttpClient.h
        include/network/WebSocketClient.h
        include/network/IRCConnection.h
        include/ui/Interface.h
        include/ui/TUI.h
        include/ui/GUI.h
        include/ui/widgets/ChatWidget.h
        include/ui/widgets/ContactListWidget.h
        include/ui/widgets/InputWidget.h
        include/utils/Logger.h
        include/utils/Crypto.h
        include/utils/StringUtils.h
        include/utils/JsonParser.h
)

add_executable(unified-messenger ${ALL_SOURCES})

target_include_directories(unified-messenger PRIVATE include)

target_link_libraries(unified-messenger PRIVATE
        ${LIBCURL_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${SQLite3_LIBRARIES}
        Threads::Threads
)

if(BUILD_TUI)
    target_link_libraries(unified-messenger PRIVATE ${NCURSES_LIBRARIES})
endif()

if(BUILD_GUI)
    target_link_libraries(unified-messenger PRIVATE
            Qt5::Core
            Qt5::Widgets
            Qt5::Network
    )
endif()

target_compile_definitions(unified-messenger PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
        PROJECT_NAME="${PROJECT_NAME}"
)

if(BUILD_TUI)
    target_compile_definitions(unified-messenger PRIVATE ENABLE_TUI=1)
endif()

if(BUILD_GUI)
    target_compile_definitions(unified-messenger PRIVATE ENABLE_GUI=1)
endif()

if(ENABLE_TELEGRAM)
    target_compile_definitions(unified-messenger PRIVATE ENABLE_TELEGRAM=1)
endif()

if(INSTALL_APP)
    install(TARGETS unified-messenger
            RUNTIME DESTINATION bin
            BUNDLE DESTINATION Applications
    )

    install(DIRECTORY config/
            DESTINATION share/${PROJECT_NAME}/config
            FILES_MATCHING PATTERN "*.conf"
            PATTERN "*.theme"
    )

    install(DIRECTORY docs/
            DESTINATION share/${PROJECT_NAME}/docs
    )

    if(UNIX AND NOT APPLE)
        install(FILES resources/unified-messenger.desktop
                DESTINATION share/applications
        )
        install(FILES resources/icons/app_icon.png
                DESTINATION share/pixmaps
                RENAME unified-messenger.png
        )
    endif()
endif()

if(BUILD_TESTS)
    enable_testing()

    find_package(GTest)
    if(GTest_FOUND)
        add_subdirectory(tests)
    else()
        message(WARNING "GTest not found - tests will not be built")
    endif()
endif()

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
        set(DOXYGEN_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/docs)
        doxygen_add_docs(docs ${DOXYGEN_INPUT})
    else()
        message(WARNING "Doxygen not found - documentation will not be generated")
    endif()
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "Unified multi-protocol messenger client")
set(CPACK_PACKAGE_VENDOR "Unified Messenger Team")

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Unified Messenger Team")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4, libjsoncpp-dev, sqlite3, libncurses5")
elseif(APPLE)
    set(CPACK_GENERATOR "Bundle")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
endif()

include(CPack)

message(STATUS "")
message(STATUS "Unified Messenger Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  TUI: ${BUILD_TUI}")
message(STATUS "  GUI: ${BUILD_GUI}")
message(STATUS "  Telegram: ${ENABLE_TELEGRAM}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Install: ${INSTALL_APP}")
message(STATUS "")