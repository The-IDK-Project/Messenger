#pragma once

#include <string>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& get_instance();

    void set_log_file(const std::string& filename);
    void set_min_level(LogLevel level);
    void set_console_output(bool enable);
    void set_timestamp_format(const std::string& format);

    void debug(const std::string& message, const std::string& component = "");
    void info(const std::string& message, const std::string& component = "");
    void warning(const std::string& message, const std::string& component = "");
    void error(const std::string& message, const std::string& component = "");
    void fatal(const std::string& message, const std::string& component = "");

    static std::string level_to_string(LogLevel level);
    static LogLevel string_to_level(const std::string& level);
    void flush();

    class ScopedLogger {
    public:
        ScopedLogger(const std::string& component, const std::string& operation);
        ~ScopedLogger();
    private:
        std::string component_;
        std::string operation_;
        std::chrono::steady_clock::time_point start_;
    };

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message, const std::string& component);
    std::string get_timestamp();

    std::ofstream log_file_;
    LogLevel min_level_ = LogLevel::INFO;
    bool console_output_ = true;
    std::string timestamp_format_ = "%Y-%m-%d %H:%M:%S";
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) Logger::get_instance().debug(msg, __FILE__)
#define LOG_INFO(msg) Logger::get_instance().info(msg, __FILE__)
#define LOG_WARNING(msg) Logger::get_instance().warning(msg, __FILE__)
#define LOG_ERROR(msg) Logger::get_instance().error(msg, __FILE__)
#define LOG_FATAL(msg) Logger::get_instance().fatal(msg, __FILE__)

#define LOG_SCOPE(component, operation) Logger::ScopedLogger scoped_logger(component, operation)
