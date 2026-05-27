#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <atomic>

namespace Sylva {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

/**
 * @brief Thread-safe logger with a service-locator surface.
 *
 * Logger is a real instantiable class: production code constructs one (or
 * uses the default fallback), tests construct their own and swap it in via
 * setCurrent(). All log output goes through the *current* logger, which the
 * static convenience methods (logInfo / logWarning / logError / logDebug)
 * resolve at call time.
 *
 * The default Logger always exists so callers can log during static init or
 * before Engine has run — there is never a null pointer to dereference.
 */
class Logger {
public:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // --- Instance API (virtual so tests can derive a TestLogger that
    // captures lines into a vector for assertions) ---
    virtual void info(const std::string& message);
    virtual void warning(const std::string& message);
    virtual void error(const std::string& message);
    virtual void debug(const std::string& message);

    virtual void setLevel(LogLevel level);
    /**
     * @brief Redirect logger output to a file (in addition to stdout).
     * @return true if the file was opened successfully.
     */
    bool setFile(const std::string& filePath);

    // --- Service locator ---
    /**
     * @brief Currently-active logger. Defaults to a process-wide instance.
     */
    static Logger& current();

    /**
     * @brief Replace the active logger (e.g. a TestLogger that captures
     *        output into a vector). Pass nullptr to restore the default.
     */
    static void setCurrent(Logger* logger);

    // --- Static facade (forwards to current()) ---
    // Kept so existing call sites Logger::logInfo("...") continue to work
    // without 195 mechanical edits across the codebase.
    static void logInfo(const std::string& m) { current().info(m); }
    static void logWarning(const std::string& m) { current().warning(m); }
    static void logError(const std::string& m) { current().error(m); }
    static void logDebug(const std::string& m) { current().debug(m); }
    static void setLogLevel(LogLevel l) { current().setLevel(l); }
    static bool setLogFile(const std::string& p) { return current().setFile(p); }

private:
    void log(LogLevel level, const std::string& message);
    std::string logLevelToString(LogLevel level);

    std::atomic<LogLevel> m_currentLevel{LogLevel::INFO};
    std::ofstream m_fileStream;
    bool m_fileLoggingEnabled = false;
    std::mutex m_mutex;

    static Logger* s_current;
};

} // namespace Sylva
