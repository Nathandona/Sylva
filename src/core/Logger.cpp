#include "logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

namespace Sylva {

Logger::Logger() {
    // Initialize logger
}

Logger::~Logger() {
    // Close file if open
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::logDebug(const std::string& message) {
    getInstance().log(LogLevel::DEBUG, message);
}

void Logger::logInfo(const std::string& message) {
    getInstance().log(LogLevel::INFO, message);
}

void Logger::logWarning(const std::string& message) {
    getInstance().log(LogLevel::WARNING, message);
}

void Logger::logError(const std::string& message) {
    getInstance().log(LogLevel::ERROR, message);
}

void Logger::setLogLevel(LogLevel level) {
    getInstance().m_currentLevel.store(level, std::memory_order_relaxed);
}

void Logger::reset() {
    Logger& inst = getInstance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    if (inst.m_fileStream.is_open()) {
        inst.m_fileStream.close();
    }
    inst.m_fileLoggingEnabled = false;
    inst.m_currentLevel.store(LogLevel::INFO, std::memory_order_relaxed);
}

bool Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    
    // Close existing file if open
    if (getInstance().m_fileStream.is_open()) {
        getInstance().m_fileStream.close();
        getInstance().m_fileLoggingEnabled = false;
    }
    
    // Open new file
    getInstance().m_fileStream.open(filePath, std::ios::out | std::ios::app);
    
    if (getInstance().m_fileStream.is_open()) {
        getInstance().m_fileLoggingEnabled = true;
        return true;
    }
    
    return false;
}

void Logger::log(LogLevel level, const std::string& message) {
    // Skip if level is below current log level
    if (level < m_currentLevel.load(std::memory_order_relaxed)) {
        return;
    }
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream timeStr;
    timeStr << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    // Format message
    std::stringstream formattedMessage;
    formattedMessage << "[" << timeStr.str() << "] [" << logLevelToString(level) << "] " << message;
    
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Output to console
    std::cout << formattedMessage.str() << std::endl;
    
    // Output to file if enabled
    if (m_fileLoggingEnabled && m_fileStream.is_open()) {
        m_fileStream << formattedMessage.str() << std::endl;
        m_fileStream.flush();
    }
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

} // namespace Sylva 