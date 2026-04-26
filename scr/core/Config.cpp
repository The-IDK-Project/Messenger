#include "core/Config.h"
#include "utils/StringUtils.h"
#include "utils/Logger.h"
#include <fstream>
#include <filesystem>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace {

struct LuaValue {
    enum class Type {
        String,
        Number,
        Boolean,
        Table,
    };

    Type type = Type::String;
    std::string scalar;
    std::map<std::string, LuaValue> table;
};

bool is_lua_identifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(value.front());
    if (!(std::isalpha(first) || value.front() == '_')) {
        return false;
    }

    for (const char ch : value) {
        const unsigned char current = static_cast<unsigned char>(ch);
        if (!(std::isalnum(current) || ch == '_')) {
            return false;
        }
    }

    return true;
}

std::string escape_lua_string(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (const char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }

    return escaped;
}

bool looks_like_number(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    size_t index = 0;
    if (value[index] == '-' || value[index] == '+') {
        ++index;
    }

    bool has_digits = false;
    bool has_dot = false;
    for (; index < value.size(); ++index) {
        const char ch = value[index];
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            has_digits = true;
            continue;
        }

        if (ch == '.' && !has_dot) {
            has_dot = true;
            continue;
        }

        return false;
    }

    return has_digits;
}

std::string format_lua_scalar(const std::string& value) {
    const std::string normalized = StringUtils::to_lower(StringUtils::trim(value));
    if (normalized == "true" || normalized == "false") {
        return normalized;
    }

    if (looks_like_number(value)) {
        return value;
    }

    return "\"" + escape_lua_string(value) + "\"";
}

std::string format_lua_key(const std::string& key) {
    if (is_lua_identifier(key)) {
        return key;
    }

    return "[\"" + escape_lua_string(key) + "\"]";
}

std::string get_home_directory() {
    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return home;
    }

    const char* user_profile = std::getenv("USERPROFILE");
    if (user_profile != nullptr && *user_profile != '\0') {
        return user_profile;
    }

    const char* home_drive = std::getenv("HOMEDRIVE");
    const char* home_path = std::getenv("HOMEPATH");
    if (home_drive != nullptr && home_path != nullptr) {
        return std::string(home_drive) + home_path;
    }

    return std::filesystem::current_path().string();
}

class LuaTableParser {
public:
    explicit LuaTableParser(std::string source) : source_(std::move(source)) {}

    LuaValue parse() {
        skip_space_and_comments();
        if (starts_with("return")) {
            position_ += 6;
        }

        skip_space_and_comments();
        LuaValue root = parse_table();
        skip_space_and_comments();

        if (position_ != source_.size()) {
            throw std::runtime_error("Unexpected trailing data in Lua config");
        }

        return root;
    }

private:
    bool starts_with(const std::string& token) const {
        return source_.compare(position_, token.size(), token) == 0;
    }

    void skip_space_and_comments() {
        while (position_ < source_.size()) {
            const char current = source_[position_];
            if (std::isspace(static_cast<unsigned char>(current))) {
                ++position_;
                continue;
            }

            if (current == '-' && position_ + 1 < source_.size() && source_[position_ + 1] == '-') {
                position_ += 2;
                while (position_ < source_.size() && source_[position_] != '\n') {
                    ++position_;
                }
                continue;
            }

            break;
        }
    }

    char peek() const {
        if (position_ >= source_.size()) {
            return '\0';
        }
        return source_[position_];
    }

    char consume() {
        if (position_ >= source_.size()) {
            throw std::runtime_error("Unexpected end of Lua config");
        }
        return source_[position_++];
    }

    void expect(char token) {
        skip_space_and_comments();
        if (consume() != token) {
            throw std::runtime_error(std::string("Expected '") + token + "' in Lua config");
        }
    }

    std::string parse_identifier() {
        skip_space_and_comments();
        const char current = peek();
        if (!(std::isalpha(static_cast<unsigned char>(current)) || current == '_')) {
            throw std::runtime_error("Expected identifier in Lua config");
        }

        const size_t start = position_;
        consume();
        while (position_ < source_.size()) {
            const char current = source_[position_];
            if (!std::isalnum(static_cast<unsigned char>(current)) && current != '_') {
                break;
            }
            ++position_;
        }

        return source_.substr(start, position_ - start);
    }

    std::string parse_string() {
        skip_space_and_comments();
        const char quote = consume();
        if (quote != '"' && quote != '\'') {
            throw std::runtime_error("Expected string in Lua config");
        }

        std::string result;
        while (position_ < source_.size()) {
            const char current = consume();
            if (current == quote) {
                return result;
            }

            if (current == '\\') {
                const char escaped = consume();
                switch (escaped) {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    case '\'': result += '\''; break;
                    default: result += escaped; break;
                }
                continue;
            }

            result += current;
        }

        throw std::runtime_error("Unterminated string in Lua config");
    }

    LuaValue parse_value() {
        skip_space_and_comments();
        const char current = peek();

        if (current == '{') {
            return parse_table();
        }

        if (current == '"' || current == '\'') {
            return LuaValue{LuaValue::Type::String, parse_string(), {}};
        }

        if (std::isdigit(static_cast<unsigned char>(current)) || current == '-' || current == '+') {
            return LuaValue{LuaValue::Type::Number, parse_bare_token(), {}};
        }

        const std::string token = parse_bare_token();
        if (token == "true" || token == "false") {
            return LuaValue{LuaValue::Type::Boolean, token, {}};
        }

        return LuaValue{LuaValue::Type::String, token, {}};
    }

    std::string parse_bare_token() {
        skip_space_and_comments();
        const size_t start = position_;
        while (position_ < source_.size()) {
            const char current = source_[position_];
            if (std::isspace(static_cast<unsigned char>(current)) || current == ',' || current == '}') {
                break;
            }
            ++position_;
        }

        if (start == position_) {
            throw std::runtime_error("Expected value in Lua config");
        }

        return source_.substr(start, position_ - start);
    }

    LuaValue parse_table() {
        expect('{');

        LuaValue table_value;
        table_value.type = LuaValue::Type::Table;

        while (true) {
            skip_space_and_comments();
            if (peek() == '}') {
                consume();
                return table_value;
            }

            std::string key;
            if (peek() == '[') {
                consume();
                key = parse_string();
                expect(']');
            } else if (peek() == '"' || peek() == '\'') {
                key = parse_string();
            } else {
                key = parse_identifier();
            }

            expect('=');
            table_value.table[key] = parse_value();

            skip_space_and_comments();
            if (peek() == ',') {
                consume();
            }
        }
    }

    std::string source_;
    size_t position_ = 0;
};

void read_string_value(const LuaValue& table,
                      const std::string& key,
                      const std::string& default_value,
                      std::map<std::string, std::string>& target,
                      const std::string& target_key) {
    const auto it = table.table.find(key);
    if (it == table.table.end()) {
        return;
    }

    target[target_key] = it->second.scalar.empty() ? default_value : it->second.scalar;
}

const LuaValue* get_table_value(const LuaValue& table, const std::string& key) {
    const auto it = table.table.find(key);
    if (it == table.table.end() || it->second.type != LuaValue::Type::Table) {
        return nullptr;
    }

    return &it->second;
}

} // namespace

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
    if (fs_path.has_parent_path()) {
        std::filesystem::create_directories(fs_path.parent_path());
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARNING("Config file not found, using defaults: " + path);
        return false;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        const LuaValue root = LuaTableParser(content).parse();

        if (const LuaValue* ui = get_table_value(root, "ui")) {
            read_string_value(*ui, "theme", "dark", data_["ui"], "theme");
            read_string_value(*ui, "font_size", "12", data_["ui"], "font_size");
            read_string_value(*ui, "notifications", "true", data_["ui"], "notifications");
        }

        if (const LuaValue* protocols = get_table_value(root, "protocols")) {
            for (const auto& [protocol_name, protocol_value] : protocols->table) {
                if (protocol_value.type != LuaValue::Type::Table) {
                    continue;
                }

                for (const auto& [key, value] : protocol_value.table) {
                    data_["protocols"][protocol_name + "." + key] = value.scalar;
                }
            }
        }

        if (const LuaValue* database = get_table_value(root, "database")) {
            read_string_value(*database, "path", "", data_["database"], "path");
        }

        if (const LuaValue* network = get_table_value(root, "network")) {
            read_string_value(*network, "proxy", "", data_["network"], "proxy");
        }

        if (const LuaValue* app = get_table_value(root, "app")) {
            read_string_value(*app, "auto_connect", "false", data_["app"], "auto_connect");
            read_string_value(*app, "message_history_days", "30", data_["app"], "message_history_days");
        }

        if (const LuaValue* keybindings = get_table_value(root, "keybindings")) {
            for (const auto& [action, value] : keybindings->table) {
                data_["keybindings"][action] = value.scalar;
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
        std::filesystem::path fs_path(path);
        if (fs_path.has_parent_path()) {
            std::filesystem::create_directories(fs_path.parent_path());
        }

        std::ostringstream lua;
        lua << "return {\n";
        lua << "  ui = {\n";
        lua << "    theme = " << format_lua_scalar(get_theme()) << ",\n";
        lua << "    font_size = " << get_font_size() << ",\n";
        lua << "    notifications = " << (get_notifications_enabled() ? "true" : "false") << ",\n";
        lua << "  },\n";
        lua << "  protocols = {\n";

        std::map<std::string, std::map<std::string, std::string>> protocol_data;
        const auto protocols_it = data_.find("protocols");
        if (protocols_it != data_.end()) {
            for (const auto& [subkey, subvalue] : protocols_it->second) {
                const size_t dot_pos = subkey.find('.');
                if (dot_pos == std::string::npos) {
                    continue;
                }

                const std::string protocol_name = subkey.substr(0, dot_pos);
                const std::string setting_name = subkey.substr(dot_pos + 1);
                protocol_data[protocol_name][setting_name] = subvalue;
            }
        }

        for (const auto& [protocol_name, settings] : protocol_data) {
            lua << "    " << format_lua_key(protocol_name) << " = {\n";
            for (const auto& [setting_name, setting_value] : settings) {
                lua << "      " << format_lua_key(setting_name) << " = " << format_lua_scalar(setting_value) << ",\n";
            }
            lua << "    },\n";
        }

        lua << "  },\n";
        lua << "  database = {\n";
        lua << "    path = " << format_lua_scalar(get_database_path()) << ",\n";
        lua << "  },\n";
        lua << "  network = {\n";
        lua << "    proxy = " << format_lua_scalar(get_proxy_settings()) << ",\n";
        lua << "  },\n";
        lua << "  app = {\n";
        lua << "    auto_connect = " << (get_auto_connect() ? "true" : "false") << ",\n";
        lua << "    message_history_days = " << get_message_history_days() << ",\n";
        lua << "  },\n";
        lua << "  keybindings = {\n";

        const auto keybindings_it = data_.find("keybindings");
        if (keybindings_it != data_.end()) {
            for (const auto& [action, keys] : keybindings_it->second) {
                lua << "    " << format_lua_key(action) << " = " << format_lua_scalar(keys) << ",\n";
            }
        }

        lua << "  },\n";
        lua << "}\n";

        std::ofstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open config file for writing: " + path);
            return false;
        }

        file << lua.str();
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
                      (std::filesystem::path(get_home_directory()) / ".unified-messenger" / "messages.db").string());
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
    return (std::filesystem::path(get_home_directory()) / ".config" / "unified-messenger" / "config.lua").string();
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
