project('UnifiedMessenger', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++23'])

# ============================================================================
# Options
# ============================================================================
add_project_arguments('-DPROJECT_SOURCE_DIR="' + meson.project_source_root() + '"', language: 'cpp')

# ============================================================================
# Dependencies
# ============================================================================
thread_dep = dependency('threads')
jsoncpp_dep = dependency('jsoncpp', fallback: ['jsoncpp', 'jsoncpp_dep'])
sqlite3_dep = dependency('sqlite3', fallback: ['sqlite3', 'sqlite3_dep'], required: true)
openssl_dep = dependency('openssl')
inc = include_directories('include')

# Qt6
cpp = meson.get_compiler('cpp')

vcpkg_inc = 'D:/.vspgk/vspgk/installed/x64-windows/include'
vcpkg_lib_dir = 'D:/.vspgk/vspgk/installed/x64-windows/lib'

qt6_dep = declare_dependency(
  include_directories: include_directories(vcpkg_inc),
  dependencies: [
    cpp.find_library('Qt6Core', dirs: vcpkg_lib_dir),
    cpp.find_library('Qt6Network', dirs: vcpkg_lib_dir),
    cpp.find_library('Qt6Widgets', dirs: vcpkg_lib_dir),
    cpp.find_library('Qt6Gui', dirs: vcpkg_lib_dir)
  ]
)

# ncurses for TUI (ONLY FOR UNIX Base or Linux)
# ncurses_dep = dependency('ncurses', required: get_option('build_tui') and host_machine.system() != 'windows')


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
  'scr/protocols/ProtocolHandler.cpp',
  'scr/app/UnifiedMessenger.cpp',
  'scr/platform/ScreenCapturer.cpp'
)

gui_sources = files(
  'scr/ui/GUI.cpp',
  'scr/ui/widgets/ChatWidget.cpp',
  'scr/ui/widgets/RoomListWidget.cpp',
  'scr/ui/Interface.cpp'
)

#tui_sources = files(
#  'scr/ui/TUI.cpp',
#  'scr/ui/Interface.cpp'
#)

main_source = files('scr/main.cpp')

# ============================================================================
# Core
# ============================================================================
UnifiedMessengerCore = static_library('UnifiedMessengerCore',
                                      core_sources,
                                      include_directories: include_directories('include'),
                                      dependencies: [sqlite3_dep, openssl_dep]
)
# ============================================================================
# GUI Qt6
# ============================================================================
executable('UnifiedMessenger_GUI',
           'scr/main.cpp',
           dependencies: [qt6_dep, sqlite3_dep],
           link_with: UnifiedMessengerCore
)

# ============================================================================
# TUI
# ============================================================================
# if get_option('build_tui')
#  tui_deps = [core_lib]
#  if host_machine.system() != 'windows'
#    tui_deps += ncurses_dep
#  endif

#  executable('UnifiedMessenger_TUI',
#    main_source,
#    tui_sources,
#    dependencies: tui_deps,
#    cpp_args: ['-DUSE_TUI'],
#    link_with: core_lib)
# endif
