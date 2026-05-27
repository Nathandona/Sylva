#pragma once

#include <map>
#include <mutex>
#include <string>
#include <variant>

namespace Sylva {

/**
 * @brief Configuration store with a service-locator surface.
 *
 * Config is a real class with per-instance state — there is no global
 * key/value map. All static accessors route through the *current* Config
 * (a registered instance), so existing call sites (Config::getInt(...))
 * keep working while tests can swap in their own Config:
 *
 *     Config testCfg;
 *     Config::setCurrent(&testCfg);
 *     Config::set("Player.move_speed", 9.0f);     // populates testCfg
 *     float v = Config::getFloat("Player.move_speed");   // 9.0
 *     Config::setCurrent(nullptr);                // restore default
 *
 * A process-wide default Config exists from program start so logging /
 * subsystems can read values during early bootstrap.
 */
class Config {
public:
    Config();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // --- Static API (all calls route through current()) ---
    static bool load(const std::string& path);

    static std::string getString(const std::string& key);
    static std::string getString(const std::string& key, const std::string& defaultValue);
    static int getInt(const std::string& key, int defaultValue = 0);
    static float getFloat(const std::string& key, float defaultValue = 0.0f);
    static bool getBool(const std::string& key, bool defaultValue = false);

    template<typename T> static void set(const std::string& key, const T& value);

    // --- Service locator ---
    static Config& current();
    static void setCurrent(Config* config);

private:
    using ConfigValue = std::variant<std::string, int, float, bool>;
    std::map<std::string, ConfigValue> m_values;
    mutable std::mutex m_mutex;

    static Config* s_current;
};

} // namespace Sylva
