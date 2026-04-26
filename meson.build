project('Verita', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++20', 'buildtype=release'])

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
mtxclient_dep = dependency('mtxclient', fallback: ['mtxclient', 'matrix_client_dep'])
inc = include_directories('include')
qt6_core = dependency('Qt6Core', method: 'cmake', required: true)
qt6_gui = dependency('Qt6Gui', method: 'cmake', required: true)
qt6_widgets = dependency('Qt6Widgets', method: 'cmake', required: true)
qt6_network = dependency('Qt6Network', method: 'cmake', required: true)

qt6_dep = [qt6_core, qt6_gui, qt6_widgets, qt6_network]
qt_qmake = find_program('qmake6', 'qmake', required: true)
qt_host_bins = run_command(qt_qmake, '-query', 'QT_HOST_BINS', check: true).stdout().strip()
qt_moc = find_program(join_paths(qt_host_bins, 'moc.exe'), required: true)

# ============================================================================
# Source files
# ============================================================================
core_sources = files(
  'scr/core/Message.cpp',
  'scr/core/User.cpp',
  'scr/core/ChatRoom.cpp',
  'scr/core/Config.cpp',
  'scr/utils/JsonParser.cpp',
  'scr/utils/Logger.cpp',
  'scr/utils/StringUtils.cpp',
  'scr/database/DatabaseManager.cpp',
  'scr/database/Migrations.cpp',
  'scr/protocols/ProtocolFactory.cpp',
  'scr/protocols/ProtocolHandler.cpp',
  'scr/app/Verita.cpp',
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
  'include/ui/GUI.h'
)

qt_moc_gen = generator(
  qt_moc,
  output: 'moc_@BASENAME@.cpp',
  arguments: [
    '@INPUT@',
    '-o',
    '@OUTPUT@',
    '-I' + meson.project_source_root() / 'include',
  ]
)

processed_gui_sources = [
  gui_sources,
  qt_moc_gen.process(gui_headers),
]

main_source = files('scr/main.cpp')
headless_main_source = files('scr/headless_main.cpp')

# ============================================================================
# Core Library
# ============================================================================
UnifiedMessengerCore = static_library('UnifiedMessengerCore',
                                      core_sources,
                                      include_directories: inc,
                                      dependencies: [sqlite3_dep, mtxclient_dep, jsoncpp_dep]
)

# ============================================================================
# GUI Executable (Qt6)
# ============================================================================
executable('UnifiedMessenger_GUI',
           main_source,
           processed_gui_sources,
           include_directories: inc,
           cpp_args: ['-DUSE_GUI'],
           dependencies: [qt6_dep, sqlite3_dep],
           link_with: UnifiedMessengerCore,
           install: true
)

executable('VeritaHeadless',
           headless_main_source,
           include_directories: inc,
           dependencies: [sqlite3_dep],
           link_with: UnifiedMessengerCore,
           install: true
)
