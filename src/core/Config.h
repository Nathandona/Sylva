#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <type_traits>
#include <memory>
#include <vector>
#include <optional>

// Forward declaration of Logger to avoid circular dependency
class Logger;

class Config {
public:
    static Config& GetInstance() {
        static Config instance;
        return instance;
    }

    // Delete copy/move constructors and assign operators
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    // Load configuration from file
    bool LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    // Trim whitespace from key and value
                    key = TrimString(key);
                    value = TrimString(value);
                    m_StringSettings[key] = value;
                }
            }
        }

        // Convert string values to appropriate types
        ProcessLoadedValues();
        return true;
    }

    // Save configuration to file
    bool SaveToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << filename << std::endl;
            return false;
        }

        file << "# Sylva Engine Configuration File\n";
        file << "# Generated on " << GetCurrentDateTimeString() << "\n\n";

        for (const auto& [key, value] : m_StringSettings) {
            file << key << " = " << value << "\n";
        }

        return true;
    }

    // Get a configuration value with a default
    template<typename T>
    T Get(const std::string& key, const T& defaultValue) const {
        if constexpr (std::is_same_v<T, std::string>) {
            auto it = m_StringSettings.find(key);
            if (it != m_StringSettings.end()) {
                return it->second;
            }
        } else if constexpr (std::is_same_v<T, int>) {
            auto it = m_IntSettings.find(key);
            if (it != m_IntSettings.end()) {
                return it->second;
            }
        } else if constexpr (std::is_same_v<T, float>) {
            auto it = m_FloatSettings.find(key);
            if (it != m_FloatSettings.end()) {
                return it->second;
            }
        } else if constexpr (std::is_same_v<T, double>) {
            auto it = m_DoubleSettings.find(key);
            if (it != m_DoubleSettings.end()) {
                return it->second;
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            auto it = m_BoolSettings.find(key);
            if (it != m_BoolSettings.end()) {
                return it->second;
            }
        }
        return defaultValue;
    }

    // Set a configuration value
    template<typename T>
    void Set(const std::string& key, const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            m_StringSettings[key] = value;
        } else if constexpr (std::is_same_v<T, int>) {
            m_IntSettings[key] = value;
            m_StringSettings[key] = std::to_string(value);
        } else if constexpr (std::is_same_v<T, float>) {
            m_FloatSettings[key] = value;
            m_StringSettings[key] = std::to_string(value);
        } else if constexpr (std::is_same_v<T, double>) {
            m_DoubleSettings[key] = value;
            m_StringSettings[key] = std::to_string(value);
        } else if constexpr (std::is_same_v<T, bool>) {
            m_BoolSettings[key] = value;
            m_StringSettings[key] = value ? "true" : "false";
        }
    }

    // Check if a configuration key exists
    bool HasKey(const std::string& key) const {
        return m_StringSettings.find(key) != m_StringSettings.end();
    }

    // Get the asset path (with optional subfolder)
    std::string GetAssetPath(const std::string& subPath = "") const {
        std::string baseAssetPath = Get<std::string>("assetPath", "assets");
        if (subPath.empty()) {
            return baseAssetPath;
        }

        // Ensure path has a trailing slash
        if (baseAssetPath.back() != '/' && baseAssetPath.back() != '\\') {
            baseAssetPath += '/';
        }

        // Remove leading slash from subPath if present
        std::string cleanSubPath = subPath;
        if (!cleanSubPath.empty() && (cleanSubPath.front() == '/' || cleanSubPath.front() == '\\')) {
            cleanSubPath = cleanSubPath.substr(1);
        }

        return baseAssetPath + cleanSubPath;
    }

private:
    Config() {
        // Set default values
        Set<std::string>("assetPath", "assets");
        Set<int>("windowWidth", 1920);
        Set<int>("windowHeight", 1080);
        Set<std::string>("windowTitle", "Sylva");
        Set<bool>("fullscreen", false);
        Set<float>("mouseSensitivity", 0.1f);
        Set<bool>("vsync", true);
    }

    // Process loaded string values into their correct types
    void ProcessLoadedValues() {
        // Clear existing typed maps
        m_IntSettings.clear();
        m_FloatSettings.clear();
        m_DoubleSettings.clear();
        m_BoolSettings.clear();

        // Process each string value
        for (const auto& [key, value] : m_StringSettings) {
            // Try to convert to different types
            if (IsInteger(value)) {
                m_IntSettings[key] = std::stoi(value);
            } else if (IsFloat(value)) {
                m_FloatSettings[key] = std::stof(value);
                m_DoubleSettings[key] = std::stod(value);
            } else if (IsBool(value)) {
                m_BoolSettings[key] = (value == "true" || value == "1" || value == "yes");
            }
            // Keep the string value in all cases
        }
    }

    // Helper functions to check string value types
    bool IsInteger(const std::string& s) const {
        if (s.empty()) return false;
        char* p;
        std::strtol(s.c_str(), &p, 10);
        return (*p == 0);
    }

    bool IsFloat(const std::string& s) const {
        if (s.empty()) return false;
        char* p;
        std::strtof(s.c_str(), &p);
        return (*p == 0);
    }

    bool IsBool(const std::string& s) const {
        return s == "true" || s == "false" || s == "1" || s == "0" || s == "yes" || s == "no";
    }

    // Trim whitespace from a string
    std::string TrimString(const std::string& str) const {
        const auto start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";

        const auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // Get current date and time as a string
    std::string GetCurrentDateTimeString() const {
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
        return std::string(buffer);
    }

    std::unordered_map<std::string, std::string> m_StringSettings;
    std::unordered_map<std::string, int> m_IntSettings;
    std::unordered_map<std::string, float> m_FloatSettings;
    std::unordered_map<std::string, double> m_DoubleSettings;
    std::unordered_map<std::string, bool> m_BoolSettings;
};

// Convenience macro
#define CONFIG Config::GetInstance() 