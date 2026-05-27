#include "engine.h"
#include "system_manager.h"

#include <iostream>

int main() {
    using namespace Sylva;
    if (!SystemManager::initializeSystems()) {
        std::cerr << "System initialization failed!" << std::endl;
        return 1;
    }
    Engine engine;
    if (!engine.initialize()) {
        std::cerr << "Engine initialization failed!" << std::endl;
        SystemManager::shutdownSystems();
        return 1;
    }
    engine.run();
    engine.shutdown();
    SystemManager::shutdownSystems();
    return 0;
}
