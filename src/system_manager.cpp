#include "system_manager.h"
#include "core/logger.h"
#include "core/config.h"
#include "platform/window.h"
#include "platform/input.h"
#include "audio/audio_system.h"
#include "renderer/ui.h"
#include <string>
#include <GLFW/glfw3.h>

namespace Sylva {

bool SystemManager::initializeSystems() {
    // Initialize configuration
    if (!Config::load("config/default_config.ini")) {
        Logger::logError("Failed to load default configuration!");
        return false;
    }
    Config::load("config/user_config.ini");

    // Initialize logger
    Logger::setLogLevel(
        Config::getString("Logging.level") == "DEBUG" ? LogLevel::DEBUG :
        Config::getString("Logging.level") == "WARNING" ? LogLevel::WARNING :
        Config::getString("Logging.level") == "ERROR" ? LogLevel::ERROR :
        LogLevel::INFO
    );
    std::string logFile = Config::getString("Logging.file");
    if (!logFile.empty()) {
        Logger::setLogFile(logFile);
    }
    Logger::logInfo("Sylva Engine starting...");
    Logger::logInfo("Version: 0.1.0");
    Logger::logInfo("Window size: " + 
        std::to_string(Config::getInt("Window.width")) + "x" + 
        std::to_string(Config::getInt("Window.height")));
    Logger::logInfo("Fullscreen: " + 
        std::string(Config::getBool("Window.fullscreen") ? "Yes" : "No"));
    Logger::logInfo("Initializing systems...");

    // Initialize GLFW and Window
    if (!Window::initialize()) {
        Logger::logError("Failed to initialize GLFW!");
        return false;
    }
    return true;
}

void SystemManager::shutdownSystems() {
    Logger::logInfo("Shutting down systems...");
    AudioSystem::shutdown();
}

} // namespace Sylva
