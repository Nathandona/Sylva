#pragma once

#include <string>
#include <map>
#include <any>
#include <memory>
#include <variant>
#include <mutex>

namespace Sylva {

/**
 * @brief Configuration system for engine and game settings
 * 
 * Loads engine and game settings from .ini files, supporting
 * default and user-specific overrides. Provides access to
 * configuration values throughout the engine.
 */
class Config {
public:
    /**
     * @brief Get the Config instance
     * @return Reference to the Config singleton
     */
    static Config& getInstance();

    /**
     * @brief Load configuration from a file
     * @param path Path to the configuration file
     * @return true if file was loaded successfully, false otherwise
     */
    static bool load(const std::string& path);

    /**
     * @brief Get a string value from the configuration
     * @param key The key to look up
     * @param defaultValue The default value to return if key is not found
     * @return The value associated with the key, or defaultValue if not found
     */
    static std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief Get an integer value from the configuration
     * @param key The key to look up
     * @param defaultValue The default value to return if key is not found
     * @return The value associated with the key, or defaultValue if not found
     */
    static int getInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief Get a float value from the configuration
     * @param key The key to look up
     * @param defaultValue The default value to return if key is not found
     * @return The value associated with the key, or defaultValue if not found
     */
    static float getFloat(const std::string& key, float defaultValue = 0.0f);

    /**
     * @brief Get a boolean value from the configuration
     * @param key The key to look up
     * @param defaultValue The default value to return if key is not found
     * @return The value associated with the key, or defaultValue if not found
     */
    static bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief Set a value in the configuration
     * @param key The key to set
     * @param value The value to set
     */
    template<typename T>
    static void set(const std::string& key, const T& value);

    // Delete copy constructor and assignment operator
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

private:
    // Private constructor for singleton pattern
    Config();
    
    // Configuration values
    using ConfigValue = std::variant<std::string, int, float, bool>;
    std::map<std::string, ConfigValue> m_values;
    
    // Mutex for thread safety
    std::mutex m_mutex;
    
    // Parse a section.key notation
    std::pair<std::string, std::string> parseKey(const std::string& key);
};

} // namespace Sylva 