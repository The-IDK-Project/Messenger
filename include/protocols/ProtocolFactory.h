#pragma once

#include "ProtocolHandler.h"
#include <memory>
#include <string>
#include <map>
#include <functional>

class ProtocolFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<ProtocolHandler>()>;

    static ProtocolFactory& get_instance();

    bool register_protocol(const std::string& name, CreatorFunc creator);
    bool unregister_protocol(const std::string& name);

    std::unique_ptr<ProtocolHandler> create_protocol(const std::string& name);
    std::vector<std::string> get_available_protocols() const;
    bool is_protocol_available(const std::string& name) const;

    std::map<std::string, std::unique_ptr<ProtocolHandler>>
    create_all_protocols();

    std::unique_ptr<ProtocolHandler> create_from_config(const std::string& name,
                                                       const std::map<std::string, std::string>& config);

    void register_default_protocols();

private:
    ProtocolFactory() = default;
    void register_matrix();
    void register_irc();
    void register_telegram();

    std::map<std::string, CreatorFunc> creators_;
};