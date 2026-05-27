#include "engine.h"

#include <iostream>

int main() {
    Sylva::Engine engine;
    if (!engine.initialize()) {
        std::cerr << "Engine initialization failed!" << std::endl;
        return 1;
    }
    engine.run();
    engine.shutdown();
    return 0;
}
