#pragma once

#include <string>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>

namespace btd4 {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    using LogSink = std::function<void(LogLevel level, const std::string& message)>;

    static Logger& instance();

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    void addSink(LogSink sink);
    void clearSinks();
    void setMinLevel(LogLevel level);
    LogLevel minLevel() const;

    static const char* levelToString(LogLevel level);

private:
    Logger();
    ~Logger() = default;

    std::atomic<LogLevel> m_minLevel{LogLevel::Info};
    std::vector<LogSink> m_sinks;
    mutable std::recursive_mutex m_mutex;
};

#define BTD4_LOG_DEBUG(msg) ::btd4::Logger::instance().debug(msg)
#define BTD4_LOG_INFO(msg)  ::btd4::Logger::instance().info(msg)
#define BTD4_LOG_WARN(msg)  ::btd4::Logger::instance().warn(msg)
#define BTD4_LOG_ERROR(msg) ::btd4::Logger::instance().error(msg)

} // namespace btd4
