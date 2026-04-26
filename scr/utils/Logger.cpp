#include "utils/Logger.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Default to console output only
    console_output_ = true;
    min_level_ = LogLevel::INFO;
}

Logger::~Logger() {
    flush();
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::set_log_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (log_file_.is_open()) {
        log_file_.close();
    }
    std::filesystem::path path(filename);
    std::filesystem::create_directories(path.parent_path());

    log_file_.open(filename, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::set_min_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::set_console_output(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_output_ = enable;
}

void Logger::set_timestamp_format(const std::string& format) {
    std::lock_guard<std::mutex> lock(mutex_);
    timestamp_format_ = format;
}

void Logger::debug(const std::string& message, const std::string& component) {
    log(LogLevel::DEBUG, message, component);
}

void Logger::info(const std::string& message, const std::string& component) {
    log(LogLevel::INFO, message, component);
}

void Logger::warning(const std::string& message, const std::string& component) {
    log(LogLevel::WARNING, message, component);
}

void Logger::error(const std::string& message, const std::string& component) {
    log(LogLevel::ERROR, message, component);
}

void Logger::fatal(const std::string& message, const std::string& component) {
    log(LogLevel::FATAL, message, component);
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::string_to_level(const std::string& level) {
    std::string level_upper = level;
    std::transform(level_upper.begin(), level_upper.end(), level_upper.begin(), ::toupper);

    if (level_upper == "DEBUG") return LogLevel::DEBUG;
    if (level_upper == "INFO") return LogLevel::INFO;
    if (level_upper == "WARNING") return LogLevel::WARNING;
    if (level_upper == "ERROR") return LogLevel::ERROR;
    if (level_upper == "FATAL") return LogLevel::FATAL;

    return LogLevel::INFO;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

void Logger::log(LogLevel level, const std::string& message, const std::string& component) {
    if (level < min_level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::string timestamp = get_timestamp();
    std::string level_str = level_to_string(level);

    std::ostringstream log_line;
    log_line << "[" << timestamp << "] "
             << "[" << level_str << "] ";

    if (!component.empty()) {
        log_line << "[" << component << "] ";
    }

    log_line << message;

    if (console_output_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << log_line.str() << std::endl;
        } else {
            std::cout << log_line.str() << std::endl;
        }
    }

    if (log_file_.is_open()) {
        log_file_ << log_line.str() << std::endl;
    }
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm = *std::localtime(&time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

Logger::ScopedLogger::ScopedLogger(const std::string& component, const std::string& operation)
    : component_(component), operation_(operation) {
    start_ = std::chrono::steady_clock::now();
    Logger::get_instance().debug("Started: " + operation_, component_);
}

Logger::ScopedLogger::~ScopedLogger() {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
    Logger::get_instance().debug("Completed: " + operation_ + " (" +
                                std::to_string(duration.count()) + "ms)", component_);
}
