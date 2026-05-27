#include "logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Sylva {

namespace {
// The default logger lives for the entire process. Reaching it via current()
// means callers never have to worry about a null pointer; tests opt into a
// substitute by calling setCurrent(testLogger), then restore by passing
// nullptr.
Logger& defaultLogger() {
    static Logger instance;
    return instance;
}
} // namespace

Logger* Logger::s_current = nullptr;

Logger::Logger() = default;

Logger::~Logger() {
    // m_fileStream's own destructor closes + flushes. We don't call close()
    // explicitly because it can throw (and ~Logger must be noexcept).
    // If we *were* the active logger, fall back to the default so subsequent
    // logs don't reference us after destruction.
    if (s_current == this) {
        s_current = nullptr;
    }
}

Logger& Logger::current() {
    return s_current ? *s_current : defaultLogger();
}

void Logger::setCurrent(Logger* logger) {
    s_current = logger;
}

void Logger::setLevel(LogLevel level) {
    m_currentLevel.store(level, std::memory_order_relaxed);
}

bool Logger::setFile(const std::string& filePath) {
    std::lock_guard<std::mutex> const lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
        m_fileLoggingEnabled = false;
    }
    m_fileStream.open(filePath, std::ios::out | std::ios::app);
    m_fileLoggingEnabled = m_fileStream.is_open();
    return m_fileLoggingEnabled;
}

void Logger::info(const std::string& m) {
    log(LogLevel::INFO, m);
}
void Logger::warning(const std::string& m) {
    log(LogLevel::WARNING, m);
}
void Logger::error(const std::string& m) {
    log(LogLevel::ERROR, m);
}
void Logger::debug(const std::string& m) {
    log(LogLevel::DEBUG, m);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_currentLevel.load(std::memory_order_relaxed)) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream timeStr;
    timeStr << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    std::stringstream formatted;
    formatted << "[" << timeStr.str() << "] [" << logLevelToString(level) << "] " << message;

    std::lock_guard<std::mutex> const lock(m_mutex);
    std::cout << formatted.str() << '\n';
    if (m_fileLoggingEnabled && m_fileStream.is_open()) {
        m_fileStream << formatted.str() << '\n';
        m_fileStream.flush();
    }
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    }
    return "UNKNOWN";
}

} // namespace Sylva
