#include "Config.h"
#include "Logger.h"
#include <filesystem>

// Most functionality is implemented as templates in the header,
// this file provides additional utility functions

// Initialize default configuration
void InitializeConfig(int argc, char* argv[]) {
    Config& config = CONFIG;
    
    // Try to load config file from default location
    std::string configPath = "config.ini";
    
    // Check if there's a command-line override for config file
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--config" || arg == "-c") {
            configPath = argv[i + 1];
            break;
        }
    }
    
    // Load configuration from file if it exists
    if (std::filesystem::exists(configPath)) {
        if (config.LoadFromFile(configPath)) {
            std::cout << "Loaded configuration from: " << configPath << std::endl;
        } else {
            std::cerr << "Failed to load configuration from: " << configPath << std::endl;
        }
    } else {
        // Create a default configuration file
        config.SaveToFile(configPath);
        std::cout << "Created default configuration file: " << configPath << std::endl;
    }
    
    // Process command line arguments to override config
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // Skip the config file argument which we already processed
        if ((arg == "--config" || arg == "-c") && i < argc - 1) {
            i++; // Skip the next argument (config path)
            continue;
        }
        
        // Check for key=value pairs
        size_t equalsPos = arg.find('=');
        if (equalsPos != std::string::npos && equalsPos > 0) {
            std::string key = arg.substr(0, equalsPos);
            std::string value = arg.substr(equalsPos + 1);
            
            // Store in the string settings map
            config.Set<std::string>(key, value);
            std::cout << "Set configuration " << key << " = " << value << " from command line" << std::endl;
        }
    }
} 