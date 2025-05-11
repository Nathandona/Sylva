#pragma once

#include <string>

namespace Sylva {

class SystemManager {
public:
    static bool initializeSystems();
    static void shutdownSystems();
};

} // namespace Sylva 