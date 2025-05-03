#include "Logger.h"
#include "Config.h"

// This file is mostly a placeholder since Logger is implemented as a header-only class.
// It's included here for completeness and to allow for future expansion.

// Initialize the logger with settings from config if available
void InitializeLogger() {
    // Get singleton instance
    Logger& logger = Logger::GetInstance();

    // Set log level from config if available
    std::string configLogLevel = CONFIG.Get<std::string>("logLevel", "INFO");
    LogLevel level = LogLevel::INFO;

    if (configLogLevel == "DEBUG") {
        level = LogLevel::DEBUG;
    } else if (configLogLevel == "INFO") {
        level = LogLevel::INFO;
    } else if (configLogLevel == "WARNING") {
        level = LogLevel::WARNING;
    } else if (configLogLevel == "ERROR") {
        level = LogLevel::ERROR;
    } else if (configLogLevel == "FATAL") {
        level = LogLevel::FATAL;
    }

    logger.SetLogLevel(level);

    // Set log file if enabled in config
    bool logToFile = CONFIG.Get<bool>("logToFile", false);
    std::string logFilename = CONFIG.Get<std::string>("logFilename", "sylva.log");
    
    if (logToFile) {
        logger.SetLogToFile(true, logFilename);
    }

    // Log an initialization message
    LOG_INFO("Logger initialized with level: ", configLogLevel);
} 