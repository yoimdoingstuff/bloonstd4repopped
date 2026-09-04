#include "Logger.hpp"

namespace btd4 {

Logger::Logger() {
    // Default console sink
    m_sinks.push_back([](LogLevel level, const std::string& message) {
        std::ostream& out = (level == LogLevel::Error) ? std::cerr : std::cout;
        out << "[" << Logger::levelToString(level) << "] " << message << std::endl;
    });
}

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (static_cast<int>(level) < static_cast<int>(m_minLevel.load(std::memory_order_relaxed))) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& sink : m_sinks) {
        sink(level, message);
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

void Logger::addSink(LogSink sink) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_sinks.push_back(std::move(sink));
}

void Logger::clearSinks() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_sinks.clear();
}

void Logger::setMinLevel(LogLevel level) {
    m_minLevel.store(level, std::memory_order_relaxed);
}

LogLevel Logger::minLevel() const {
    return m_minLevel.load(std::memory_order_relaxed);
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

} // namespace btd4
