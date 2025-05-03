#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iomanip>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    // Delete copy/move constructors and assign operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void SetLogLevel(LogLevel level) {
        m_LogLevel = level;
    }

    void SetLogToFile(bool logToFile, const std::string& filename = "sylva.log") {
        m_LogToFile = logToFile;
        if (m_LogToFile && !m_LogFile.is_open()) {
            m_LogFile.open(filename, std::ios::out | std::ios::app);
        } else if (!m_LogToFile && m_LogFile.is_open()) {
            m_LogFile.close();
        }
    }

    template<typename... Args>
    void Debug(Args... args) {
        Log(LogLevel::DEBUG, args...);
    }

    template<typename... Args>
    void Info(Args... args) {
        Log(LogLevel::INFO, args...);
    }

    template<typename... Args>
    void Warning(Args... args) {
        Log(LogLevel::WARNING, args...);
    }

    template<typename... Args>
    void Error(Args... args) {
        Log(LogLevel::ERROR, args...);
    }

    template<typename... Args>
    void Fatal(Args... args) {
        Log(LogLevel::FATAL, args...);
    }

private:
    Logger() : m_LogLevel(LogLevel::INFO), m_LogToFile(false) {}
    ~Logger() {
        if (m_LogFile.is_open()) {
            m_LogFile.close();
        }
    }

    template<typename T>
    void LogItem(std::ostream& stream, T value) {
        stream << value;
    }

    template<typename T, typename... Args>
    void LogItem(std::ostream& stream, T value, Args... args) {
        stream << value;
        LogItem(stream, args...);
    }

    template<typename... Args>
    void Log(LogLevel level, Args... args) {
        if (level < m_LogLevel) return;

        const std::lock_guard<std::mutex> lock(m_Mutex);

        std::string levelStr;
        std::ostream* consoleStream = &std::cout;

        switch (level) {
            case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
            case LogLevel::INFO:    levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARNING"; consoleStream = &std::cerr; break;
            case LogLevel::ERROR:   levelStr = "ERROR"; consoleStream = &std::cerr; break;
            case LogLevel::FATAL:   levelStr = "FATAL"; consoleStream = &std::cerr; break;
        }

        // Get current time
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);

        // Format: [LEVEL] [YYYY-MM-DD HH:MM:SS] Message
        std::stringstream messageStream;
        messageStream << "[" << levelStr << "] "
                     << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << "] ";
        LogItem(messageStream, args...);
        messageStream << std::endl;

        std::string message = messageStream.str();
        *consoleStream << message;
        
        if (m_LogToFile && m_LogFile.is_open()) {
            m_LogFile << message;
            m_LogFile.flush();
        }
    }

    LogLevel m_LogLevel;
    bool m_LogToFile;
    std::ofstream m_LogFile;
    std::mutex m_Mutex;
};

// Convenience macros
#define LOG_DEBUG(...) Logger::GetInstance().Debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::GetInstance().Info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::GetInstance().Warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::GetInstance().Error(__VA_ARGS__)
#define LOG_FATAL(...) Logger::GetInstance().Fatal(__VA_ARGS__) 