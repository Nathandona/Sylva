#include "config.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Sylva {

Config::Config() {
    // Initialize config system
}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::logError("Failed to open config file: " + path);
        return false;
    }
    
    Logger::logInfo("Loading configuration from: " + path);

    std::string line;
    std::string currentSection;

    // Lock the whole load: concurrent readers should see either the pre-load
    // state or the fully-loaded state, never a partial one.
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    auto& values = getInstance().m_values;

    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // Section header
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }
        
        // Key-value pairs
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Remove quotes from value if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            
            // Store with section prefix
            std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
            
            // Type guess: bool > float > int > string (fallback).
            if (value == "true" || value == "false") {
                values[fullKey] = (value == "true");
            } else if (value.find('.') != std::string::npos) {
                try { values[fullKey] = std::stof(value); }
                catch (...) { values[fullKey] = value; }
            } else {
                try { values[fullKey] = std::stoi(value); }
                catch (...) { values[fullKey] = value; }
            }
        }
    }
    
    Logger::logInfo("Configuration file loaded: " + path);
    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    
    auto it = getInstance().m_values.find(key);
    if (it != getInstance().m_values.end()) {
        if (std::holds_alternative<std::string>(it->second)) {
            return std::get<std::string>(it->second);
        }
        
        // Try to convert other types to string
        try {
            if (std::holds_alternative<int>(it->second)) {
                return std::to_string(std::get<int>(it->second));
            } else if (std::holds_alternative<float>(it->second)) {
                return std::to_string(std::get<float>(it->second));
            } else if (std::holds_alternative<bool>(it->second)) {
                return std::get<bool>(it->second) ? "true" : "false";
            }
        } catch (...) {
            Logger::logWarning("Failed to convert config value to string: " + key);
        }
    }
    
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    
    auto it = getInstance().m_values.find(key);
    if (it != getInstance().m_values.end()) {
        if (std::holds_alternative<int>(it->second)) {
            return std::get<int>(it->second);
        }
        
        // Try to convert other types to int
        try {
            if (std::holds_alternative<std::string>(it->second)) {
                return std::stoi(std::get<std::string>(it->second));
            } else if (std::holds_alternative<float>(it->second)) {
                return static_cast<int>(std::get<float>(it->second));
            } else if (std::holds_alternative<bool>(it->second)) {
                return std::get<bool>(it->second) ? 1 : 0;
            }
        } catch (...) {
            Logger::logWarning("Failed to convert config value to int: " + key);
        }
    }
    
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    
    auto it = getInstance().m_values.find(key);
    if (it != getInstance().m_values.end()) {
        if (std::holds_alternative<float>(it->second)) {
            return std::get<float>(it->second);
        }
        
        // Try to convert other types to float
        try {
            if (std::holds_alternative<std::string>(it->second)) {
                return std::stof(std::get<std::string>(it->second));
            } else if (std::holds_alternative<int>(it->second)) {
                return static_cast<float>(std::get<int>(it->second));
            } else if (std::holds_alternative<bool>(it->second)) {
                return std::get<bool>(it->second) ? 1.0f : 0.0f;
            }
        } catch (...) {
            Logger::logWarning("Failed to convert config value to float: " + key);
        }
    }
    
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    
    auto it = getInstance().m_values.find(key);
    if (it != getInstance().m_values.end()) {
        if (std::holds_alternative<bool>(it->second)) {
            return std::get<bool>(it->second);
        }
        
        // Try to convert other types to bool
        try {
            if (std::holds_alternative<std::string>(it->second)) {
                std::string value = std::get<std::string>(it->second);
                std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                return value == "true" || value == "1" || value == "yes";
            } else if (std::holds_alternative<int>(it->second)) {
                return std::get<int>(it->second) != 0;
            } else if (std::holds_alternative<float>(it->second)) {
                return std::get<float>(it->second) != 0.0f;
            }
        } catch (...) {
            Logger::logWarning("Failed to convert config value to bool: " + key);
        }
    }
    
    return defaultValue;
}

template<typename T>
void Config::set(const std::string& key, const T& value) {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    getInstance().m_values[key] = value;
}

// Explicit template instantiations
template void Config::set<std::string>(const std::string& key, const std::string& value);
template void Config::set<int>(const std::string& key, const int& value);
template void Config::set<float>(const std::string& key, const float& value);
template void Config::set<bool>(const std::string& key, const bool& value);

void Config::reset() {
    std::lock_guard<std::mutex> lock(getInstance().m_mutex);
    getInstance().m_values.clear();
}

} // namespace Sylva 