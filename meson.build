project('Verita', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++20'])

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
inc = include_directories('include')

# mbedTLS (via CMake subproject)
cmake = import('cmake')
mbedtls_proj = cmake.subproject('mbedtls')
mbedtls_deps = [
  mbedtls_proj.dependency('mbedcrypto'),
  mbedtls_proj.dependency('mbedx509'),
  mbedtls_proj.dependency('mbedtls')
]

# Qt6
qt6 = import('qt6')
qt6_dep = dependency('qt6', modules: ['Core', 'Gui', 'Widgets', 'Network'], required: true)

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

# Qt requires headers with Q_OBJECT to be processed by MOC
# We assume the headers are in the include directory with a similar structure
gui_headers = files(
  'include/ui/GUI.h',
  'include/ui/widgets/ChatWidget.h',
  'include/ui/widgets/RoomListWidget.h',
  'include/ui/Interface.h'
)

processed_gui_sources = qt6.preprocess(
  moc_headers: gui_headers,
  sources: gui_sources
)

main_source = files('scr/main.cpp')

# ============================================================================
# Core Library
# ============================================================================
UnifiedMessengerCore = static_library('UnifiedMessengerCore',
                                      core_sources,
                                      include_directories: inc,
                                      dependencies: [sqlite3_dep] + mbedtls_deps
)

# ============================================================================
# GUI Executable (Qt6)
# ============================================================================
executable('UnifiedMessenger_GUI',
           main_source,
           processed_gui_sources,
           dependencies: [qt6_dep, sqlite3_dep],
           link_with: UnifiedMessengerCore,
           install: true
)
