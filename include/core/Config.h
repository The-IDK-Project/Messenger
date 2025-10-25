#pragma once

#include <string>
#include <map>
#include <vector>

class Config {
public:

    static Config& get_instance();

    bool load_from_file(const std::string& file_path = "");
    bool save_to_file(const std::string& file_path = "") const;

    void set_theme(const std::string& theme);
    std::string get_theme() const;

    void set_font_size(int size);
    int get_font_size() const;

    void set_notifications_enabled(bool enabled);
    bool get_notifications_enabled() const;

    void set_protocol_config(const std::string& protocol,
                           const std::string& key,
                           const std::string& value);
    std::string get_protocol_config(const std::string& protocol,
                                  const std::string& key,
                                  const std::string& default_value = "") const;

    void set_database_path(const std::string& path);
    std::string get_database_path() const;

    void set_proxy_settings(const std::string& proxy);
    std::string get_proxy_settings() const;

    void set_auto_connect(bool auto_connect);
    bool get_auto_connect() const;

    void set_message_history_days(int days);
    int get_message_history_days() const;

    void set_keybinding(const std::string& action, const std::string& keys);
    std::string get_keybinding(const std::string& action) const;

    bool validate() const;
    std::vector<std::string> get_errors() const;

    void reset_to_defaults();

private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string config_file_path_;
    std::map<std::string, std::map<std::string, std::string>> data_;
    mutable std::vector<std::string> errors_;

    void set_defaults();
    std::string get_config_path() const;
};