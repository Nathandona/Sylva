#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <atomic>

namespace Sylva {

/**
 * @brief Log level enumeration for controlling log verbosity
 */
enum class LogLevel {
    DEBUG,   // Detailed information for debugging
    INFO,    // General information about program execution
    WARNING, // Potentially harmful situations
    ERROR    // Error events that might still allow the program to continue
};

/**
 * @brief Logger class for centralized logging throughout the engine
 * 
 * Thread-safe singleton logger that supports multiple log levels and
 * can output to console and optionally to a file.
 */
class Logger {
public:
    /**
     * @brief Get the Logger instance
     * @return Reference to the Logger singleton
     */
    static Logger& getInstance();

    /**
     * @brief Log debug information
     * @param message The message to log
     */
    static void logDebug(const std::string& message);

    /**
     * @brief Log general information
     * @param message The message to log
     */
    static void logInfo(const std::string& message);

    /**
     * @brief Log warnings
     * @param message The message to log
     */
    static void logWarning(const std::string& message);

    /**
     * @brief Log errors
     * @param message The message to log
     */
    static void logError(const std::string& message);

    /**
     * @brief Set the minimum log level
     * @param level The minimum log level to display
     */
    static void setLogLevel(LogLevel level);

    /**
     * @brief Set the output log file
     * @param filePath Path to the log file
     * @return true if file was opened successfully, false otherwise
     */
    static bool setLogFile(const std::string& filePath);

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    // Private constructor for singleton pattern
    Logger();
    ~Logger();
    
    // Log a message with a specific level
    void log(LogLevel level, const std::string& message);

    // Current minimum log level (atomic — read on every log call from any thread)
    std::atomic<LogLevel> m_currentLevel{LogLevel::INFO};
    
    // File output stream
    std::ofstream m_fileStream;
    
    // Flag to indicate if file logging is enabled
    bool m_fileLoggingEnabled = false;
    
    // Mutex for thread safety
    std::mutex m_mutex;
    
    // Convert LogLevel to string
    std::string logLevelToString(LogLevel level);
};

} // namespace Sylva 