project('UnifiedMessenger', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++17'])

# ============================================================================
# Options
# ============================================================================
add_project_arguments('-DPROJECT_SOURCE_DIR="' + meson.project_source_root() + '"', language: 'cpp')

# ============================================================================
# Dependencies
# ============================================================================
thread_dep = dependency('threads')
jsoncpp_dep = dependency('jsoncpp', fallback: ['jsoncpp', 'jsoncpp_dep'])
sqlite3_dep = dependency('sqlite3', fallback: ['sqlite3', 'sqlite3_dep'])

# Qt6
qt6_dep = dependency('qt6', modules: ['Core', 'Widgets', 'Network'], required: get_option('build_gui'))

# ncurses for TUI
ncurses_dep = dependency('ncurses', required: get_option('build_tui') and host_machine.system() != 'windows')


# ============================================================================
# Source files
# ============================================================================
core_sources = files(
  'scr/core/Message.cpp',
  'scr/core/User.cpp',
  'scr/core/ChatRoom.cpp',
  'scr/core/Config.cpp',
  'scr/utils/Logger.cpp',
  'scr/utils/StringUtils.cpp',
  'scr/database/DatabaseManager.cpp',
  'scr/database/Migrations.cpp',
  'scr/network/Socket.cpp',
  'scr/protocols/ProtocolHandler.cpp',
  'scr/protocols/ProtocolFactory.cpp',
  'scr/app/UnifiedMessenger.cpp'
)

gui_sources = files(
  'scr/ui/GUI.cpp',
  'scr/ui/widgets/ChatWidget.cpp',
  'scr/ui/widgets/RoomListWidget.cpp',
  'scr/ui/Interface.cpp'
)

tui_sources = files(
  'scr/ui/TUI.cpp',
  'scr/ui/Interface.cpp'
)

main_source = files('scr/main.cpp')

# ============================================================================
# Core
# ============================================================================
core_lib = static_library('UnifiedMessengerCore',
  core_sources,
  include_directories: 'include',
  dependencies: [thread_dep, jsoncpp_dep, sqlite3_dep])

# ============================================================================
# GUI Qt6
# ============================================================================
if get_option('build_gui')
  executable('UnifiedMessenger_GUI',
    main_source,
    gui_sources,
    dependencies: [core_lib, qt6_dep],
    cpp_args: ['-DUSE_GUI'],
    link_with: core_lib)
endif

# ============================================================================
# TUI
# ============================================================================
if get_option('build_tui')
  tui_deps = [core_lib]
  if host_machine.system() != 'windows'
    tui_deps += ncurses_dep
  endif

  executable('UnifiedMessenger_TUI',
    main_source,
    tui_sources,
    dependencies: tui_deps,
    cpp_args: ['-DUSE_TUI'],
    link_with: core_lib)
endif
