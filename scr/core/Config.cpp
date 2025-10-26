#include "core/Config.h"
#include "utils/StringUtils.h"
#include "utils/JsonParser.h"
#include "utils/Logger.h"
#include <fstream>
#include <filesystem>

Config& Config::get_instance() {
    static Config instance;
    return instance;
}

Config::Config() {
    set_defaults();
}

bool Config::load_from_file(const std::string& file_path) {
    std::string path = file_path.empty() ? get_config_path() : file_path;
    config_file_path_ = path;

    std::filesystem::path fs_path(path);
    std::filesystem::create_directories(fs_path.parent_path());

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARNING("Config file not found, using defaults: " + path);
        return false;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        JsonValue json = JsonValue::parse(content);

        if (json.has_key("ui")) {
            auto ui = json["ui"];
            data_["ui"]["theme"] = ui["theme"].as_string("dark");
            data_["ui"]["font_size"] = ui["font_size"].as_string("12");
            data_["ui"]["notifications"] = ui["notifications"].as_string("true");
        }

        if (json.has_key("protocols")) {
            auto protocols = json["protocols"];
            for (const auto& protocol_name : protocols.get_keys()) {
                auto protocol = protocols[protocol_name];
                for (const auto& key : protocol.get_keys()) {
                    data_["protocols"][protocol_name + "." + key] = protocol[key].as_string();
                }
            }
        }

        if (json.has_key("database")) {
            auto database = json["database"];
            data_["database"]["path"] = database["path"].as_string();
        }

        if (json.has_key("network")) {
            auto network = json["network"];
            data_["network"]["proxy"] = network["proxy"].as_string();
        }

        if (json.has_key("app")) {
            auto app = json["app"];
            data_["app"]["auto_connect"] = app["auto_connect"].as_string("false");
            data_["app"]["message_history_days"] = app["message_history_days"].as_string("30");
        }

        if (json.has_key("keybindings")) {
            auto keybindings = json["keybindings"];
            for (const auto& action : keybindings.get_keys()) {
                data_["keybindings"][action] = keybindings[action].as_string();
            }
        }

        LOG_INFO("Config loaded from: " + path);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file: " + std::string(e.what()));
        return false;
    }
}

bool Config::save_to_file(const std::string& file_path) const {
    std::string path = file_path.empty() ? config_file_path_ : file_path;
    if (path.empty()) {
        path = get_config_path();
    }

    try {
        JsonValue json = JsonValue::object();

        JsonValue ui = JsonValue::object();
        ui["theme"] = get_theme();
        ui["font_size"] = StringUtils::from_int(get_font_size());
        ui["notifications"] = get_notifications_enabled() ? "true" : "false";
        json["ui"] = ui;

        JsonValue protocols = JsonValue::object();
        std::map<std::string, JsonValue> protocol_data;

        for (const auto& [key, value] : data_) {
            if (key == "protocols") {
                for (const auto& [subkey, subvalue] : value) {
                    size_t dot_pos = subkey.find('.');
                    if (dot_pos != std::string::npos) {
                        std::string protocol_name = subkey.substr(0, dot_pos);
                        std::string setting_name = subkey.substr(dot_pos + 1);

                        if (!protocol_data.count(protocol_name)) {
                            protocol_data[protocol_name] = JsonValue::object();
                        }
                        protocol_data[protocol_name][setting_name] = subvalue;
                    }
                }
            }
        }

        for (const auto& [protocol_name, protocol_obj] : protocol_data) {
            protocols[protocol_name] = protocol_obj;
        }
        json["protocols"] = protocols;

        JsonValue database = JsonValue::object();
        database["path"] = get_database_path();
        json["database"] = database;

        JsonValue network = JsonValue::object();
        network["proxy"] = get_proxy_settings();
        json["network"] = network;

        JsonValue app = JsonValue::object();
        app["auto_connect"] = get_auto_connect() ? "true" : "false";
        app["message_history_days"] = StringUtils::from_int(get_message_history_days());
        json["app"] = app;
        JsonValue keybindings = JsonValue::object();
        for (const auto& [action, keys] : data_.at("keybindings")) {
            keybindings[action] = keys;
        }
        json["keybindings"] = keybindings;

        std::ofstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open config file for writing: " + path);
            return false;
        }

        file << json.to_string(true);
        file.close();

        LOG_INFO("Config saved to: " + path);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save config: " + std::string(e.what()));
        return false;
    }
}

void Config::set_theme(const std::string& theme) {
    data_["ui"]["theme"] = theme;
}

std::string Config::get_theme() const {
    auto it = data_.find("ui");
    if (it != data_.end()) {
        auto it2 = it->second.find("theme");
        if (it2 != it->second.end()) {
            return it2->second;
        }
    }
    return "dark";
}

void Config::set_font_size(int size) {
    data_["ui"]["font_size"] = StringUtils::from_int(size);
}

int Config::get_font_size() const {
    return StringUtils::to_int(get_setting("ui", "font_size", "12"));
}

void Config::set_notifications_enabled(bool enabled) {
    data_["ui"]["notifications"] = enabled ? "true" : "false";
}

bool Config::get_notifications_enabled() const {
    return StringUtils::to_bool(get_setting("ui", "notifications", "true"));
}

void Config::set_protocol_config(const std::string& protocol,
                               const std::string& key,
                               const std::string& value) {
    data_["protocols"][protocol + "." + key] = value;
}

std::string Config::get_protocol_config(const std::string& protocol,
                                      const std::string& key,
                                      const std::string& default_value) const {
    return get_setting("protocols", protocol + "." + key, default_value);
}

void Config::set_database_path(const std::string& path) {
    data_["database"]["path"] = path;
}

std::string Config::get_database_path() const {
    return get_setting("database", "path",
                      std::filesystem::path(std::getenv("HOME")) / ".unified-messenger" / "messages.db");
}

void Config::set_proxy_settings(const std::string& proxy) {
    data_["network"]["proxy"] = proxy;
}

std::string Config::get_proxy_settings() const {
    return get_setting("network", "proxy", "");
}

void Config::set_auto_connect(bool auto_connect) {
    data_["app"]["auto_connect"] = auto_connect ? "true" : "false";
}

bool Config::get_auto_connect() const {
    return StringUtils::to_bool(get_setting("app", "auto_connect", "false"));
}

void Config::set_message_history_days(int days) {
    data_["app"]["message_history_days"] = StringUtils::from_int(days);
}

int Config::get_message_history_days() const {
    return StringUtils::to_int(get_setting("app", "message_history_days", "30"));
}

void Config::set_keybinding(const std::string& action, const std::string& keys) {
    data_["keybindings"][action] = keys;
}

std::string Config::get_keybinding(const std::string& action) const {
    return get_setting("keybindings", action, "");
}

void Config::set_defaults() {
    data_["ui"]["theme"] = "dark";
    data_["ui"]["font_size"] = "12";
    data_["ui"]["notifications"] = "true";
    data_["app"]["auto_connect"] = "false";
    data_["app"]["message_history_days"] = "30";
    data_["keybindings"]["send_message"] = "Enter";
    data_["keybindings"]["next_room"] = "Ctrl+N";
    data_["keybindings"]["previous_room"] = "Ctrl+P";
    data_["keybindings"]["quit"] = "Ctrl+Q";
}

std::string Config::get_config_path() const {
    std::string home = std::getenv("HOME");
    return home + "/.config/unified-messenger/config.json";
}

std::string Config::get_setting(const std::string& category,
                              const std::string& key,
                              const std::string& default_value) const {
    auto cat_it = data_.find(category);
    if (cat_it != data_.end()) {
        auto key_it = cat_it->second.find(key);
        if (key_it != cat_it->second.end()) {
            return key_it->second;
        }
    }
    return default_value;
}

bool Config::validate() const {
    errors_.clear();

    int font_size = get_font_size();
    if (font_size < 8 || font_size > 72) {
        errors_.push_back("Font size must be between 8 and 72");
    }

    int history_days = get_message_history_days();
    if (history_days < 1 || history_days > 3650) {
        errors_.push_back("Message history days must be between 1 and 3650");
    }

    return errors_.empty();
}

std::vector<std::string> Config::get_errors() const {
    return errors_;
}

void Config::reset_to_defaults() {
    data_.clear();
    set_defaults();
}