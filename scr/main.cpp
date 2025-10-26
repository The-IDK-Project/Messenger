#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "app/UnifiedMessenger.h"
#include "ui/Interface.h"
#include "ui/TUI.h"
#include "ui/GUI.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"

void show_help(const std::string& program_name) {
    std::cout << "Unified Messenger - Multi-protocol chat client\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --tui, -t          Start with terminal user interface (default)\n";
    std::cout << "  --gui, -g          Start with graphical user interface\n";
    std::cout << "  --help, -h         Show this help message\n";
    std::cout << "  --version, -v      Show version information\n";
    std::cout << "  --config FILE      Use specified configuration file\n";
    std::cout << "  --database FILE    Use specified database file\n";
    std::cout << "  --log-level LEVEL  Set log level (debug, info, warning, error, fatal)\n";
    std::cout << "  --no-log-file      Disable logging to file\n";
    std::cout << "  --auto-connect     Auto-connect to configured protocols\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --tui\n";
    std::cout << "  " << program_name << " --gui --config ~/.config/my-messenger.conf\n";
    std::cout << "  " << program_name << " --log-level debug --auto-connect\n";
}

void show_version() {
    std::cout << "Unified Messenger v1.0.0\n";
    std::cout << "Multi-protocol chat client for Matrix, IRC, and Telegram\n";
    std::cout << "Built on " << __DATE__ << " " << __TIME__ << "\n";
}

struct CommandLineArgs {
    bool use_tui = true;
    bool use_gui = false;
    bool show_help = false;
    bool show_version = false;
    bool auto_connect = false;
    bool no_log_file = false;
    std::string config_file;
    std::string database_file;
    std::string log_level = "info";
};

CommandLineArgs parse_arguments(int argc, char* argv[]) {
    CommandLineArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--tui" || arg == "-t") {
            args.use_tui = true;
            args.use_gui = false;
        } else if (arg == "--gui" || arg == "-g") {
            args.use_tui = false;
            args.use_gui = true;
        } else if (arg == "--help" || arg == "-h") {
            args.show_help = true;
        } else if (arg == "--version" || arg == "-v") {
            args.show_version = true;
        } else if (arg == "--auto-connect") {
            args.auto_connect = true;
        } else if (arg == "--no-log-file") {
            args.no_log_file = true;
        } else if (arg == "--config" && i + 1 < argc) {
            args.config_file = argv[++i];
        } else if (arg == "--database" && i + 1 < argc) {
            args.database_file = argv[++i];
        } else if (arg == "--log-level" && i + 1 < argc) {
            args.log_level = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            exit(1);
        }
    }

    return args;
}

void setup_logging(const CommandLineArgs& args) {
    Logger::get_instance().set_min_level(
        Logger::string_to_level(args.log_level)
    );

    if (!args.no_log_file) {
        std::string log_file;
        if (const char* home = std::getenv("HOME")) {
            log_file = std::string(home) + "/.unified-messenger/logs/app.log";
        } else {
            log_file = "unified-messenger.log";
        }
        Logger::get_instance().set_log_file(log_file);
    }

    LOG_INFO("Unified Messenger starting up...");
    LOG_INFO("Version: 1.0.0");
    LOG_INFO("Log level: " + args.log_level);
    LOG_INFO("Interface: " + std::string(args.use_gui ? "GUI" : "TUI"));
}

std::unique_ptr<Interface> create_interface(const CommandLineArgs& args) {
    if (args.use_gui) {
        LOG_INFO("Creating GUI interface");
        return std::make_unique<GUI>();
    } else {
        LOG_INFO("Creating TUI interface");
        return std::make_unique<TUI>();
    }
}

void setup_callbacks(UnifiedMessenger& app, Interface& ui) {
    app.set_message_callback([&ui](const Message& message) {
        ui.display_message(message);
    });

    app.set_room_callback([&ui](const ChatRoom& room) {

        LOG_DEBUG("New room: " + room.name);
    });

    app.set_status_callback([&ui](const std::string& protocol, bool connected) {
        ui.set_connection_status(protocol, connected);
    });

    app.set_error_callback([&ui](const std::string& error) {
        ui.show_error(error);
    });

    ui.set_input_handler([&app](const std::string& input) {

        LOG_DEBUG("UI input: " + input);

        auto protocols = app.get_available_protocols();
        auto rooms = app.get_all_rooms();

        if (!protocols.empty() && !rooms.empty()) {
            app.send_message(protocols[0], rooms[0].id, input);
        }
    });

    ui.set_room_select_handler([&app](const std::string& room_id) {
        app.set_active_room(room_id);
    });

    ui.set_quit_handler([&app]() {
        LOG_INFO("Quit requested by user");
        app.shutdown();
    });
}

int run_application(const CommandLineArgs& args) {
    LOG_SCOPE("main", "application_run");

    try {

        auto app = std::make_unique<UnifiedMessenger>();

        if (!args.config_file.empty()) {
            if (!app->load_config(args.config_file)) {
                LOG_ERROR("Failed to load configuration from: " + args.config_file);
                return 1;
            }
        } else {
            if (!app->load_config()) {
                LOG_WARNING("Using default configuration");
            }
        }

        if (args.auto_connect) {
            app->set_setting("auto_connect", "true");
        }

        if (!app->initialize()) {
            LOG_ERROR("Failed to initialize application core");
            return 1;
        }

        auto ui = create_interface(args);
        if (!ui->initialize()) {
            LOG_ERROR("Failed to initialize UI");
            return 1;
        }

        setup_callbacks(*app, *ui);

        if (args.auto_connect) {
            app->connect_all();
        }

        ui->set_rooms(app->get_all_rooms());
        ui->set_title("Unified Messenger");

        LOG_INFO("Application initialized successfully");

        int result = ui->run();

        LOG_INFO("Application shutdown complete");
        return result;

    } catch (const std::exception& e) {
        LOG_FATAL("Unhandled exception: " + std::string(e.what()));
        return 1;
    } catch (...) {
        LOG_FATAL("Unknown unhandled exception");
        return 1;
    }
}

int main(int argc, char* argv[]) {
    CommandLineArgs args = parse_arguments(argc, argv);

    if (args.show_help) {
        show_help(argv[0]);
        return 0;
    }

    if (args.show_version) {
        show_version();
        return 0;
    }

    setup_logging(args);

    LOG_DEBUG("Command line arguments parsed:");
    LOG_DEBUG("  UI: " + std::string(args.use_gui ? "GUI" : "TUI"));
    LOG_DEBUG("  Auto-connect: " + std::string(args.auto_connect ? "yes" : "no"));
    LOG_DEBUG("  Config file: " + (args.config_file.empty() ? "default" : args.config_file));
    LOG_DEBUG("  Database file: " + (args.database_file.empty() ? "default" : args.database_file));

    int exit_code = run_application(args);

    // Log shutdown
    if (exit_code == 0) {
        LOG_INFO("Application exited successfully");
    } else {
        LOG_ERROR("Application exited with error code: " + std::to_string(exit_code));
    }
    Logger::get_instance().flush();

    return exit_code;
}