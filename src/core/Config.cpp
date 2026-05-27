#include "config.h"
#include "logger.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace Sylva {

namespace {
// Process-wide default Config. Accessible before Engine is constructed so
// early-bootstrap subsystems can still read keys.
Config& defaultConfig() {
    static Config instance;
    return instance;
}
} // namespace

Config* Config::s_current = nullptr;

Config::Config() = default;

Config& Config::current() {
    return s_current ? *s_current : defaultConfig();
}

void Config::setCurrent(Config* config) {
    s_current = config;
}

bool Config::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::logError("Failed to open config file: " + path);
        return false;
    }
    Logger::logInfo("Loading configuration from: " + path);

    Config& cfg = current();
    std::lock_guard<std::mutex> lock(cfg.m_mutex);
    auto& values = cfg.m_values;

    std::string line;
    std::string currentSection;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        const std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;

        if (value == "true" || value == "false") {
            values[fullKey] = (value == "true");
        } else if (value.find('.') != std::string::npos) {
            try {
                values[fullKey] = std::stof(value);
            } catch (...) {
                values[fullKey] = value;
            }
        } else {
            try {
                values[fullKey] = std::stoi(value);
            } catch (...) {
                values[fullKey] = value;
            }
        }
    }
    Logger::logInfo("Configuration file loaded: " + path);
    return true;
}

std::string Config::getString(const std::string& key) {
    return getString(key, "");
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    Config& cfg = current();
    std::lock_guard<std::mutex> lock(cfg.m_mutex);
    auto it = cfg.m_values.find(key);
    if (it == cfg.m_values.end())
        return defaultValue;
    if (std::holds_alternative<std::string>(it->second))
        return std::get<std::string>(it->second);
    if (std::holds_alternative<int>(it->second))
        return std::to_string(std::get<int>(it->second));
    if (std::holds_alternative<float>(it->second))
        return std::to_string(std::get<float>(it->second));
    if (std::holds_alternative<bool>(it->second))
        return std::get<bool>(it->second) ? "true" : "false";
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    Config& cfg = current();
    std::lock_guard<std::mutex> lock(cfg.m_mutex);
    auto it = cfg.m_values.find(key);
    if (it == cfg.m_values.end())
        return defaultValue;
    if (std::holds_alternative<int>(it->second))
        return std::get<int>(it->second);
    try {
        if (std::holds_alternative<std::string>(it->second))
            return std::stoi(std::get<std::string>(it->second));
        if (std::holds_alternative<float>(it->second))
            return static_cast<int>(std::get<float>(it->second));
        if (std::holds_alternative<bool>(it->second))
            return std::get<bool>(it->second) ? 1 : 0;
    } catch (...) {
        Logger::logWarning("Failed to convert config value to int: " + key);
    }
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) {
    Config& cfg = current();
    std::lock_guard<std::mutex> lock(cfg.m_mutex);
    auto it = cfg.m_values.find(key);
    if (it == cfg.m_values.end())
        return defaultValue;
    if (std::holds_alternative<float>(it->second))
        return std::get<float>(it->second);
    try {
        if (std::holds_alternative<std::string>(it->second))
            return std::stof(std::get<std::string>(it->second));
        if (std::holds_alternative<int>(it->second))
            return static_cast<float>(std::get<int>(it->second));
        if (std::holds_alternative<bool>(it->second))
            return std::get<bool>(it->second) ? 1.0f : 0.0f;
    } catch (...) {
        Logger::logWarning("Failed to convert config value to float: " + key);
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    Config& cfg = current();
    std::lock_guard<std::mutex> lock(cfg.m_mutex);
    auto it = cfg.m_values.find(key);
    if (it == cfg.m_values.end())
        return defaultValue;
    if (std::holds_alternative<bool>(it->second))
        return std::get<bool>(it->second);
    try {
        if (std::holds_alternative<std::string>(it->second)) {
            std::string s = std::get<std::string>(it->second);
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s == "true" || s == "1" || s == "yes";
        }
        if (std::holds_alternative<int>(it->second))
            return std::get<int>(it->second) != 0;
        if (std::holds_alternative<float>(it->second))
            return std::get<float>(it->second) != 0.0f;
    } catch (...) {
        Logger::logWarning("Failed to convert config value to bool: " + key);
    }
    return defaultValue;
}

template<typename T> void Config::set(const std::string& key, const T& value) {
    Config& cfg = current();
    std::lock_guard<std::mutex> lock(cfg.m_mutex);
    cfg.m_values[key] = value;
}

template void Config::set<std::string>(const std::string& key, const std::string& value);
template void Config::set<int>(const std::string& key, const int& value);
template void Config::set<float>(const std::string& key, const float& value);
template void Config::set<bool>(const std::string& key, const bool& value);

} // namespace Sylva
